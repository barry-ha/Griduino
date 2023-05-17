#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     model_gps.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

   Note: All data must be self-contained so it can be save/restored in
         non-volatile memory; do not use String class because it's on the heap.

  Units of Time:
         This relies on "TimeLib.h" which uses "time_t" to represent time.
         The basic unit of time (time_t) is the number of seconds since Jan 1, 1970,
         a compact 4-byte integer.
         https://github.com/PaulStoffregen/Time
*/

#include <Arduino.h>             //
#include <Adafruit_GPS.h>        // "Ultimate GPS" library
#include "constants.h"           // Griduino constants, colors, typedefs
#include "hardware.h"            //
#include "logger.h"              // conditional printing to Serial port
#include "model_breadcrumbs.h"   // breadcrumb trail
#include "grid_helper.h"         // lat/long conversion routines
#include "date_helper.h"         // date/time conversions
#include "save_restore.h"        // Configuration data in nonvolatile RAM

// ========== extern ===========================================
extern Adafruit_GPS GPS;    // Griduino.ino
extern Breadcrumbs trail;   // model of breadcrumb trail
extern Logger logger;       // Griduino.ino
extern Grids grid;          // grid_helper.h
extern Dates date;          // date_helper.h

// ========== constants ========================================
// These initial values are displayed until GPS gets provides better info

#define INIT_GRID6 "CN77tt";   // initialize to a nearby grid for demo
#define INIT_GRID4 "CN77";     // but not my home grid CN87, so that GPS lock will announce where we are in Morse code

// ========== class Model ======================
class Model {
public:
  // Class member variables
  double gLatitude    = 0;       // GPS position, floating point, decimal degrees
  double gLongitude   = 0;       // GPS position, floating point, decimal degrees
  float gAltitude     = 0;       // Altitude in meters above MSL
  time_t gTimestamp   = 0;       // date/time of GPS reading
  bool gHaveGPSfix    = false;   // true = GPS.fix() = whether or not gLatitude/gLongitude is valid
  uint8_t gSatellites = 0;       // number of satellites in use
  float gSpeed        = 0.0;     // current speed over ground in MPH
  float gAngle        = 0.0;     // direction of travel, degrees from true north
  bool gMetric        = false;   // distance reported in miles(false), kilometers(true)
  int gTimeZone       = -7;      // default local time Pacific (-7 hours)
  bool compare4digits = true;    // true=4 digit, false=6 digit comparisons

protected:
  int gPrevFix       = false;        // previous value of gPrevFix, to help detect "signal lost"
  char sPrevGrid4[5] = INIT_GRID4;   // previous value of gsGridName, to help detect "enteredNewGrid4()"
  char sPrevGrid6[7] = INIT_GRID6;   // previous value of gsGridName, to help detect "enteredNewGrid6()"

public:
  // Constructor - create and initialize member variables
  Model() {}

  // Setters
  void setEnglish() {
    gMetric = false;   // caller is responsible to save GPS model to NVR
                       // on a schedule convenient to their UI timing, typ. "endView()"
                       // because saving entire model is slow, ~1 second
  }
  void setMetric() {
    gMetric = true;   // caller is responsible to save GPS model to NVR
                      // on a schedule convenient to their UI timing, typ. "endView()"
                      // because saving entire model is slow, ~1 second
  }

  // ========== load/save config setting =========================
  const char MODEL_FILE[25] = CONFIG_FOLDER "/gpsmodel.cfg";   // CONFIG_FOLDER
  const char MODEL_VERS[15] = "GPS Data v3";                   // <-- always change version when changing model data

  // ----- save entire C++ object to non-volatile memory as binary object -----
  int save() {   // returns 1=success, 0=failure
    SaveRestore sdram(MODEL_FILE, MODEL_VERS);
    if (sdram.writeConfig((byte *)this, sizeof(Model))) {
      // Serial.println("Success, GPS Model object stored to SDRAM");
    } else {
      Serial.println("ERROR! Failed to save GPS Model object to SDRAM");
      return 0;   // return failure
    }
    trail.saveGPSBreadcrumbTrail();
    return 1;   // return success
  }

  // ----- load from SDRAM -----
  int restore() {
    // restore current GPS state from non-volatile memory
    SaveRestore sdram(MODEL_FILE, MODEL_VERS);
    int rc = 1;        // assume success
    Model tempModel;   // temp buffer for restoring the "Model" object from RAM file system

    extern Model modelGPS;                                                // debug
    logger.info(". source: sizeof(tempModel) = %d", sizeof(tempModel));   // debug
    logger.info(". target: sizeof(modelGPS) = %d", sizeof(modelGPS));     // debug

    if (sdram.readConfig((byte *)&tempModel, sizeof(Model))) {
      // warning: this can corrupt our object's data if something failed
      // so we blob the bytes to a work area and copy individual values
      copyFrom(tempModel);
      logger.info("Success, GPS Model restored from SDRAM");
    } else {
      logger.error("Error, failed to restore GPS Model object to SDRAM");
      rc = 0;   // return failure
    }
    // note: the caller is responsible for fixups to the model,
    // e.g., indicate 'lost satellite signal'
    return rc;   // return success
  }

