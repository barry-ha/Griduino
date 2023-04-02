#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     model_breadcrumbs.h

            Contains everything about the breadcrumb history trail.
            This is a history[] array which periodically does backups to a CSV file.

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

*/

// #include <Arduino.h>        //
#include "logger.h"         // conditional printing to Serial port
#include "grid_helper.h"    // lat/long conversion routines
#include "date_helper.h"    // date/time conversions
#include "save_restore.h"   // Configuration data in nonvolatile RAM

// ========== extern ===========================================
extern Logger logger;   // Griduino.ino
extern Grids grid;      // grid_helper.h
extern Dates date;      // date_helper.h
// extern Model *model;   // "model" portion of model-view-controller

void floatToCharArray(char *result, int maxlen, double fValue, int decimalPlaces);   // Griduino.ino
bool isVisibleDistance(const PointGPS from, const PointGPS to);                      // view_grid.cpp

// Size of GPS breadcrumb trail:
// Our goal is to keep track of at least one long day's travel, 500 miles or more.
// If 180 pixels horiz = 100 miles, then we need (500*180/100) = 900 entries.
// If 160 pixels vert = 70 miles, then we need (500*160/70) = 1,140 entries.
// In reality, with a drunken-sailor route around the Olympic Peninsula,
// we need at least 800 entries to capture the whole out-and-back 500-mile loop.

// ========== class History ======================
class Breadcrumbs {
public:
  // Class member variables
  int nextHistoryItem = 0;   // index of next item to write
  Location history[3000];    // remember a list of GPS coordinates and stuff
  const int numHistory = sizeof(history) / sizeof(Location);
  int saveInterval     = 2;

protected:
public:
  // Constructor - create and initialize member variables
  Breadcrumbs() {}

  void remember(Location vLoc) {
    // save this GPS location and timestamp in internal array
    // so that we can display it as a breadcrumb trail
    // optionally write the breadcrumb trail to file

    int prevIndex = nextHistoryItem - 1;   // find prev location in circular buffer
    if (prevIndex < 0) {
      prevIndex = numHistory - 1;
    }
    PointGPS prevLoc = history[prevIndex].loc;
    if (isVisibleDistance(vLoc.loc, prevLoc)) {
      history[nextHistoryItem] = vLoc;

      nextHistoryItem = (++nextHistoryItem % numHistory);

      // 2023-04-01 todo:
      //  . don't reach into the "model" from here and tell it what to do
      //  . this "breadcrumbs" object should just only manage itself
      // now the GPS location is saved in history array, now protect
      // the array in non-volatile memory in case of power loss
      // if (nextHistoryItem % saveInterval == 0) {
      //  model->save();   // filename is #define MODEL_FILE[25] above
      // }
    }
  }

  // ----- save GPS history[] to non-volatile memory as CSV file -----
  const char HISTORY_FILE[25]    = CONFIG_FOLDER "/gpshistory.csv";   // CONFIG_FOLDER
  const char HISTORY_VERSION[25] = "GPS Breadcrumb Trail v2";         // <-- always change version when changing data format

