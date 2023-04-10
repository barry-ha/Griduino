#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     model_breadcrumbs.h

            Contains everything about the breadcrumb history trail.
            This is a circular buffer in memory, which periodically written to a CSV file.

  Todo:     1. refactor "class Location" into this file
            2. refactor "makeLocation()" into ctor (or public member) of "class Location"
            3. replace remember(Location) with remember(a,b,c,d,e,f)
            4. add rememberPDN(), rememberTOD()

  API Design:
      Init:   We need to initialize the circular buffer container with a buffer and size
              We need to reset the circular buffer container
      Add/remove data:
              We need to be able to add data to the buffer
              We need to be able to read values by iterating through the buffer
              We don't need to remove elements from the buffer
              We don't need thread safety
      State tracking:
              We need to know whether the buffer is full or empty
              We need to know the current number of elements in the buffer
              We need to know the max capacity of the buffer
      I/O:    We need to send a neatly-formatted report to the console
              We need to save and restore the buffer to/from a file
              We want to hide implementation details from other modules

  Size of GPS breadcrumb trail:
              Our goal is to keep track of at least one long day's travel, 500 miles or more.
              If 180 pixels horiz = 100 miles, then we need (500*180/100) = 900 entries.
              If 160 pixels vert = 70 miles, then we need (500*160/70) = 1,140 entries.
              For example, with a drunken-sailor route around the Olympic Peninsula,
              we need at least 800 entries to capture the whole out-and-back 500-mile loop.

              array  bytes        total          2023-04-08
              size   per entry    memory         comment
              2500     48 bytes  120,000 bytes   ok
              2950     48 bytes  141,600 bytes   ok
              2980     48 bytes  143,040 bytes   ok
              3000     48 bytes  144,000 bytes   crash on "list files" command

  Note:       This is a low-level subroutine with minimal side effects.
              The caller is responsible for asking us to write to a file when it's needed.

  Inspiration:
            https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

*/

#include "logger.h"         // conditional printing to Serial port
#include "grid_helper.h"    // lat/long conversion routines
#include "date_helper.h"    // date/time conversions
#include "save_restore.h"   // Configuration data in nonvolatile RAM

// ========== extern ===========================================
extern Logger logger;   // Griduino.ino
extern Grids grid;      // grid_helper.h
extern Dates date;      // date_helper.h

void floatToCharArray(char *result, int maxlen, double fValue, int decimalPlaces);   // Griduino.ino
bool isVisibleDistance(const PointGPS from, const PointGPS to);                      // view_grid.cpp

// ========== class History ======================
class Breadcrumbs {
public:
  // Class member variables
  int nextHistoryItem = 0;   // index of next item to write
  Location history[2500];    // remember a list of GPS coordinates and stuff
  const int numHistory = sizeof(history) / sizeof(Location);
  int saveInterval     = 2;
  PointGPS noLocation{-1.0, -1.0};   // eye-catching value, and nonzero for "isEmpty()"
  const float noSpeed     = -1.0;
  const float noDirection = -1.0;
  const float noAltitude  = -1.0;

public:
  // Constructor - create and initialize member variables
  Breadcrumbs() {}

  void rememberPUP() {
    // Save a "power-up" event in the history array.
    Location pup{rPOWERUP, noLocation, now(), 0, noSpeed, noDirection, noAltitude};

    history[nextHistoryItem] = pup;
    nextHistoryItem          = (++nextHistoryItem % numHistory);
  }

  void rememberFirstValidTime(time_t vTime, uint8_t vSats) {
    // "first valid time" can happen _without_ a satellite fix,
    // so the only data stored is the GMT timestamp
    Location fvt{rVALIDTIME, noLocation, vTime, vSats, noSpeed, noDirection, noAltitude};

    history[nextHistoryItem] = fvt;
    nextHistoryItem          = (++nextHistoryItem % numHistory);
  }

  void remember(Location vLoc) {
    // save this GPS location and timestamp in internal array
    // so that we can display it as a breadcrumb trail
    int prevIndex = nextHistoryItem - 1;   // find prev location in circular buffer
    if (prevIndex < 0) {
      prevIndex = numHistory - 1;
    }
    PointGPS prevLoc = history[prevIndex].loc;
    if (isVisibleDistance(vLoc.loc, prevLoc)) {   // todo: refactor into Controller
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
  // our breadcrumb trail file is CSV format -- you can open this Arduino file directly in a spreadsheet
  int saveGPSBreadcrumbTrail();   // returns 1=success, 0=failure

  bool isValidBreadcrumb(const char *crumb) {
    // examine an input line from saved breadcrumb trail to see if it's a plausible GPS record type
    // clang-format off
    if ((strlen(crumb) > 4)
      &&(',' == crumb[3])
      &&('A' <= crumb[0] && crumb[0] <= 'Z')
      &&('A' <= crumb[1] && crumb[1] <= 'Z')
      &&('A' <= crumb[2] && crumb[2] <= 'Z')) {
      return true;
    } else {
      return false;
    }
    // clang-format on
  }

  int restoreGPSBreadcrumbTrail();   // returns 1=success, 0=failure

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
    // wipe clean the trail of breadcrumbs
    // Note: This is a low-level subroutine with minimal side effects.
    //       The caller is responsible for writing it to a file, if that's needed.
    for (uint ii = 0; ii < numHistory; ii++) {
      history[ii].reset();
    }
    nextHistoryItem = 0;
    Serial.println("Breadcrumb trail has been erased");
  }

  void deleteFile();   // model_breadcrumbs.cpp

  void dumpHistoryGPS(int limit = 0) {
    // limit = for unit tests, how many entries to dump from 0..limit
    Serial.print("\nMaximum saved records = ");
    Serial.println(numHistory);

    Serial.print("Current number of records saved = ");
    int count = getHistoryCount();
    Serial.println(count);

    if (limit) {
      logger.info("Limited to first %d records", limit);
    } else {
      limit = numHistory;   // default to all records
    }

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

    Serial.println("Record, Type, Date GMT, Time GMT, Grid, Lat, Long, Alt(m), Speed(mph), Direction(Degrees), Sats");
    int ii;
    for (ii = 0; ii < limit; ii++) {
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
        if (item.isPUP()) {
          snprintf(out, sizeof(out), "%d, %s, %s, %s",
                   ii, item.recordType, sDate, sTime);

        } else if (item.isFirstValidTime()) {
          snprintf(out, sizeof(out), "%d, %s, %s, %s, , , , , , , %d",
                   ii, item.recordType, sDate, sTime, nSats);

        } else if (item.isGPS()) {
          //                           1   2   3   4   5   6   7   8   9  10
          snprintf(out, sizeof(out), "%d, %s, %s, %s, %s, %s, %s, %s, %s, %s, %d",
                   ii, item.recordType, sDate, sTime, grid6, sLat, sLng, sSpeed, sDirection, sAltitude, nSats);

        } else {
          snprintf(out, sizeof(out), "--> Type '%s' unknown: ", item.recordType);
          Serial.print(out);
          //                           1   2   3   4   5   6   7   8   9  10
          snprintf(out, sizeof(out), "%d, %s, %s, %s, %s, %s, %s, %s, %s, %s, %d",
                   ii, item.recordType, sDate, sTime, grid6, sLat, sLng, sSpeed, sDirection, sAltitude, nSats);
        }
        Serial.println(out);
      }
    }
    int remaining = numHistory - ii;
    if (remaining > 0) {
      Serial.print("... and ");
      Serial.print(remaining);
      Serial.println(" more");
    }
  }

  void dumpHistoryKML();
};   // end class Breadcrumbs