  // pick'n pluck values from the restored instance
  void copyFrom(const Model from) {
    gLatitude      = from.gLatitude;        // GPS position, floating point, decimal degrees
    gLongitude     = from.gLongitude;       // GPS position, floating point, decimal degrees
    gAltitude      = from.gAltitude;        // Altitude in meters above MSL
    gTimestamp     = from.gTimestamp;       // date/time of GPS reading
    gHaveGPSfix    = false;                 // assume no fix yet
    gSatellites    = 0;                     // assume no satellites yet
    gSpeed         = 0.0;                   // assume speed unknown
    gAngle         = 0.0;                   // assume direction of travel unknown
    gMetric        = from.gMetric;          // distance report in miles/kilometers
    gTimeZone      = from.gTimeZone;        // offset from GMT to local time
    compare4digits = from.compare4digits;   // true=4 digit, false=6 digit comparisons
  }

  // given a GPS reading in NMEA format, create a "time_t" timestamp
  // written as a small independent function so it can be unit tested
  time_t NMEAtoTime_t(uint8_t nmeaYear, uint8_t nmeaMonth, uint8_t nmeaDay,
                      uint8_t nmeaHour, uint8_t nmeaMinute, uint8_t nmeaSeconds) {

    // NMEA years are the last two digits and start at year 2000, time_t starts at 1970
    const uint8_t year = nmeaYear + (2000 - 1970);
    TimeElements tm{nmeaSeconds, nmeaMinute, nmeaHour, 1, nmeaDay, nmeaMonth, year};
    time_t ret = makeTime(tm);
    return ret;
  }

  // read GPS hardware
  virtual void getGPS() {                  // "virtual" allows derived class MockModel to replace it
    if (GPS.fix) {                         // DO NOT use "GPS.fix" anywhere else in the program,
                                           // or the simulated position in MockModel won't work correctly
      gLatitude  = GPS.latitudeDegrees;    // double-precision float
      gLongitude = GPS.longitudeDegrees;   //
      gAltitude  = GPS.altitude;           // Altitude in meters above MSL
      // save timestamp as compact 4-byte integer (number of seconds since Jan 1 1970)
      // using https://github.com/PaulStoffregen/Time
      // NMEA sentences contain only the last two digits of year, so add the century
      gTimestamp  = NMEAtoTime_t(GPS.year, GPS.month, GPS.day, GPS.hour, GPS.minute, GPS.seconds);
      gHaveGPSfix = true;
    } else {
      gHaveGPSfix = false;
    }

    // read hardware regardless of GPS signal acquisition
    gSatellites = GPS.satellites;
    gSpeed      = GPS.speed * mphPerKnots;
    gAngle      = GPS.angle;
  }

  // the Model will update its internal state on a schedule determined by the Controller
  void processGPS() {
    getGPS();        // read the hardware for location
    echoGPSinfo();   // send GPS statistics to serial console for debug
  }

  void makeLocation(Location *vLoc) {
    // collect GPS information from the Model into an object that can be saved in the breadcrumb trail
    strncpy(vLoc->recordType, rGPS, sizeof(vLoc->recordType));

    PointGPS whereAmI{gLatitude, gLongitude};
    vLoc->loc = whereAmI;

    vLoc->timestamp     = gTimestamp;
    vLoc->numSatellites = gSatellites;
    vLoc->speed         = gSpeed;
    vLoc->direction     = gAngle;
    vLoc->altitude      = gAltitude;
    return;
  }

  // 4-digit grid-crossing detector
  bool enteredNewGrid4() {
    // returns TRUE if the first FOUR characters of grid name have changed
    char newGrid4[5];   // strlen("CN87") = 4
    grid.calcLocator(newGrid4, gLatitude, gLongitude, 4);
    if (strcmp(newGrid4, sPrevGrid4) != 0) {
      char msg[128];
      snprintf(msg, sizeof(msg), "Prev grid: %s New grid: %s", sPrevGrid4, sPrevGrid4);
      logger.warning(msg);
      strncpy(sPrevGrid4, newGrid4, sizeof(sPrevGrid4));
      return true;
    } else {
      return false;
    }
  }

