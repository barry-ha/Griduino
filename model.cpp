#ifndef _GRIDUINO_MODEL_CPP
#define _GRIDUINO_MODEL_CPP

/* File: model.cpp
   Project: Griduino by Barry K7BWH
*/

#include <Arduino.h>
#include <Adafruit_GPS.h>         // Ultimate GPS library
#include "constants.h"              // Griduino constants and colors
#include "save_restore.h"           // Configuration data in nonvolatile RAM

// ========== extern ==================================
extern Adafruit_GPS GPS;
String calcLocator(double lat, double lon);           // Griduino.ino
double calcDistanceLat(double fromLat, double toLat); // Griduino.ino
double calcDistanceLong(double lat, double fromLong, double toLong);  // Griduino.ino
float nextGridLineEast(float longitudeDegrees);
float nextGridLineWest(float longitudeDegrees);
float nextGridLineSouth(float latitudeDegrees);
bool isVisibleDistance(const PointGPS from, const PointGPS to); // view_grid.cpp

// ========== constants =======================
// These initial values are displayed until GPS gets comes up with better info.

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

    String gsGridNorth = "CN78";      // e.g. "CN78"
    String gsGridEast = "CN87";       // e.g. "CN87"
    String gsGridSouth = "CN76";      // e.g. "CN76"
    String gsGridWest = "CN67";       // e.g. "CN67"
    String gsDistanceNorth = "17.3";  // distance in miles to next grid crossing
    String gsDistanceEast = "15.5";   // placeholder distances to show at power-on
    String gsDistanceSouth = "63.4";
    String gsDistanceWest = "81.4";   // Strings: https://www.arduino.cc/en/Reference/StringLibrary

    Location history[200];            // remember a list of GPS coordinates
    int nextHistoryItem = 0;          // index of next item to write
    const int numHistory = sizeof(history)/sizeof(Location);

  protected:
    int    gPrevFix = false;          // previous value of GPS.fix(), to help detect "signal lost"
    String sPrevGrid4 = INIT_GRID4;   // previous value of gsGridName, to help detect "enteredNewGrid()"

  public:
    // Constructor - create and initialize member variables
    Model() { }

    // save current GPS state to non-volatile memory
    const char MODEL_FILE[25] = "/Griduino/gpsmodel.cfg";  // CONFIG_FOLDER
    const char MODEL_VERS[15] = "GPS Model v01";
    int save() {
      SaveRestore sdram(MODEL_FILE, MODEL_VERS);
      if (sdram.writeConfig( (byte*) this, sizeof(Model))) {
        Serial.println("Success, GPS Model stored to SDRAM");
      } else {
        Serial.println("ERROR! Failed to save GPS Model object to SDRAM");
        return 0;     // return failure
      }
      return 1;       // return success
    }

    // restore current GPS state from non-volatile memory
    int restore() {
      SaveRestore sdram(MODEL_FILE, MODEL_VERS);
      Model tempModel;
      if (sdram.readConfig( (byte*) &tempModel, sizeof(Model))) {
        // warning: this can corrupt our object's data if something failed
        // so we blob the bytes to a work area and copy individual values
        Serial.println("Success, GPS Model restored from SDRAM");
        copyFrom( tempModel );
      } else {
        Serial.println("ERROR! Failed to restore GPS Model object to SDRAM");
        return 0;     // return failure
      }
      // note: the caller is responsible for fixups to the model, e.g., indicate 'lost satellite signal'
      return 1;       // return success
    }

    // pick'n pluck values from another "Model" instance
    void copyFrom(const Model from) {
      return;   // debug
      
      gLatitude = from.gLatitude;     // GPS position, floating point, decimal degrees
      gLongitude = from.gLongitude;   // GPS position, floating point, decimal degrees
      gAltitude = from.gAltitude;     // Altitude in meters above MSL
      gHaveGPSfix = false;            // assume no fix yet
      gSatellites = 0;                // assume no satellites yet
      gSpeed = 0.0;                   // assume speed unknown
      gAngle = 0.0;                   // assume direction of travel unknown

      gsLatitude = from.gsLatitude;   // GPS position, string, decimal degrees
      gsLongitude = from.gsLongitude;

      gsGridName = from.gsGridName;

      gsGridNorth = from.gsGridNorth; // e.g. "CN78"
      gsGridEast = from.gsGridEast;   // e.g. "CN87"
      gsGridSouth = from.gsGridSouth; // e.g. "CN76"
      gsGridWest = from.gsGridWest;   // e.g. "CN67"
      gsDistanceNorth = from.gsDistanceNorth;  // distance in miles to next grid crossing
      gsDistanceEast = from.gsDistanceEast;
      gsDistanceSouth = from.gsDistanceSouth;
      gsDistanceWest = from.gsDistanceWest;

      for (int ii=0; ii<numHistory; ii++) {
        history[ii] = from.history[ii];
      }
      nextHistoryItem = from.nextHistoryItem;
    }

    // read GPS hardware
    virtual void getGPS() {
      if (GPS.fix) {
        gLatitude = GPS.latitudeDegrees;    // position as double-precision float
        gLongitude = GPS.longitudeDegrees;
        gAltitude = GPS.altitude;
        gHaveGPSfix = true;
      } else {
        gHaveGPSfix = false;
      }

      // read hardware regardless of GPS signal acquisition
      gSatellites = GPS.satellites;
      gSpeed = GPS.speed * mphPerKnots;
      gAngle = GPS.angle;
    }

    // the Model will update its internal state on a schedule determined by the Controller
    void processGPS() {
      getGPS();         // read the hardware for location
      echoGPSinfo();    // send GPS statistics to serial console for debug

      PointGPS whereAmI{gLatitude, gLongitude};
      remember(whereAmI, GPS.hour, GPS.minute, GPS.seconds);

      if (GPS.fix) {
        // update model
        gsLatitude = String(gLatitude, 4);   // compatible with TFT display
        gsLongitude = String(gLongitude, 4);
        gsGridName = ::calcLocator(gLatitude, gLongitude);
        gsGridNorth = ::calcLocator(gLatitude + 1.0, gLongitude).substring(0, 4);
        gsGridSouth = ::calcLocator(gLatitude - 1.0, gLongitude).substring(0, 4);
        gsGridEast = ::calcLocator(gLatitude, gLongitude + 2.0).substring(0, 4);
        gsGridWest = ::calcLocator(gLatitude, gLongitude - 2.0).substring(0, 4);

        // N-S: find nearest integer grid lines
        gsDistanceNorth = ::calcDistanceLat(gLatitude, ceil(gLatitude));
        gsDistanceSouth = ::calcDistanceLat(gLatitude, floor(gLatitude));

        // E-W: find nearest EVEN numbered grid lines
        int eastLine = ::nextGridLineEast(gLongitude);
        int westLine = ::nextGridLineWest(gLongitude);

        gsDistanceEast = ::calcDistanceLong(gLatitude, gLongitude, eastLine);
        gsDistanceWest = ::calcDistanceLong(gLatitude, gLongitude, westLine);

        String grid4 = gsGridName.substring(0, 4);
        String grid6 = gsGridName.substring(4, 6);

      } else {
        // nothing - leave current info as-is and displayed on screen
        // it's not unusual to lose GPS lock so don't distract driver
      }
    }

    void clearHistory() {
      // wipe clean the array of lat/long that we remember
      for (uint ii=0; ii<numHistory; ii++) {
        history[ii].reset();
      }
      nextHistoryItem = 0;
    }
    void remember(PointGPS vLoc, int vHour, int vMinute, int vSecond) {
      // save this GPS location and timestamp in internal array
      // so that we can display it as a breadcrumb trail

      int prevIndex = nextHistoryItem - 1;  // find prev location in circular buffer
      if (prevIndex < 0) {
        prevIndex = numHistory-1;
      }
      PointGPS prevLoc = history[prevIndex].loc;
      if (isVisibleDistance(vLoc, prevLoc)) {
        history[nextHistoryItem].loc = vLoc;
        history[nextHistoryItem].hh = vHour;
        history[nextHistoryItem].mm = vMinute;
        history[nextHistoryItem].ss = vSecond;

        nextHistoryItem = (++nextHistoryItem % numHistory);
      }
    }
    void dumpHistory() {
      Serial.println("History review........");
      Serial.print("Number of items = "); Serial.println(numHistory);
      for (int ii=0; ii<5 /*numHistory*/; ii++) {
        Location item = history[ii];
        Serial.print(ii); 
        Serial.print(". GPS("); 
        Serial.print(item.loc.lat,3); Serial.print(",");
        Serial.print(item.loc.lng,3); Serial.print(") ");
        Serial.print("Time("); 
        Serial.print(item.hh); Serial.print(":"); 
        Serial.print(item.mm); Serial.print(":"); 
        Serial.print(item.ss); Serial.print(")");
        Serial.println(" ");
      }
    }
    bool enteredNewGrid() {
      // grid-crossing detector
      // returns TRUE if the first four characters of 'gsGridName' have changed
      String newGrid4 = gsGridName.substring(0, 4);
      if (newGrid4 != sPrevGrid4) {
        Serial.print("Prev grid: "); Serial.print(sPrevGrid4);
        Serial.print(" New grid: "); Serial.println(gsGridName);
        sPrevGrid4 = newGrid4;
        return true;
      } else {
        return false;
      }
    }
    bool signalLost() {
      // GPS "lost satellite lock" detector
      // returns TRUE only once on the transition from GPS lock to no-lock
      bool lostFix = false;     // assume no transition
      if (gPrevFix && !GPS.fix) {
        lostFix = true;
        gHaveGPSfix = false;
        Serial.println("Lost GPS positioning");
      } else if (!gPrevFix && GPS.fix) {
        Serial.println("Acquired GPS position lock");
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
    
    // Provide formatted GMT date/time "2019-12-31  10:11:12"
    void getDateTime(char* result) {
      // input: result = char[25] = string buffer to modify
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
          //~Serial.print(") Quality("); //~Serial.print((int)GPS.fixquality);
          //~Serial.print(") Sats("); //~Serial.print((int)GPS.satellites);
          //~Serial.print(") Speed("); //~Serial.print(GPS.speed); //~Serial.print(" knots");
          //~Serial.print(") Angle("); //~Serial.print(GPS.angle);
          //~Serial.print(") Alt("); //~Serial.print(GPS.altitude);
          //~Serial.println(")");
        }
      #endif
    }
};
// ========== class MockModel ======================
// A derived class with a single replacement function to pretend
// about the GPS location for the sake fo testing.

