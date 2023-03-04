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
#include "constants.h"      // Griduino constants, colors, typedefs
#include "logger.h"         // conditional printing to Serial port
#include "grid_helper.h"    // lat/long conversion routines
#include "date_helper.h"    // date/time conversions
#include "save_restore.h"   // Configuration data in nonvolatile RAM

// ========== extern ===========================================
extern Adafruit_GPS GPS;       // Griduino.ino
extern Location history[];     // Griduino.ino, GPS breadcrumb trail
extern const int numHistory;   // Griduino.ino, number of elements in history[]
extern Logger logger;          // Griduino.ino
extern Grids grid;             // grid_helper.h
extern Dates date;             // date_helper.h

void floatToCharArray(char *result, int maxlen, double fValue, int decimalPlaces);   // Griduino.ino
bool isVisibleDistance(const PointGPS from, const PointGPS to);                      // view_grid.cpp

// ========== constants ========================================
// These initial values are displayed until GPS gets comes up with better info.

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
  //int gpsBattery      = 1023;    // measured coin battery ADC sample  // moved to model_adc.h

  float gSeaLevelPressure = DEFAULT_SEALEVEL_HPA;   // todo - unused by 'model_gps.h', delete me

  // Location history[1500];     // 2022-06 the GPS breadcrumb trail moved to Griduino.ino
  int nextHistoryItem = 0;   // index of next item to write