  // 6-digit grid-crossing detector
  bool enteredNewGrid6() {
    // returns TRUE if the first SIX characters of grid name have changed
    char newGrid6[7];   // strlen("CN87us") = 6
    grid.calcLocator(newGrid6, gLatitude, gLongitude, 6);
    if (strcmp(newGrid6, sPrevGrid6) != 0) {
      Serial.print("Prev grid: ");
      Serial.print(sPrevGrid6);
      Serial.print(" New grid: ");
      Serial.println(newGrid6);
      strncpy(sPrevGrid6, newGrid6, sizeof(sPrevGrid6));
      return true;
    } else {
      return false;
    }
  }

  bool enteredNewGrid_delete_me() {
    if (compare4digits) {
      // returns TRUE if the first FOUR characters of grid name have changed
      char newGrid4[5];   // strlen("CN87") = 4
      grid.calcLocator(newGrid4, gLatitude, gLongitude, 4);
      if (strcmp(newGrid4, sPrevGrid4) != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Prev grid: %s New grid: %s", sPrevGrid4, sPrevGrid4);
        logger.warning(msg);
        strncpy(sPrevGrid4, newGrid4, sizeof(sPrevGrid4));
        return true;
      } else {
        return false;
      }
    } else {
      // returns TRUE if the first SIX characters of grid name have changed
      char newGrid6[7];   // strlen("CN87us") = 6
      grid.calcLocator(newGrid6, gLatitude, gLongitude, 6);
      if (strcmp(newGrid6, sPrevGrid6) != 0) {
        Serial.print("Prev grid: ");
        Serial.print(sPrevGrid6);
        Serial.print(" New grid: ");
        Serial.println(newGrid6);
        strncpy(sPrevGrid6, newGrid6, sizeof(sPrevGrid6));
        return true;
      } else {
        return false;
      }
    }
  }

  bool signalLost() {
    // GPS "lost satellite lock" detector
    // returns TRUE only once on the transition from GPS lock to no-lock
    bool lostFix = false;   // assume no transition
    if (gPrevFix && !gHaveGPSfix) {
      lostFix     = true;
      gHaveGPSfix = false;
      logger.warning("Lost GPS positioning");
      Location loc;
      makeLocation(&loc);
      trail.rememberLOS(loc);
    } else if (!gPrevFix && gHaveGPSfix) {
      logger.warning("Acquired GPS position lock");
      Location loc;
      makeLocation(&loc);
      trail.rememberAOS(loc);
    }
    gPrevFix = gHaveGPSfix;
    return lostFix;
  }

  //=========== time helpers =================================
  // Formatted GMT time
  void getTime(char *result) {
    // result = char[10] = string buffer to modify
    int hh = GPS.hour;
    int mm = GPS.minute;
    int ss = GPS.seconds;
    snprintf(result, 10, "%02d:%02d:%02d",
             hh, mm, ss);
  }

  // Formatted GMT date "Jan 12, 2020"
  void getDate(char *result, int maxlen) {
    // @param result = char[15] = string buffer to modify
    // @param maxlen = string buffer length
    // Note that GPS can have a valid date without it knowing its lat/long, and so
    // we can't rely on gHaveGPSfix to know if the date is correct or not,
    // so we deduce whether we have valid date from the y/m/d values.
    char sDay[3];    // "12"
    char sYear[5];   // "2020"
    char aMonth[][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "err"};

    uint8_t yy = GPS.year;
    int mm     = GPS.month;
    int dd     = GPS.day;

    if (date.isDateValid(yy, mm, dd)) {
      int year  = yy + 2000;   // convert two-digit year into four-digit integer
      int month = mm - 1;      // GPS month is 1-based, our array is 0-based
      snprintf(result, maxlen, "%s %d, %4d", aMonth[month], dd, year);
    } else {
      // GPS does not have a valid date, we will display it as "0000-00-00"
      snprintf(result, maxlen, "0000-00-00");
    }
  }

