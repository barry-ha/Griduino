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

#include <Arduino.h>        //
#include <Adafruit_GPS.h>   // "Ultimate GPS" library
#include <cmath>
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
  double gLatitude     = 0;       // GPS position, floating point, decimal degrees
  double gLongitude    = 0;       // GPS position, floating point, decimal degrees
  float gAltitude      = 0;       // Altitude in meters above MSL
  time_t gTimestamp    = 0;       // date/time of GPS reading
  bool gHaveGPSfix     = false;   // GPS.fix() = whether or not gLatitude/gLongitude is valid
  int gFixQuality      = 0;       // GPS.fixquality() 0 = no signal, 1 = only using GNSS satellites, 2=differential GPS (n/a), 3=RTK (n/a)
  uint8_t gSatellites  = 0;       // number of satellites in use
  float gSpeed         = 0.0;     // current speed over ground in MPH
  float gAngle         = 0.0;     // direction of travel, degrees from true north
  bool gMetric         = false;   // distance reported in miles(false), kilometers(true)
  int gTimeZone        = -7;      // default local time Pacific (-7 hours)
  bool compare4digits  = true;    // true=4 digit, false=6 digit comparisons
  const int dataSource = eGPSRECEIVER;

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
  void factoryReset() {
// Should never be needed, but in rare cases the GPS gets into some state
// where it goes for days/weeks without acquiring satellites.
// This will nuke it from orbit.
// It erases stored time, position, almanacs, ephemeris, clears
// system/user configurations, and resets receiver to factory status.
// Please allow 15 minutes to 2 hours to reacquire satellites.
#define PMTK_FACTORY_RESET "$PMTK104*37"   ///< Full cold start, factory reset

    logger.log(GPS_SETUP, INFO, "Full cold start:");
    logger.log(GPS_SETUP, INFO, PMTK_FACTORY_RESET);
    GPS.sendCommand(PMTK_FACTORY_RESET);

    GPS.fix        = false;   // reset the Adafruit interface object, too
    GPS.satellites = 0;       //
  }

  // ========== load/save config setting =========================
  const char MODEL_FILE[25] = CONFIG_FOLDER "/gpsmodel.cfg";   // CONFIG_FOLDER
  const char MODEL_VERS[15] = "GPS Data v3";                   // <-- always change version when changing model data

  // ----- save entire C++ object to non-volatile memory as binary object -----
  int save() {   // returns 1=success, 0=failure
    SaveRestore sdram(MODEL_FILE, MODEL_VERS);
    if (sdram.writeConfig((byte *)this, sizeof(Model))) {
      logger.log(GPS_SETUP, DEBUG, "Success, GPS Model object stored to SDRAM");
    } else {
      logger.log(GPS_SETUP, ERROR, "Failed to save GPS Model object to SDRAM");
      return 0;   // return failure
    }
    trail.saveGPSBreadcrumbTrail();   // while saving gpsmodel, also write breadcrumbs
    return 1;                         // return success
  }

  // ----- load from SDRAM -----
  int restore() {
    // restore current GPS state from non-volatile memory
    SaveRestore sdram(MODEL_FILE, MODEL_VERS);
    int rc = 1;        // assume success
    Model tempModel;   // temp buffer for restoring the "Model" object from RAM file system

    extern Model modelGPS;                                                                // debug
    logger.log(GPS_SETUP, INFO, ". source: sizeof(tempModel) = %d", sizeof(tempModel));   // debug
    logger.log(GPS_SETUP, INFO, ". target: sizeof(modelGPS) = %d", sizeof(modelGPS));     // debug

    if (sdram.readConfig((byte *)&tempModel, sizeof(Model))) {
      // warning: this can corrupt our object's data if something failed
      // so we blob the bytes to a work area and copy individual values
      copyFrom(tempModel);
      logger.log(GPS_SETUP, DEBUG, "Success, GPS Model restored from SDRAM");
    } else {
      logger.log(GPS_SETUP, ERROR, "failed to restore GPS Model object to SDRAM");
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
    gTimestamp     = from.gTimestamp;       // number of seconds since Jan 1, 1970
    gHaveGPSfix    = false;                 // assume no fix yet
    gSatellites    = 0;                     // GPS.satellites, assume no satellites
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
    time_t ret = makeTime(tm);   // number of seconds since Jan 1, 1970
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
    gFixQuality = GPS.fixquality;

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
      logger.log(GPS_SETUP, WARNING, "Prev grid: %s New grid: %s", sPrevGrid4, newGrid4);
      strncpy(sPrevGrid4, newGrid4, sizeof(sPrevGrid4));   // save for next grid transition
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
      logger.log(GPS_SETUP, WARNING, "Prev grid: %s New grid: %s", sPrevGrid6, newGrid6);
      strncpy(sPrevGrid6, newGrid6, sizeof(sPrevGrid6));   // save for next grid transition
      return true;
    } else {
      return false;
    }
  }

  bool signalLost() {
    // GPS "lost satellite lock" detector
    // returns TRUE only once on the transition from GPS lock to no-lock
    bool lostFix = false;   // assume no transition
    if (gPrevFix && !gHaveGPSfix) {
      lostFix     = true;
      gHaveGPSfix = false;
      logger.log(GPS_SETUP, WARNING, "Lost GPS positioning");
      Location loc;
      makeLocation(&loc);
      trail.rememberLOS(loc);
    } else if (!gPrevFix && gHaveGPSfix) {
      logger.log(GPS_SETUP, WARNING, "Acquired GPS position lock");
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
    // result = char[12] = string buffer to modify
    uint hh = GPS.hour;
    uint mm = GPS.minute;
    uint ss = GPS.seconds;
    snprintf(result, 12, "%02d:%02d:%02d",
             hh, mm, ss);
  }

  // Formatted GMT date "Jan 12, 2020"
  void getDate(char *result, int maxlen) {
    // @param result = char[15] = string buffer to modify
    // @param maxlen = string buffer length
    // Note that GPS can have a valid date without it knowing its lat/long, and so
    // we can't rely on gHaveGPSfix to know if the date is correct or not,
    // so we deduce whether we have valid date from the y/m/d values.
    // char sDay[3];    // "12" (unused)
    // char sYear[5];   // "2020" (unused)
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
    // input: result = char[26] = string buffer to modify
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
    snprintf(result, 26, "%04d-%02d-%02d  %02d:%02d:%02d",
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
    logger.log(GPS_SETUP, INFO, "Time zone changed to %d", gTimeZone);
    // don't save to NVR here, "save()" is slow, to the caller
    // should call model->save() when it's able to spend the time
    // this->save();   // save the new timezone (and model) in non-volatile memory
  }
  void timeZoneMinus() {
    gTimeZone--;
    if (gTimeZone < -12) {
      gTimeZone = 11;
    }
    logger.log(GPS_SETUP, INFO, "Time zone changed to %d", gTimeZone);
    // don't save to NVR here, "save()" is slow, to the caller
    // should call model->save() when it's able to spend the time
    // this->save();   // save the new timezone (and model) in non-volatile memory
  }

private:
  virtual void echoGPSinfo() {
    logger.fencepost("model_gps.h", "getGPS()", __LINE__);   // debug
    // report GPS statistics from the Adafruit_GPS library (not from our own data in our model)
    // to serial console for desktop debugging
    char sDate[26];   // strlen("0000-00-00  hh:mm:ss") = 20
    getCurrentDateTime(sDate);

    char msg[128];
    snprintf(msg, sizeof(msg), "GPS: %s  Sats(%d) Fix(%d)", sDate, GPS.satellites, (int)GPS.fix);
    logger.log(GPS_SETUP, DEBUG, msg);

    if (GPS.fix) {
      char sLat[12], sLong[12], sSpeed[12], sAngle[12], sAlt[12];
      floatToCharArray(sLat, sizeof(sLat), gLatitude, 4);
      floatToCharArray(sLong, sizeof(sLong), gLongitude, 4);
      floatToCharArray(sSpeed, sizeof(sSpeed), GPS.speed, 1);
      floatToCharArray(sAngle, sizeof(sAngle), GPS.angle, 0);
      floatToCharArray(sAlt, sizeof(sAlt), gAltitude, 1);

      snprintf(msg, sizeof(msg), "   gps(%s,%s) Quality(%d) Sats(%d) Speed(%s knots) Heading(%s) Alt(%s)",
               sLat, sLong, (int)GPS.fixquality, gSatellites, sSpeed, sAngle, sAlt);
      logger.log(GPS_SETUP, DEBUG, msg);
    }
  }
};
// ========== class MockModel ======================
// A derived class with a single replacement function to pretend
// about the GPS location for the sake of testing.

class MockModel : public Model {
  // Generate a simulated travel path for demonstrations and testing
  // Note: GPS Position: Simulated
  //       Realtime Clock: Actual time as given by hardware
public:
  const int dataSource = eGPSSIMULATOR;   //

protected:
  const double lngDegreesPerPixel = gridWidthDegrees / gBoxWidth;     // grid square = 2.0 degrees wide E-W
  const double latDegreesPerPixel = gridHeightDegrees / gBoxHeight;   // grid square = 1.0 degrees high N-S

  const PointGPS midCN87{47.50, -123.00};   // center of CN87
  const PointGPS llCN87{47.40, -123.35};    // lower left of CN87
  unsigned long startTime = 0;

  PointGPS prevGPS;   // helps compute simulated ground speed
  time_t prevTime;

public:
  // read SIMULATED GPS position
  void getGPS() override {
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
    gTimestamp  = now() / 1000.0;   // seconds

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

    case 3:   // ----- move slowly NORTH forever, with sine wave east-west
      gLatitude  = midCN87.lat + secondHand / gBoxHeight;
      gLongitude = midCN87.lng + 0.7 * gridWidthDegrees * sin(secondHand / 800.0 * 2.0 * PI);
      gAltitude += float(GPS.seconds) / 60.0 * 100.0;   // Simulated altitude (meters)
      break;

    case 4:   // ----- move in oval around a single grid
      const int elapsed   = 5;   // arbitrary time to have moved some distance
      PointGPS currentPos = simulatedPosition(secondHand);
      PointGPS prevPos    = simulatedPosition(secondHand - elapsed);

      gLatitude       = currentPos.lat;
      gLongitude      = currentPos.lng;
      double distance = grid.calcDistance(prevPos.lat, prevPos.lng, gLatitude, gLongitude, gMetric);

      // calculate speed-over-ground in mph or kph
      double speedKps = (distance / elapsed);       // speed in kps
      double speedKph = speedKps * SECS_PER_HOUR;   // speed in kph

      Serial.print("Distance/Time = ");   // debug arithmetic
      Serial.print(distance);
      Serial.print(" km / ");
      Serial.print(elapsed);
      Serial.print(" sec = ");
      Serial.print(speedKps);
      Serial.print(" kps = ");
      Serial.print(speedKph, 1);
      Serial.print(" kph = ");
      Serial.print(speedKph*milesPerKm,1);
      Serial.println(" mph");

      gSpeed = speedKph * knotsPerKph;                                              // = distance/time (knots)
      gAngle = grid.calcHeading(prevGPS.lat, prevGPS.lng, gLatitude, gLongitude);   // direction of travel, degrees from true north

      int nSpeed = (int)gSpeed;                                                       // debug
      int nAngle = (int)gAngle;                                                       // debug
      logger.log(GPS_SETUP, DEBUG, "   Sim speed(%d) heading(%d)", nSpeed, nAngle);   // debug

      // simulate number of satellites
      // should look like a slow smooth sine wave in the "Satellites" view
      float timeScale   = 500;   // arbitrary divisor for "slow Satellite Count sine wave"`
      gSatellites = round(4.0 + 3.0 * sin(secondHand / timeScale * 2.0 * PI));

      // simulate whether or not we have a valid gps position
      gHaveGPSfix = gSatellites ? true : false;
      gFixQuality = gSatellites ? 1 : 0;

      break;
    }
    prevGPS  = {gLatitude, gLongitude};   // help compute simulated ground speed
    prevTime = gTimestamp;
  }
  PointGPS simulatedPosition(double secondHand) {
    // Funny note!
    //    How fast are we moving???
    //    Perimeter of an ellipse is P = 2 pi sqrt(a^2 + b^2) / 2
    //    where a is the length of the major axis, and b is the length of the minor axis
    //
    //    The simulated ground speeds with timeScale=1200 are:
    //    1-degree north-south is about (69.1 miles / (6m 17s) = 660 mph
    //    2-degrees east-west is about (94.3 miles) / (6m 17s) = 900 mph
    //
    //  200 takes Xm 10s for one trip around the circle
    //  100 takes Xm 29s for one trip (about as fast as you want it to go)
    //   50 takes 0m 45s for one trip (each jump is very large)
    const float timeScale = 50.0;   // arbitrary divisor to slow down the motion
    PointGPS where;
    where.lat = midCN87.lat + 0.55 * gridHeightDegrees * cos(secondHand / timeScale);
    where.lng = midCN87.lng + 0.68 * gridWidthDegrees * sin(secondHand / timeScale);
    return where;
  }

  void echoGPSinfo() override {
    logger.fencepost("model_gps.h", "getGPS()", __LINE__);   // debug
    // report position from simulated GPS
    char sDate[26];   // strlen("0000-00-00  hh:mm:ss") = 20
    getCurrentDateTime(sDate);

    char msg[128];
    snprintf(msg, sizeof(msg), "Simulated: %s  Sats(%d) Fix(%d)", sDate, gSatellites, gHaveGPSfix);
    logger.log(GPS_SETUP, DEBUG, msg);

    if (GPS.fix) {
      char sLat[12], sLong[12], sSpeed[12], sAngle[12], sAlt[12];
      floatToCharArray(sLat, sizeof(sLat), gLatitude, 4);
      floatToCharArray(sLong, sizeof(sLong), gLongitude, 4);
      floatToCharArray(sSpeed, sizeof(sSpeed), gSpeed, 1);
      floatToCharArray(sAngle, sizeof(sAngle), gAngle, 0);
      floatToCharArray(sAlt, sizeof(sAlt), gAltitude, 1);

      snprintf(msg, sizeof(msg), "   Sim(%s,%s) Quality(%d) Sats(%d) Speed(%s knots) Heading(%s) Alt(%s)",
               sLat, sLong, gFixQuality, gSatellites, sSpeed, sAngle, sAlt);
      logger.log(GPS_SETUP, DEBUG, msg);
    }
  }
};