protected:
  int gPrevFix       = false;        // previous value of gHaveGPSfix, to help detect "signal lost"
  char sPrevGrid4[5] = INIT_GRID4;   // previous value of gsGridName, to help detect "enteredNewGrid()"
  char sPrevGrid6[7] = INIT_GRID6;   // previous value of gsGridName, to help detect "enteredNewGrid()"

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
  const char MODEL_VERS[15] = "GPS Data v2";                   // <-- always change version when changing model data

  const char HISTORY_FILE[25]    = CONFIG_FOLDER "/gpshistory.csv";   // CONFIG_FOLDER
  const char HISTORY_VERSION[25] = "GPS Breadcrumb Trail v1";         // <-- always change version when changing data format

  // ----- save entire C++ object to non-volatile memory as binary object -----
  int save() {   // returns 1=success, 0=failure
    SaveRestore sdram(MODEL_FILE, MODEL_VERS);
    if (sdram.writeConfig((byte *)this, sizeof(Model))) {
      // Serial.println("Success, GPS Model object stored to SDRAM");
    } else {
      Serial.println("ERROR! Failed to save GPS Model object to SDRAM");
      return 0;   // return failure
    }
    saveGPSBreadcrumbTrail();
    return 1;   // return success
  }

  // ----- save GPS history[] to non-volatile memory as CSV file -----
  int saveGPSBreadcrumbTrail() {   // returns 1=success, 0=failure
    // Internal breadcrumb trail is CSV format -- you can open this Arduino file directly in a spreadsheet
    // dumpHistoryGPS();   // debug

    // delete old file and open new file
    SaveRestoreStrings config(HISTORY_FILE, HISTORY_VERSION);
    config.open(HISTORY_FILE, "w");

    // line 1,2,3,4: filename, data format, version, compiled
    char msg[256];
    snprintf(msg, sizeof(msg), "File:,%s\nData format:,%s\nGriduino:,%s\nCompiled:,%s",
             HISTORY_FILE, HISTORY_VERSION, PROGRAM_VERSION, PROGRAM_COMPILED);
    config.writeLine(msg);

    // line 5: column headings
    config.writeLine("GMT Date, GMT Time, Grid, Latitude, Longitude");

    // line 6..x: date-time, grid6, latitude, longitude
    int count = 0;
    for (uint ii = 0; ii < numHistory; ii++) {
      if (!history[ii].isEmpty()) {
        count++;

        char sDate[12];   // sizeof("2022-11-10") = 10
        date.dateToString(sDate, sizeof(sDate), history[ii].timestamp);

        char sTime[12];   // sizeof("12:34:56") = 8
        date.timeToString(sTime, sizeof(sTime), history[ii].timestamp);

        char sGrid6[7];
        grid.calcLocator(sGrid6, history[ii].loc.lat, history[ii].loc.lng, 6);

        char sLat[12], sLng[12];
        floatToCharArray(sLat, sizeof(sLat), history[ii].loc.lat, 5);
        floatToCharArray(sLng, sizeof(sLng), history[ii].loc.lng, 5);

        snprintf(msg, sizeof(msg), "%s,%s,%s,%s,%s", sDate, sTime, sGrid6, sLat, sLng);
        config.writeLine(msg);
      }
    }
    logger.info(". Wrote %d entries to GPS log", count);

    // close file
    config.close();

    return 1;   // success
  }

  int restoreGPSBreadcrumbTrail() {   // returns 1=success, 0=failure
    clearHistory();                   // clear breadcrumb memory

    // open file
    SaveRestoreStrings config(HISTORY_FILE, HISTORY_VERSION);
    if (!config.open(HISTORY_FILE, "r")) {
      logger.error("SaveRestoreStrings::open() failed to open ", HISTORY_FILE);

      // most likely error is 'file not found' so create a new one for next time
      saveGPSBreadcrumbTrail();
      return 0;
    }

    // read file line-by-line, ignoring lines we don't understand
    // for maximum compatibility across versions, there's no "version check"
    // example of CSV line: "2022/06/16,15:44:01,48.09667,-122.85268"
    int csv_line_number = 0;
    int items_restored  = 0;
    char csv_line[256], original_line[256];
    const char delimiter[] = ",/:";
    int count;
    bool done = false;
    while (count = config.readLine(csv_line, sizeof(csv_line)) && !done) {
      // save line for possible console messages because 'strtok' will modify buffer
      strncpy(original_line, csv_line, sizeof(original_line));

      // process line according to # bytes read
      char msg[256];
      if (count == 0) {
        logger.info(". EOF");
        done = true;
        break;
      } else if (count < 0) {
        int err = config.getError();
        logger.error(". File error %d", err);   // 1=write, 2=read
        done = true;
        break;
      } else {
        // snprintf(msg, sizeof(msg), ". CSV string[%2d] = \"%s\"",
        //                               csv_line_number, csv_line); // debug
        // logger.info(msg);  // debug
        int iYear4        = atoi(strtok(csv_line, delimiter));   // YYYY: calendar year
        uint8_t iYear2    = CalendarYrToTm(iYear4);              // YY: offset from 1970
        uint8_t iMonth    = atoi(strtok(NULL, delimiter));
        uint8_t iDay      = atoi(strtok(NULL, delimiter));
        uint8_t iHour     = atoi(strtok(NULL, delimiter));
        uint8_t iMinute   = atoi(strtok(NULL, delimiter));
        uint8_t iSecond   = atoi(strtok(NULL, delimiter));
        double fLatitude  = atof(strtok(NULL, delimiter));
        double fLongitude = atof(strtok(NULL, delimiter));

        // save this return value into history[]
        // https://cplusplus.com/reference/cstring/
        if (iYear2 > 0 && fLatitude != 0.0 && fLongitude != 0.0) {
          // echo info for debug
          // char msg[256];
          // snprintf(msg, sizeof(msg), ".       Internal =  %d-%d-%d, %02d:%02d:%02d",
          //         iYear4, iMonth, iDay, iHour, iMinute, iSecond);
          // logger.info(msg);
          // Serial.print(fLatitude, 5);    // todo: replace with 'printLocation(index,Location item)'
          // Serial.print(", ");
          // Serial.println(fLongitude, 5);

          // save values in the history[] array
          TimeElements tm{iSecond, iMinute, iHour, 0, iDay, iMonth, iYear2};
          history[nextHistoryItem].timestamp = makeTime(tm);   // convert time elements into time_t
          history[nextHistoryItem].loc.lat   = fLatitude;
          history[nextHistoryItem].loc.lng   = fLongitude;

          // adjust loop variables
          nextHistoryItem++;
          items_restored++;
        } else {
          snprintf(msg, sizeof(msg), ". CSV string[%2d] = \"%s\" - ignored",
                   csv_line_number, original_line);   // debug
          logger.warning(msg);                        // debug
        }
      }
      csv_line[0] = 0;
      csv_line_number++;
      if (nextHistoryItem >= numHistory) {
        done = true;
      }
    }
    logger.info(". Restored %d breadcrumbs from %d lines in CSV file", items_restored, csv_line_number);

    // This "restore" design always fills history[] from 0..N.
    // Make sure the /next/ GPS point logged goes into the next open slot
    // and doesn't overwrite any historical data.
    int indexOldest = 0;                             // default to start
    TimeElements future{59, 59, 23, 0, 1, 1, 255};   // maximum date = year(1970 + 255) = 2,225
    time_t oldest = makeTime(future);

    int indexNewest = 0;                      // default to start
    TimeElements past{0, 0, 0, 0, 0, 0, 0};   // minimum date = Jan 1, 1970
    time_t newest = makeTime(past);

    // find the oldest item (unused slots contain zero and are automatically the oldest)
    for (int ii = 0; ii < numHistory; ii++) {
      time_t tm = history[ii].timestamp;
      if (tm < oldest) {
        // keep track of oldest GPS bread crumb
        indexOldest = ii;
        oldest      = tm;
      }
      if (tm > newest) {
        // keep track of most recent GPS bread crumb, out of curiosity
        indexNewest = ii;
        newest      = tm;
      }
    }
    // here's the real meat of the potato
    nextHistoryItem = indexOldest;

    // report statistics for a visible sanity check to aid debug
    char sOldest[24], sNewest[24];
    date.datetimeToString(sOldest, sizeof(sOldest), oldest);
    date.datetimeToString(sNewest, sizeof(sNewest), newest);

    char msg1[256], msg2[256];
    snprintf(msg1, sizeof(msg1), ". Oldest date = history[%d] = %s", indexOldest, sOldest);
    snprintf(msg2, sizeof(msg2), ". Newest date = history[%d] = %s", indexNewest, sNewest);
    logger.info(msg1);
    logger.info(msg2);

    // close file
    config.close();
    return 1;   // success
  }

  // ----- load from SDRAM -----
  int restore() {
    // restore current GPS state from non-volatile memory
    SaveRestore sdram(MODEL_FILE, MODEL_VERS);

    extern Model modelGPS;   // debug
    Model tempModel;         // temp buffer for restoring the "Model" object from RAM file system

    logger.info(". source: sizeof(tempModel) = %d", sizeof(tempModel));
    logger.info(". target: sizeof(modelGPS) = %d", sizeof(modelGPS));
    logger.info(". GPS history buffer, sizeof(history) = %d", numHistory * sizeof(Location));

    if (sdram.readConfig((byte *)&tempModel, sizeof(Model))) {
      // warning: this can corrupt our object's data if something failed
      // so we blob the bytes to a work area and copy individual values
      copyFrom(tempModel);
      logger.info("Success, GPS Model restored from SDRAM");
    } else {
      logger.error("Error, failed to restore GPS Model object to SDRAM");
      return 0;   // return failure
    }
    // note: the caller is responsible for fixups to the model,
    // e.g., indicate 'lost satellite signal'
    return 1;   // return success
  }

  // pick'n pluck values from the restored instance
  void copyFrom(const Model from) {
    gLatitude         = from.gLatitude;           // GPS position, floating point, decimal degrees
    gLongitude        = from.gLongitude;          // GPS position, floating point, decimal degrees
    gAltitude         = from.gAltitude;           // Altitude in meters above MSL
    gHaveGPSfix       = false;                    // assume no fix yet
    gSatellites       = 0;                        // assume no satellites yet
    gSpeed            = 0.0;                      // assume speed unknown
    gAngle            = 0.0;                      // assume direction of travel unknown
    gMetric           = from.gMetric;             // distance report in miles/kilometers
    gTimeZone         = from.gTimeZone;           // offset from GMT to local time
    compare4digits    = from.compare4digits;      //
    gSeaLevelPressure = from.gSeaLevelPressure;   // hPa
    //gpsBattery        = from.gpsBattery;          // measured coin battery ADC sample
  }

  // sanity check data from NVR
  void printLocation(int ii, Location item) {
    Serial.print(". lat/long[");
    Serial.print(ii);
    Serial.print("] = ");
    Serial.print(item.loc.lat);
    Serial.print(", ");
    Serial.println(item.loc.lng);
  }

  // given a GPS reading in NMEA format, create a "time_t" timestamp
  // written as a small independent function so it can be unit tested
  time_t NMEAtoTime_t(uint8_t nmeaYear, uint8_t nmeaMonth, uint8_t nmeaDay,
                      uint8_t nmeaHour, uint8_t nmeaMinute, uint8_t nmeaSeconds) {

    // NMEA years are the last two digits and start at year 2000, time_t starts at 1970
    const uint8_t year = nmeaYear + (2000 - 1970);
    TimeElements tm{nmeaSeconds, nmeaMinute, nmeaHour, 1, nmeaDay, nmeaMonth, year};
    return makeTime(tm);
  }

  // read GPS hardware
  virtual void getGPS() {                 // "virtual" allows derived class MockModel to replace it
    if (GPS.fix) {                        // DO NOT use "GPS.fix" anywhere else in the program,
                                          // or the simulated position in MockModel won't work correctly
      gLatitude  = GPS.latitudeDegrees;   // double-precision float
      gLongitude = GPS.longitudeDegrees;
      gAltitude  = GPS.altitude;
      // save timestamp as compact 4-byte integer (number of seconds since Jan 1 1970)
      // using https://github.com/PaulStoffregen/Time
      // NMEA sentences contain only the last two digits of year, so add the century
      gTimestamp = NMEAtoTime_t(GPS.year, GPS.month, GPS.day, GPS.hour, GPS.minute, GPS.seconds);
      gHaveGPSfix = true;
    } else {
      gHaveGPSfix = false;
    }

    // read hardware regardless of GPS signal acquisition
    gSatellites = GPS.satellites;
    gSpeed      = GPS.speed * mphPerKnots;
    gAngle      = GPS.angle;
    //gpsBattery  = analogRead(A0);   // moved to model_adc.h
  }

  // the Model will update its internal state on a schedule determined by the Controller
  void processGPS() {
    getGPS();        // read the hardware for location
    echoGPSinfo();   // send GPS statistics to serial console for debug

    PointGPS whereAmI{gLatitude, gLongitude};
    remember(whereAmI, gTimestamp);
  }

  int getHistoryCount() {
    // how many history slots currently have valid position data
    int count = 0;
    for (uint ii = 0; ii < numHistory; ii++) {
      if (!history[ii].isEmpty()) {
        count++;
      }
    }
    return count;
  }

  void clearHistory() {
    // wipe clean the array of lat/long that we remember
    for (uint ii = 0; ii < numHistory; ii++) {
      history[ii].reset();
    }
    nextHistoryItem = 0;
    Serial.println("GPS history has been erased");
  }

  void remember(PointGPS vLoc, time_t vTimestamp) {
    // save this GPS location and timestamp in internal array
    // so that we can display it as a breadcrumb trail

    int prevIndex = nextHistoryItem - 1;   // find prev location in circular buffer
    if (prevIndex < 0) {
      prevIndex = numHistory - 1;
    }
    PointGPS prevLoc = history[prevIndex].loc;
    if (isVisibleDistance(vLoc, prevLoc)) {
      history[nextHistoryItem].loc       = vLoc;
      history[nextHistoryItem].timestamp = vTimestamp;

      nextHistoryItem = (++nextHistoryItem % numHistory);

// now the GPS location is saved in history array, now protect
// the array in non-volatile memory in case of power loss
#if defined RUN_UNIT_TESTS      // todo: make this a run-time value selected by "run unittest"
      const int SAVE_INTERVAL = 9999;
#else
      const int SAVE_INTERVAL = 2;
#endif
      if (nextHistoryItem % SAVE_INTERVAL == 0) {
        save();   // filename is #define MODEL_FILE[25] above
      }
    }
  }

