#ifndef _GRIDUINO_MODEL_CPP
#define _GRIDUINO_MODEL_CPP

/* File: model.cpp
   Project: Griduino by Barry K7BWH
*/

#include <Arduino.h>
#include <Adafruit_GPS.h>         // Ultimate GPS library
#include "constants.h"

// ---------- global variables ----------
// prefix 'g' for global, 'gs' for global string, 'ga' for global array

// ------------ typedef's

// ------------ definitions

// ========== extern ==================================
extern Adafruit_GPS GPS;
String calcLocator(double lat, double lon);
String calcDistanceLat(double fromLat, double toLat);
String calcDistanceLong(double lat, double fromLong, double toLong);
float nextGridLineEast(float longitudeDegrees);
float nextGridLineWest(float longitudeDegrees);
float nextGridLineSouth(float latitudeDegrees);

// ========== constants =======================
// These initial values are displayed until GPS gets its position lock.

#define INIT_GRID6 "CN77tt";   // initialize to a nearby grid for demo but not
#define INIT_GRID4 "CN77";     // to my home grid CN87, so that GPS lock will announce where we are in Morse code

#define INIT_LAT  "waiting";   // show this after losing GPS lock
#define INIT_LONG " for GPS";  //

// ========== class Model ======================
class Model {
  public:
    // Class member variables
    double gLatitude = 0;             // GPS position, floating point, decimal degrees
    double gLongitude = 0;            // GPS position, floating point, decimal degrees
    float  gAltitude = 0;             // Altitude in meters above MSL
    bool   gHaveGPSfix = false;       // true = GPS.fix() = whether or not gLatitude/gLongitude is valid
    uint8_t gSatellites = 0;          // number of satellites in use
    float  gSpeed = 0.0;              // current speed over ground in MPH
    float  gAngle = 0.0;              // direction of travel, degrees from true north

    String gsLatitude = INIT_LAT;     // GPS position, string, decimal degrees
    String gsLongitude = INIT_LONG;

    String gsGridName = INIT_GRID6;   // e.g. "CN77tt", placeholder grids to show at power-on
    bool   grid4dirty = true;         // true = need to update grid4 displayed text (set by model, cleared by view)
    bool   grid6dirty = true;         // true = need to update grid6 displayed text (set by model, cleared by view)

    String gsGridNorth = "CN78";      // e.g. "CN78"
    String gsGridEast = "CN87";       // e.g. "CN87"
    String gsGridSouth = "CN76";      // e.g. "CN76"
    String gsGridWest = "CN67";       // e.g. "CN67"
    String gsDistanceNorth = "17.3";  // distance in miles to next grid crossing
    String gsDistanceEast = "15.5";   // placeholder distances to show at power-on
    String gsDistanceSouth = "63.4";
    String gsDistanceWest = "81.4";   // Strings: https://www.arduino.cc/en/Reference/StringLibrary
  protected:
    int    gPrevFix = false;          // previous value of GPS.fix(), to help detect "signal lost"
    String sPrevGrid4 = INIT_GRID4;   // previous value of gsGridName, to help detect "enteredNewGrid()"
    String sPrevGrid6 = "";           // previous value of gsGridName

  public:
    // Constructor - create and initialize member variables
    Model() { }

    // the Model will update its internal state on a schedule determined by the Controller
    void processGPS() {
      echoGPSinfo();    // send GPS statistics to serial console for debug

      if (GPS.fix) {
        // update model
        gLatitude = GPS.latitudeDegrees;    // position as double-precision float
        gLongitude = GPS.longitudeDegrees;
        gAltitude = GPS.altitude;

        gsLatitude = String(GPS.latitudeDegrees, 4);   // compatible with TFT display
        gsLongitude = String(GPS.longitudeDegrees, 4);
        gsGridName = ::calcLocator(GPS.latitudeDegrees, GPS.longitudeDegrees);
        gsGridNorth = ::calcLocator(GPS.latitudeDegrees + 1.0, GPS.longitudeDegrees).substring(0, 4);
        gsGridSouth = ::calcLocator(GPS.latitudeDegrees - 1.0, GPS.longitudeDegrees).substring(0, 4);
        gsGridEast = ::calcLocator(GPS.latitudeDegrees, GPS.longitudeDegrees + 2.0).substring(0, 4);
        gsGridWest = ::calcLocator(GPS.latitudeDegrees, GPS.longitudeDegrees - 2.0).substring(0, 4);

        // N-S: find nearest integer grid lines
        gsDistanceNorth = ::calcDistanceLat(GPS.latitudeDegrees, ceil(GPS.latitudeDegrees));
        gsDistanceSouth = ::calcDistanceLat(GPS.latitudeDegrees, floor(GPS.latitudeDegrees));

        // E-W: find nearest EVEN numbered grid lines
        int eastLine = ::nextGridLineEast(GPS.longitudeDegrees);
        int westLine = ::nextGridLineWest(GPS.longitudeDegrees);

        gsDistanceEast = ::calcDistanceLong(GPS.latitudeDegrees, GPS.longitudeDegrees, eastLine);
        gsDistanceWest = ::calcDistanceLong(GPS.latitudeDegrees, GPS.longitudeDegrees, westLine);

        String grid4 = gsGridName.substring(0, 4);
        String grid6 = gsGridName.substring(4, 6);
        if (grid4 != sPrevGrid4) {
          grid4dirty = true;
        }
        if (grid6 != sPrevGrid6) {
          grid6dirty = true;
          sPrevGrid6 = grid6;
        }

        gHaveGPSfix = true;
      } else {
        // nothing - leave current info as-is and displayed on screen
        // it's not unusual to lose GPS lock so don't distract driver
        gHaveGPSfix = false;
      }
      gSatellites = GPS.satellites;
      gSpeed = GPS.speed * mphPerKnots;
      gAngle = GPS.angle;
    }