class MockModel : public Model {
  // Generate a simulated travel path for demonstrations and testing
  // Note: GPS Position: Simulated
  //       Realtime Clock: Actual time as given by hardware

  protected:
    const double lngDegreesPerPixel = gridWidthDegrees / gBoxWidth;    // grid square = 2.0 degrees wide E-W
    const double latDegreesPerPixel = gridHeightDegrees / gBoxHeight;  // grid square = 1.0 degrees high N-S
  
    const PointGPS midCN87{47.50, -123.00};  // center of CN87
    const PointGPS llCN87 {47.40, -123.35};  // lower left of CN87
    unsigned long startTime = 0;
  
  public:
    // SIMULATED read GPS hardware
    void getGPS() {
      if (GPS.fix) {
        gLatitude = llCN87.lat;
        gLongitude = llCN87.lng;
        gAltitude = GPS.altitude;

        if (startTime == 0) {
          // one-time single initialization
          startTime = millis();
        }
        
        switch (2) {
          // Simulated GPS readings
          case 0: // ----- move slowly NORTH forever from starting position
            gLatitude = midCN87.lat + (millis() - startTime) / 1000.0 / gBoxHeight;
            gLongitude = midCN87.lng;
            break;

          case 1: // ----- move slowly EAST forever from starting position
            gLatitude = llCN87.lat;
            gLongitude = llCN87.lng + (millis() - startTime) / 1000.0 / gBoxWidth;
            break;

          case 2: // ----- move slowly NE forever from starting position
            gLatitude = midCN87.lat + (millis() - startTime) / 1000.0 / gBoxHeight;
            gLongitude = llCN87.lng + (millis() - startTime) / 1000.0 / gBoxWidth;
            break;

          case 3:
            // --------------------
            // move by 70% of a grid square north, over a span of one minute
            gLatitude += GPS.seconds/60.0*0.70;

            // sin wave, amplitude is 70% width of 2-degree grid, complete sine wave period in one minute
            gLongitude += 0.7 * gridWidthDegrees * sin((GPS.minute*60.0 + GPS.seconds)/600.0 * 2.0 * PI);

            // Simulated altitude (meters)
            gAltitude += float(GPS.seconds)/60.0*100.0;
            break;
        }
        // position as double-precision float
        //gLatitude = GPS.latitudeDegrees + gridHeightDegrees;  // e.g. CN87 -> CN88
        //gLongitude = GPS.longitudeDegrees + gridWidthDegrees; // e.g. CN87 -> CN97
        
        gHaveGPSfix = true;
      } else {
        
        gHaveGPSfix = false;
      }

      // read hardware regardless of GPS signal acquisition
      gSatellites = GPS.satellites;
      gSpeed = GPS.speed * mphPerKnots;
      gAngle = GPS.angle;
    }

};

#endif // _GRIDUINO_MODEL_CPP