// ----- Beginning/end of each KML file -----
#define KML_PREFIX "\r\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<kml xmlns=\"http://www.opengis.net/kml/2.2\"\
 xmlns:gx=\"http://www.google.com/kml/ext/2.2\"\
 xmlns:kml=\"http://www.opengis.net/kml/2.2\"\
 xmlns:atom=\"http://www.w3.org/2005/Atom\">\r\n\
<Document>\r\n\
\t<name>Griduino Track</name>\r\n\
\t<Style id=\"gstyle\">\r\n\
\t\t<LineStyle>\r\n\
\t\t\t<color>ffffff00</color>\r\n\
\t\t\t<width>4</width>\r\n\
\t\t</LineStyle>\r\n\
\t</Style>\r\n\
\t<StyleMap id=\"gstyle0\">\r\n\
\t\t<Pair>\r\n\
\t\t\t<key>normal</key>\r\n\
\t\t\t<styleUrl>#gstyle1</styleUrl>\r\n\
\t\t</Pair>\r\n\
\t\t<Pair>\r\n\
\t\t\t<key>highlight</key>\r\n\
\t\t\t<styleUrl>#gstyle</styleUrl>\r\n\
\t\t</Pair>\r\n\
\t</StyleMap>\r\n\
\t<Style id=\"gstyle1\">\r\n\
\t\t<LineStyle>\r\n\
\t\t\t<color>ffffff00</color>\r\n\
\t\t\t<width>4</width>\r\n\
\t\t</LineStyle>\r\n\
\t</Style>\r\n"