    bool enteredNewGrid() {
      // grid-crossing detector
      // returns TRUE if the first four characters of 'gsGridName' have changed
      String newGrid4 = gsGridName.substring(0, 4);
      if (newGrid4 != sPrevGrid4) {
        Serial.print("Prev grid: "); Serial.print(sPrevGrid4);
        Serial.print(" New grid: "); Serial.println(gsGridName);
        sPrevGrid4 = newGrid4;
        grid4dirty = true;      // 'dirty' flags are set by model, cleared by view
        grid6dirty = true;
        return true;
      } else {
        return false;
      }
    }
    bool signalLost() {
      // GPS "signal lost" detector
      // returns TRUE only once on the transition from GPS lock to no-lock
      bool lostFix;
      if (gPrevFix && !GPS.fix) {
        lostFix = true;
        gHaveGPSfix = false;
        Serial.println("Lost GPS signal.");
      } else {
        lostFix = false;
      }
      gPrevFix = GPS.fix;
      return lostFix;
    }

    void indicateSignalLost() {
      // we want SOME indication to not trust the readings
      // but make it low-key to not distract the driver
      // TODO - architecturally, it seems like this subroutine should be part of the view (not model)
      gsLatitude = INIT_LAT;    // GPS position, string
      gsLongitude = INIT_LONG;
    }

    // Formatted GMT time
    void getTime(char* result) {
      // result = char[10] = string buffer to modify
        int hh = GPS.hour;
        int mm = GPS.minute;
        int ss = GPS.seconds;
        snprintf(result, 10, "%02d:%02d:%02d",
                               hh,  mm,  ss);
    }

    // Does the GPS report a valid date?
    bool isDateValid(int yy, int mm, int dd) {
      if (yy < 19) {
        return false;
      }
      if (mm < 1 || mm > 12) {
        return false;
      }
      if (dd < 1 || dd > 31) {
        return false;
      }
      return true;
    }

    // Formatted GMT date "Jan 12, 2020"
    void getDate(char* result, int maxlen) {
      // @param result = char[15] = string buffer to modify
      // @param maxlen = string buffer length
      // Note that GPS can have a valid date without a position; we can't rely on GPS.fix()
      // to know if the date is correct or not. So we deduce it from the y/m/d values.
      char sDay[3];       // "12"
      char sYear[5];      // "2020"
      char aMonth[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "err" };

      uint8_t yy = GPS.year;
      int mm = GPS.month;
      int dd = GPS.day;

      if (isDateValid(yy,mm,dd)) {
        int year = yy + 2000;     // convert two-digit year into four-digit integer
        int month = mm - 1;       // GPS month is 1-based, our array is 0-based
        snprintf(result, maxlen, "%s %d, %4d", aMonth[month], dd, year);
      } else {
        // GPS does not have a valid date, we will display it as "0000-00-00"
        snprintf(result, maxlen, "0000-00-00");
      }
    }
    
    // Provide pre-formatted GMT date/time "2019-12-31  10:11:12"
    void getDateTime(char* result) {
      // result = char[25] = string buffer to modify
      //if (GPS.fix) {
        int yy = GPS.year;
        if (yy >= 19) {
          // if GPS reports a date before 19, then it's bogus
          // and it's displayed as-is
          yy += 2000;
        }
        int mo = GPS.month;
        int dd = GPS.day;
        int hh = GPS.hour;
        int mm = GPS.minute;
        int ss = GPS.seconds;
        snprintf(result, 25, "%04d-%02d-%02d  %02d:%02d:%02d",
                              yy,  mo,  dd,  hh,  mm,  ss);
      //} else {
      //  strncpy(result, "0000-00-00 hh:mm:ss GMT", 25);
      //}
    }

  private:
    void echoGPSinfo() {
      #ifdef ECHO_GPS
        // send GPS statistics to serial console for desktop debugging
        char sDate[20];         // strlen("0000-00-00 hh:mm:ss") = 19
        getDateTime(sDate);
        Serial.print("Model: ");
        Serial.print(sDate);
        Serial.print("  Fix("); Serial.print((int)GPS.fix); Serial.println(")");
  
        if (GPS.fix) {
          //~Serial.print("   Loc("); //~Serial.print(gsLatitude); //~Serial.print(","); //~Serial.print(gsLongitude);
          //Serial.print(") Quality("); //~Serial.print((int)GPS.fixquality);
          //~Serial.print(") Sats("); //~Serial.print((int)GPS.satellites);
          //~Serial.print(") Speed("); //~Serial.print(GPS.speed); //~Serial.print(" knots");
          //~Serial.print(") Angle("); //~Serial.print(GPS.angle);
          //~Serial.print(") Alt("); //~Serial.print(GPS.altitude);
          //~Serial.println(")");
        }
      #endif
    }
};

#endif // _GRIDUINO_MODEL_CPP