  // Provide formatted GMT date/time "2019-12-31  10:11:12"
  void getCurrentDateTime(char *result) {
    // input: result = char[25] = string buffer to modify
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
             yy, mo, dd, hh, mm, ss);
  }

  //=========== local time helpers ===============================
  // #define TIME_FOLDER  "/GMTclock"     // 8.3 names
  // #define TIME_FILE    TIME_FOLDER "/AddHours.cfg"
  // #define TIME_VERSION "v01"

  // Formatted Local time
  void getTimeLocal(char *result, int len) {
    // @param result = char[10] = string buffer to modify
    // @param len = string buffer length
    int hh = GPS.hour + gTimeZone;   // 24-hour clock (format matches GMT time)
    hh     = (hh + 24) % 24;         // ensure positive number of hours
    int mm = GPS.minute;
    int ss = GPS.seconds;
    snprintf(result, len, "%02d:%02d:%02d",
             hh, mm, ss);
  }

  void timeZonePlus() {
    gTimeZone++;
    if (gTimeZone > 12) {
      gTimeZone = -11;
    }
    Serial.print("Time zone changed to ");
    Serial.println(gTimeZone);
    // don't save to NVR here, "save()" is slow, to the caller
    // should call model->save() when it's able to spend the time
    // this->save();   // save the new timezone (and model) in non-volatile memory
  }
  void timeZoneMinus() {
    gTimeZone--;
    if (gTimeZone < -12) {
      gTimeZone = 11;
    }
    Serial.print("Time zone changed to ");
    Serial.println(gTimeZone);
    // don't save to NVR here, "save()" is slow, to the caller
    // should call model->save() when it's able to spend the time
    // this->save();   // save the new timezone (and model) in non-volatile memory
  }

private:
  void echoGPSinfo() {
#ifdef ECHO_GPS
    // send GPS statistics to serial console for desktop debugging
    char sDate[20];   // strlen("0000-00-00 hh:mm:ss") = 19
    getCurrentDateTime(sDate);
    Serial.print("Model: ");
    Serial.print(sDate);
    Serial.print("  Fix(");
    Serial.print((int)GPS.fix);
    Serial.println(")");

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
// about the GPS location for the sake of testing.

class MockModel : public Model {
  // Generate a simulated travel path for demonstrations and testing
  // Note: GPS Position: Simulated
  //       Realtime Clock: Actual time as given by hardware

protected:
  const double lngDegreesPerPixel = gridWidthDegrees / gBoxWidth;     // grid square = 2.0 degrees wide E-W
  const double latDegreesPerPixel = gridHeightDegrees / gBoxHeight;   // grid square = 1.0 degrees high N-S

  const PointGPS midCN87{47.50, -123.00};   // center of CN87
  const PointGPS llCN87{47.40, -123.35};    // lower left of CN87
  unsigned long startTime = 0;

public:
  // read SIMULATED GPS hardware
  void getGPS() {
    gHaveGPSfix = true;   // indicate 'fix' whether the GPS hardware sees
                          // any satellites or not, so the simulator can work
                          // indoors all the time for everybody
    // set initial conditions, just in case following code does not
    gLatitude  = llCN87.lat;
    gLongitude = llCN87.lng;
    gAltitude  = GPS.altitude;   // Altitude in meters above MSL

    // read hardware regardless of GPS signal acquisition
    gSatellites = GPS.satellites;
    gSpeed      = GPS.speed * mphPerKnots;
    gAngle      = GPS.angle;
    gTimestamp  = now();

    if (startTime == 0) {
      // one-time single initialization
      startTime = millis();
    }
    float secondHand = (millis() - startTime) / 1000.0;   // count seconds since start-up

    switch (4) {
    // Simulated GPS readings
    case 0:   // ----- move slowly NORTH forever from starting position
      gLatitude  = midCN87.lat + secondHand / gBoxHeight;
      gLongitude = midCN87.lng;
      break;

    case 1:   // ----- move slowly EAST forever from starting position
      gLatitude  = llCN87.lat;
      gLongitude = llCN87.lng + secondHand / gBoxWidth;
      break;

    case 2:   // ----- move slowly NE forever from starting position
      gLatitude  = midCN87.lat + secondHand / gBoxHeight;
      gLongitude = llCN87.lng + secondHand / gBoxWidth;
      break;

    case 3:   // ----- move slowly NORTH forever, with sine wave left-right
      gLatitude  = midCN87.lat + secondHand / gBoxHeight;
      gLongitude = midCN87.lng + 0.7 * gridWidthDegrees * sin(secondHand / 800.0 * 2.0 * PI);
      gAltitude += float(GPS.seconds) / 60.0 * 100.0;   // Simulated altitude (meters)
      break;

    case 4:   // ----- move in oval around a single grid
      // Funny note!
      //    How fast are we going???
      //    The simulated ground speeds with timeScale=1200 are:
      //    1-degree north-south is about (69.1 miles / (6m 17s) = 660 mph
      //    2-degrees east-west is about (94.3 miles) / (6m 17s) = 900 mph
      float timeScale = 1200.0;   // arbitrary divisor to slow down the motion
      gLatitude       = midCN87.lat + 0.6 * gridHeightDegrees * cos(secondHand / timeScale * 2.0 * PI);
      gLongitude      = midCN87.lng + 0.7 * gridWidthDegrees * sin(secondHand / timeScale * 2.0 * PI);
      break;
    }
  }
};