#define KML_SUFFIX "\
</Document>\r\n\
</kml>\r\n"

// ----- Beginning/end of a pushpin in a KML file -----
#define PUSHPIN_PREFIX_PART1 "\
\t<Placemark>\r\n\
\t\t<name>Start "
// PUSHPIN_PREFIX_PART2 contains the date "mm/dd/yy"
#define PUSHPIN_PREFIX_PART3 "\
</name>\r\n\
\t\t<styleUrl>#m_ylw-pushpin0</styleUrl>\r\n\
\t\t<Point>\r\n\
\t\t\t<gx:drawOrder>1</gx:drawOrder>\r\n\
\t\t\t<coordinates>"

#define PUSHPIN_SUFFIX "</coordinates>\r\n\
\t\t</Point>\r\n\
\t</Placemark>\r\n"

// ----- Icon for breadcrumbs along route -----
#define BREADCRUMB_STYLE "\
\t<Style id=\"crumb\">\r\n\
\t\t<IconStyle>\r\n\
\t\t\t<Icon><href>https://www.coilgun.info/images/bullet.png</href></Icon>\r\n\
\t\t\t<hotSpot x=\".5\" y=\".5\" xunits=\"fraction\" yunits=\"fraction\"/>\r\n\
\t\t</IconStyle>\r\n\
\t</Style>\r\n"