  int saveGPSBreadcrumbTrail() {   // returns 1=success, 0=failure
    // our breadcrumb trail file is CSV format -- you can open this Arduino file directly in a spreadsheet
    // trail.dumpHistoryGPS();   // debug

    // delete old file and open new file
    SaveRestoreStrings config(HISTORY_FILE, HISTORY_VERSION);
    config.open(HISTORY_FILE, "w");

    // line 1,2,3,4: filename, data format, version, compiled
    char msg[256];
    snprintf(msg, sizeof(msg), "File:,%s\nData format:,%s\nGriduino:,%s\nCompiled:,%s",
             HISTORY_FILE, HISTORY_VERSION, PROGRAM_VERSION, PROGRAM_COMPILED);
    config.writeLine(msg);

    // line 5: column headings
    config.writeLine("GMT Date, GMT Time, Grid, Latitude, Longitude, Altitude, MPH, Direction, Satellites");

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

        char sAlt[12], sSpeed[12], sAngle[12], sSats[6];
        floatToCharArray(sAlt, sizeof(sAlt), history[ii].altitude, 1);
        floatToCharArray(sSpeed, sizeof(sSpeed), history[ii].speed, 1);
        floatToCharArray(sAngle, sizeof(sAngle), history[ii].direction, 1);
        int numSatellites = history[ii].numSatellites;

        snprintf(msg, sizeof(msg), "%s,%s,%s,%s,%s,%s,%s,%s,%d", sDate, sTime, sGrid6, sLat, sLng, sAlt, sSpeed, sAngle, numSatellites);
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
        int iYear4      = atoi(strtok(csv_line, delimiter));   // YYYY: calendar year
        uint8_t iYear2  = CalendarYrToTm(iYear4);              // YY: offset from 1970
        uint8_t iMonth  = atoi(strtok(NULL, delimiter));
        uint8_t iDay    = atoi(strtok(NULL, delimiter));
        uint8_t iHour   = atoi(strtok(NULL, delimiter));
        uint8_t iMinute = atoi(strtok(NULL, delimiter));
        uint8_t iSecond = atoi(strtok(NULL, delimiter));
        // must match same order in saveGPSBreadcrumbTrail()
        // "GMT Date, GMT Time, Grid, Latitude, Longitude, Altitude, MPH, Direction, Satellites"
        double fLatitude  = atof(strtok(NULL, delimiter));
        double fLongitude = atof(strtok(NULL, delimiter));
        double fAltitude  = atof(strtok(NULL, delimiter));
        double fSpeed     = atof(strtok(NULL, delimiter));
        double fDirection = atof(strtok(NULL, delimiter));
        int fSatellites   = atoi(strtok(NULL, delimiter));

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
          history[nextHistoryItem].timestamp     = makeTime(tm);   // convert time elements into time_t
          history[nextHistoryItem].loc.lat       = fLatitude;
          history[nextHistoryItem].loc.lng       = fLongitude;
          history[nextHistoryItem].altitude      = fAltitude;
          history[nextHistoryItem].numSatellites = fSatellites;
          history[nextHistoryItem].speed         = fSpeed;
          history[nextHistoryItem].direction     = fDirection;
          history[nextHistoryItem].numSatellites = fSatellites;

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

  const int getHistoryCount() {
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
    Serial.println("Breadcrumb trail has been erased");
  }

  void dumpHistoryGPS() {
    Serial.print("\nMaximum saved GPS records = ");
    Serial.println(numHistory);

    Serial.print("Current number of records saved = ");
    int count = getHistoryCount();
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

    Serial.println("Record, Date GMT, Time GMT, Grid, Lat, Long, Alt(m), Speed(mph), Direction(Degrees), Sats");
    int ii;
    for (ii = 0; ii < numHistory; ii++) {
      Location item = history[ii];
      if (!item.isEmpty()) {

        time_t tm = item.timestamp;                    // https://github.com/PaulStoffregen/Time
        char sDate[12], sTime[10];                     // sizeof("2022-11-25 12:34:56") = 19
        date.dateToString(sDate, sizeof(sDate), tm);   // date_helper.h
        date.timeToString(sTime, sizeof(sTime), tm);   //

        char grid6[7];
        grid.calcLocator(grid6, item.loc.lat, item.loc.lng, 6);

        char sLat[12], sLng[12];
        floatToCharArray(sLat, sizeof(sLat), history[ii].loc.lat, 5);
        floatToCharArray(sLng, sizeof(sLng), history[ii].loc.lng, 5);

        char sSpeed[12], sDirection[12], sAltitude[12];
        floatToCharArray(sSpeed, sizeof(sSpeed), item.speed, 1);
        floatToCharArray(sDirection, sizeof(sDirection), item.direction, 1);
        floatToCharArray(sAltitude, sizeof(sAltitude), item.altitude, 0);
        uint8_t nSats = item.numSatellites;

        char out[128];
        snprintf(out, sizeof(out), "%d, %s, %s, %s, %s, %s, %s, %s, %s, %d",
                 ii, sDate, sTime, grid6, sLat, sLng, sSpeed, sDirection, sAltitude, nSats);
        Serial.println(out);

        // TimeElements time;                 // https://github.com/PaulStoffregen/Time
        // breakTime(item.timestamp, time);   // debug
        // snprintf(out, sizeof(out), "item.timestamp = %02d-%02d-%04d %02d:%02d:%02d",
        //          time.Month, time.Day, 1970+time.Year, time.Hour, time.Minute, time.Second);
        // Serial.println(out);               // debug
      }
    }
    int remaining = numHistory - ii;
    if (remaining > 0) {
      Serial.print("... and ");
      Serial.print(remaining);
      Serial.println(" more");
    }
  }

};   // end class History