// ----- Placemark template with timestamp -----
// From: https://developers.google.com/static/kml/documentation/TimeStamp_example.kml 
#define PLACEMARK_WITH_TIMESTAMP "\
\t<Placemark>\r\n\
\t\t<description>\
<center>\
<h2><a target=\"_blank\" href=\"https://maps.google.com/maps?q=%s,%s\">%s</a></h2>\
%04d-%02d-%02d <br/> %02d:%02d:%02d GMT\
</center>\
</description>\r\n\
\t\t<TimeStamp><when>%04d-%02d-%02dT%02d:%02d:%02dZ</when></TimeStamp>\r\n\
\t\t<Point><coordinates>%s,%s</coordinates></Point>\r\n\
\t\t<styleUrl>#crumb</styleUrl>\r\n\
\t</Placemark>\r\n"

  void dumpHistoryKML() {

    Serial.print(KML_PREFIX);               // begin KML file
    Serial.print(BREADCRUMB_STYLE);         // add breadcrumb icon style
    int startIndex  = nextHistoryItem + 1;
    bool startFound = false;

    // start right AFTER the most recently written slot in circular buffer
    int index = nextHistoryItem;

    // loop through the entire GPS history buffer
    for (int ii = 0; ii < numHistory; ii++) {
      Location item = history[index];
      if ((item.loc.lat != 0) || (item.loc.lng != 0)) {
        if (!startFound) {
          // this is the first non-empty lat/long found,
          // so this must be chronologically the oldest entry recorded
          // Remember it for the "Start" pushpin
          startIndex = index;
          startFound = true;
        }

        // PlaceMark with timestamp
        TimeElements time;                    // https://github.com/PaulStoffregen/Time
        breakTime(item.timestamp, time);

        char sLat[12], sLng[12];
        floatToCharArray(sLat, sizeof(sLat), item.loc.lat, 5);
        floatToCharArray(sLng, sizeof(sLng), item.loc.lng, 5);

        char grid6[7];
        grid.calcLocator(grid6, item.loc.lat, item.loc.lng, 6);

        char msg[500];
        snprintf(msg, sizeof(msg), PLACEMARK_WITH_TIMESTAMP,
            sLat, sLng,   // humans prefer "latitude,longitude"
            grid6,
            time.Year+1970, time.Month, time.Day, time.Hour, time.Minute, time.Second, // human readable time
            time.Year+1970, time.Month, time.Day, time.Hour, time.Minute, time.Second, // kml timestamp
            sLng, sLat    // kml requires "longitude,latitude"
            );
        Serial.print(msg);
      }
      index = (index + 1) % numHistory;
    }

    if (startFound) {                       // begin pushpin at start of route
      char pushpinDate[10];                 // strlen("12/24/21") = 9
      TimeElements time;                    // https://github.com/PaulStoffregen/Time
      breakTime(history[startIndex].timestamp, time);
      snprintf(pushpinDate, sizeof(pushpinDate), "%02d/%02d/%02d", time.Month, time.Day, time.Year);

      Serial.print(PUSHPIN_PREFIX_PART1);
      Serial.print(pushpinDate);
      Serial.print(PUSHPIN_PREFIX_PART3);
      Location start = history[startIndex];
      Serial.print(start.loc.lng, 4);   // KML demands longitude first
      Serial.print(",");
      Serial.print(start.loc.lat, 4);
      Serial.print(",0");
      Serial.print(PUSHPIN_SUFFIX);   // end pushpin at start of route
    }
    Serial.println(KML_SUFFIX);   // end KML file
  }

  int countHistorySaved() {
    int count = 0;
    for (int ii=0; ii<numHistory; ii++) {
      Location item = history[ii];
      if (!item.isEmpty()) {
        count++;
      }
    }
    return count;
  }

  void dumpHistoryGPS() {
    Serial.print("\nMaximum saved GPS records = ");
    Serial.println(numHistory);

    Serial.print("Current number of records saved = ");
    int count = countHistorySaved();
    Serial.println(count);

    Serial.print("Next record to be written = ");
    Serial.println(nextHistoryItem);

    time_t tm = now();                           // debug: show current time in seconds
    Serial.print("now() = ");                    // debug
    Serial.print(tm);                            // debug
    Serial.println(" seconds since 1-1-1970");   // debug

    char sDate[24];                                        // debug: show current time decoded
    date.datetimeToString(sDate, sizeof(sDate), tm);       // date_helper.h
    char msg[40];                                          // sizeof("Today is 12-31-2022  12:34:56 GMT") = 32
    snprintf(msg, sizeof(msg), "now() = %s GMT", sDate);   // debug
    Serial.println(msg);                                   // debug

    Serial.println("Record, Date GMT, Grid, Lat, Long");
    int ii;
    for (ii = 0; ii < numHistory; ii++) {
      Location item = history[ii];
      if (!item.isEmpty()) {

        time_t tm = item.timestamp;                        // https://github.com/PaulStoffregen/Time
        char sDate[24];                                    // sizeof("2022-11-25 12:34:56") = 19
        date.datetimeToString(sDate, sizeof(sDate), tm);   // date_helper.h

        char grid6[7];
        grid.calcLocator(grid6, item.loc.lat, item.loc.lng, 6);

        char sLat[12], sLng[12];
        floatToCharArray(sLat, sizeof(sLat), history[ii].loc.lat, 5);
        floatToCharArray(sLng, sizeof(sLng), history[ii].loc.lng, 5);

        char out[128];
        snprintf(out, sizeof(out), "%d, %s, %s, %s, %s",
                 ii, sDate, grid6, sLat, sLng);
        Serial.println(out);

        //TimeElements time;                 // https://github.com/PaulStoffregen/Time
        //breakTime(item.timestamp, time);   // debug
        //snprintf(out, sizeof(out), "item.timestamp = %02d-%02d-%04d %02d:%02d:%02d",
        //         time.Month, time.Day, 1970+time.Year, time.Hour, time.Minute, time.Second);
        //Serial.println(out);               // debug
      }
    }
    int remaining = numHistory - ii;
    if (remaining > 0) {
      Serial.print("... and ");
      Serial.print(remaining);
      Serial.println(" more");
    }
  }
  // grid-crossing detector
  bool enteredNewGrid() {
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
    } else if (!gPrevFix && gHaveGPSfix) {
      logger.warning("Acquired GPS position lock");
    }
    gPrevFix = gHaveGPSfix;
    return lostFix;
  }

  void indicateSignalLost() {
    // we want SOME indication to not trust the readings
    // but make it low-key to not distract the driver
    // todo - architecturally, it seems like this subroutine should be part of the view (not model)
    // strncpy(gsLatitude, sizeof(gsLatitude), INIT_LAT);    // GPS position, string
    // strncpy(gsLongitude, sizeof(gsLongitude), INIT_LONG);
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
  void getDateTime(char *result) {
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
  //#define TIME_FOLDER  "/GMTclock"     // 8.3 names
  //#define TIME_FILE    TIME_FOLDER "/AddHours.cfg"
  //#define TIME_VERSION "v01"

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
    getDateTime(sDate);
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
    gAltitude  = GPS.altitude;

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
