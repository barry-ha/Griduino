// Please format this file with clang before check-in to GitHub
/*
  File:     model_breadcrumbs.cpp - file handling

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

*/

#include <Arduino.h>             // for "strncpy" and others
#include "constants.h"           // Griduino constants, colors and typedefs
#include "logger.h"              // conditional printing to Serial port
#include "grid_helper.h"         // lat/long conversion routines
#include "date_helper.h"         // date/time conversions
#include "save_restore.h"        // Configuration data in nonvolatile RAM
#include "model_breadcrumbs.h"   // breadcrumb trail

// ========== extern ===========================================
extern Logger logger;                                                                // Griduino.ino
void floatToCharArray(char *result, int maxlen, double fValue, int decimalPlaces);   // Griduino.ino

// ----- save GPS history[] to non-volatile memory as CSV file -----
const char HISTORY_FILE[25]    = CONFIG_FOLDER "/gpshistory.csv";   // CONFIG_FOLDER
const char HISTORY_VERSION[25] = "GPS Breadcrumb Trail v2";         // <-- always change version when changing data format

void Breadcrumbs::deleteFile() {
  SaveRestoreStrings config(HISTORY_FILE, HISTORY_VERSION);
  config.deleteFile(HISTORY_FILE);
}

void Breadcrumbs::dumpHistoryGPS(int limit) {
  // limit = for unit tests, how many entries to dump from 0..limit
  logger.log(FILES, CONSOLE, "\nMaximum saved records = %d", capacity);

  int count = getHistoryCount();
  logger.log(FILES, CONSOLE, "Current number of records saved = %d", count);

  if (limit) {
    logger.log(FILES, CONSOLE, "Limited to first %d records", limit);
  } else {
    limit = capacity;   // default to all records
  }

  logger.log(FILES, INFO, "Next record to be written = %d", head);

  time_t tm = now();                                     // debug: show current time in seconds
  char sDate[24];                                        // show current time decoded
  date.datetimeToString(sDate, sizeof(sDate), tm);       // date_helper.h
  char msg[40];                                          // sizeof("Today is 12-31-2022  12:34:56 GMT") = 32
  snprintf(msg, sizeof(msg), "now() = %s GMT", sDate);   //
  logger.log(FILES, INFO, msg);                          //

  logger.log(FILES, CONSOLE, "Record, Type, Date GMT, Time GMT, Grid, Lat, Long, Alt(m), Speed(mph), Direction(Degrees), Sats");
  int ii         = 0;
  Location *item = begin();
  while (item) {

    time_t tm = item->timestamp;                   // https://github.com/PaulStoffregen/Time
    char sDate[12], sTime[10];                     // sizeof("2022-11-25 12:34:56") = 19
    date.dateToString(sDate, sizeof(sDate), tm);   // date_helper.h
    date.timeToString(sTime, sizeof(sTime), tm);   //

    char grid6[7];
    grid.calcLocator(grid6, item->loc.lat, item->loc.lng, 6);

    char sLat[12], sLng[12];
    floatToCharArray(sLat, sizeof(sLat), item->loc.lat, 5);
    floatToCharArray(sLng, sizeof(sLng), item->loc.lng, 5);

    char sSpeed[12], sDirection[12], sAltitude[12];
    floatToCharArray(sSpeed, sizeof(sSpeed), item->speed, 0);
    floatToCharArray(sDirection, sizeof(sDirection), item->direction, 0);
    floatToCharArray(sAltitude, sizeof(sAltitude), item->altitude, 1);
    uint8_t nSats = item->numSatellites;

    char out[128];
    if (item->isPUP()) {
      // format for "power up" message
      snprintf(out, sizeof(out), "%d, %s, %s, %s",
               ii, item->recordType, sDate, sTime);

    } else if (item->isFirstValidTime()) {
      // format for "first valid time" message
      snprintf(out, sizeof(out), "%d, %s, %s, %s, , , , , , , %d",
               ii, item->recordType, sDate, sTime, nSats);

    } else if (item->isGPS() || item->isAcquisitionOfSignal() || item->isLossOfSignal()) {
      // format for all GPS-type messages
      //                           1   2   3   4   5   6   7   8   9  10  11
      snprintf(out, sizeof(out), "%d, %s, %s, %s, %s, %s, %s, %s, %s, %s, %d",
               ii, item->recordType, sDate, sTime, grid6, sLat, sLng, sAltitude, sSpeed, sDirection, nSats);

    } else if (item->isCoinBatteryVoltage()) {
      char sVolts[12];
      floatToCharArray(sVolts, sizeof(sVolts), item->speed, 2);
      snprintf(out, sizeof(out), "%d, %s, %s, %s, %s",
               ii, item->recordType, sDate, sTime, sVolts);

    } else {
      // format for "should not happen" messages
      snprintf(out, sizeof(out), "%d, --> Type '%s' unknown: ", ii, item->recordType);
      logger.log(FILES, ERROR, out);
      //                           1   2   3   4   5   6   7   8   9  10
      snprintf(out, sizeof(out), "%s, %s, %s, %s, %s, %s, %s, %s, %s, %d",
               item->recordType, sDate, sTime, grid6, sLat, sLng, sAltitude, sSpeed, sDirection, nSats);
    }
    logger.log(FILES, CONSOLE, out);
    ii++;
    item = next();
  }

  int remaining = getHistoryCount() - limit;
  if (remaining > 0) {
    logger.log(FILES, WARNING, "... and %d more", remaining);
  }
}

int Breadcrumbs::saveGPSBreadcrumbTrail() {   // returns 1=success, 0=failure
  // our breadcrumb trail file is CSV format -- you can open this Arduino file directly in a spreadsheet
  // dumpHistoryGPS();   // debug

  // delete old file and open new file
  SaveRestoreStrings config(HISTORY_FILE, HISTORY_VERSION);
  config.open(HISTORY_FILE, "w");

  // line 1,2,3,4: filename, data format, version, compiled
  char msg[256];
  snprintf(msg, sizeof(msg), "\nFile:,%s\nData format:,%s\nPCB:,%s\nGriduino:,%s\nCompiled:,%s",
           HISTORY_FILE, HISTORY_VERSION, HARDWARE_VERSION, PROGRAM_VERSION, PROGRAM_COMPILED);
  config.writeLine(msg);

  // line 5: column headings
  config.writeLine("Type, GMT Date, GMT Time, Grid, Latitude, Longitude, Altitude, MPH, Direction, Satellites");

  // line 6..x: date-time, grid6, latitude, longitude
  int ii              = 0;         // loop counter
  Location const *loc = begin();   // declare pointer to immutable breadcrumb
  while (loc) {

    char sDate[12];   // sizeof("2022-11-10") = 10
    date.dateToString(sDate, sizeof(sDate), loc->timestamp);

    char sTime[12];   // sizeof("12:34:56") = 8
    date.timeToString(sTime, sizeof(sTime), loc->timestamp);

    char sGrid6[7];   // sizeof("CN76us") = 6
    grid.calcLocator(sGrid6, loc->loc.lat, loc->loc.lng, 6);

    char sLat[12], sLng[12];
    floatToCharArray(sLat, sizeof(sLat), loc->loc.lat, 5);
    floatToCharArray(sLng, sizeof(sLng), loc->loc.lng, 5);

    char sAlt[12], sSpeed[12], sAngle[12], sSats[6];
    floatToCharArray(sAlt, sizeof(sAlt), loc->altitude, 1);
    floatToCharArray(sSpeed, sizeof(sSpeed), loc->speed, 1);
    floatToCharArray(sAngle, sizeof(sAngle), loc->direction, 1);
    int numSatellites = loc->numSatellites;

    // only write good valid data to file
    if (loc->isEmpty()) {
      // record type must always contain 3 characters
      snprintf(msg, sizeof(msg), "[%d] Empty record type: %s,%s,%s,%s,%s,%s,%s,%s,%s,%d",
               ii, loc->recordType, sDate, sTime, sGrid6, sLat, sLng, sAlt, sSpeed, sAngle, numSatellites);
      logger.log(CONFIG, ERROR, msg);

    } else if (!Location::isValidRecordType(loc->recordType)) {
      // record type must always be one of the allowed 3-char values
      snprintf(msg, sizeof(msg), "[%d] Invalid record type: %s,%s,%s,%s,%s,%s,%s,%s,%s,%d",
               ii, loc->recordType, sDate, sTime, sGrid6, sLat, sLng, sAlt, sSpeed, sAngle, numSatellites);
      logger.log(CONFIG, ERROR, msg);

    } else {
      // good data, write it to file
      //                           1  2  3  4  5  6  7  8  9 10
      snprintf(msg, sizeof(msg), "%s,%s,%s,%s,%s,%s,%s,%s,%s,%d",
               loc->recordType, sDate, sTime, sGrid6, sLat, sLng, sAlt, sSpeed, sAngle, numSatellites);
      config.writeLine(msg);
    }
    ii++;
    loc = next();
  }

  // close file
  config.close();

  return 1;   // success
}

int Breadcrumbs::restoreGPSBreadcrumbTrail() {   // returns 1=success, 0=failure
  clearHistory();                                // clear breadcrumb memory

  // open file
  SaveRestoreStrings config(HISTORY_FILE, HISTORY_VERSION);
  if (!config.open(HISTORY_FILE, "r")) {
    logger.log(CONFIG, ERROR, "SaveRestoreStrings::open() failed to open %s", HISTORY_FILE);

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
  const char dateDelimiters[] = ",-/:";   // include "-" for YY-MM-DD or YY/MM/DD
  const char comma[]          = ",";      // exclude "-" for -123.456
  int count;
  bool done = false;
  while (count = config.readLine(csv_line, sizeof(csv_line)) && !done) {
    // save line for possible console messages because 'strtok' will modify buffer
    strncpy(original_line, csv_line, sizeof(original_line));

    // process line according to # bytes read
    char msg[256];
    if (count == 0) {
      logger.log(FILES, INFO, ". EOF");
      done = true;
      break;
    } else if (count < 0) {
      int err = config.getError();
      logger.log(CONFIG, ERROR, ". File error %d", err);   // 1=write, 2=read
      done = true;
      break;
    } else if (isValidBreadcrumb(original_line)) {
      // parsing text must match same order in saveGPSBreadcrumbTrail()
      // "Type, GMT Date, GMT Time, Grid, Latitude, Longitude, Altitude, MPH, Direction, Satellites"
      char sType[4];
      strncpy(sType, strtok(csv_line, comma), sizeof(sType));
      int iYear4      = atoi(strtok(NULL, dateDelimiters));   // YYYY: actual calendar year
      uint8_t iYear2  = CalendarYrToTm(iYear4);               // YY: offset from 1970
      uint8_t iMonth  = atoi(strtok(NULL, dateDelimiters));
      uint8_t iDay    = atoi(strtok(NULL, dateDelimiters));
      uint8_t iHour   = atoi(strtok(NULL, dateDelimiters));
      uint8_t iMinute = atoi(strtok(NULL, dateDelimiters));
      uint8_t iSecond = atoi(strtok(NULL, dateDelimiters));
      char sGrid[7];                                        // read grid6 and discard it, we use lat,long instead
      strncpy(sGrid, strtok(NULL, comma), sizeof(sGrid));   // grid is not saved internally, we always calc it when needed
      double fLatitude    = atof(strtok(NULL, comma));
      double fLongitude   = atof(strtok(NULL, comma));
      float fAltitude     = atof(strtok(NULL, comma));   // meters
      float fSpeed        = atof(strtok(NULL, comma));   // mph
      float fDirection    = atof(strtok(NULL, comma));
      uint8_t nSatellites = atoi(strtok(NULL, comma));

      // save this value from CSV file into breadcrumb trail buffer
      PointGPS whereAmI{fLatitude, fLongitude};

      TimeElements tm{iSecond, iMinute, iHour, 0, iDay, iMonth, iYear2};
      time_t csvTime = makeTime(tm);   // convert time elements into time_t

      Location csvloc{"xxx", whereAmI, csvTime, nSatellites, fSpeed, fDirection, fAltitude};
      strncpy(csvloc.recordType, sType, sizeof(sType));

      remember(csvloc);
      items_restored++;
    } else {
      snprintf(msg, sizeof(msg), ". CSV string[%2d] = \"%s\" - ignored",
               csv_line_number, original_line);   // debug
      logger.log(CONFIG, WARNING, msg);
    }
    csv_line[0] = 0;
    csv_line_number++;
  }
  logger.log(CONFIG, INFO, ". Restored %d breadcrumbs from %d lines in CSV file", items_restored, csv_line_number);

  // The above "restore" always fills history[] from 0..N
  // Oldest allowed acceptable GPS date is Griduino's first release
  const TimeElements max_date{59, 59, 23, 0, 1, 1, 255};          // maximum date = year(1970 + 255) = 2,225
  const TimeElements min_date{0, 0, 0, 0, 1, 1, (2022 - 1970)};   // minimum date = Jan 1, 2022
  const time_t min_time_t = makeTime(min_date);

  int indexOldest = 0;   // default to start
  int indexNewest = 0;
  time_t oldest   = makeTime(max_date);
  time_t newest   = makeTime(min_date);

  // find the oldest item (unused slots contain zero and are automatically the "oldest" for comparisons)
  int ii        = 0;
  Location *loc = begin();
  while (loc) {
    time_t tm = loc->timestamp;
    if ((tm < oldest) && (tm > min_time_t)) {
      // keep track of most ancient GPS bread crumb
      indexOldest = current;
      oldest      = tm;
    }
    if (tm > newest) {
      // keep track of most modern GPS bread crumb, out of curiosity
      indexNewest = current;
      newest      = tm;
    }
    loc = next();
    ii++;
  }

  // report statistics for a visible sanity check to aid debug
  char sOldest[24], sNewest[24];   // strlen("2023-11-22 12:34:56") = 19
  date.datetimeToString(sOldest, sizeof(sOldest), oldest);
  date.datetimeToString(sNewest, sizeof(sNewest), newest);

  char msg1[256], msg2[256];
  snprintf(msg1, sizeof(msg1), ". Oldest date = history[%d] = %s", indexOldest, sOldest);
  snprintf(msg2, sizeof(msg2), ". Newest date = history[%d] = %s", indexNewest, sNewest);
  logger.log(FILES, INFO, msg1);
  logger.log(FILES, INFO, msg2);

  // close file
  config.close();
  return 1;   // success
}

// ----- Beginning/end of each KML file -----
const char *KML_PREFIX = "\r\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<kml xmlns=\"http://www.opengis.net/kml/2.2\"\
 xmlns:gx=\"http://www.google.com/kml/ext/2.2\"\
 xmlns:kml=\"http://www.opengis.net/kml/2.2\"\
 xmlns:atom=\"http://www.w3.org/2005/Atom\">\r\n\
<Document>\r\n\
\t<name>Griduino Track</name>\r\n\
\t<StyleMap id=\"crumb\">\r\n\
\t\t<Pair>\r\n\
\t\t\t<key>normal</key>\r\n\
\t\t\t<styleUrl>#snormal</styleUrl>\r\n\
\t\t</Pair>\r\n\
\t\t<Pair>\r\n\
\t\t\t<key>highlight</key>\r\n\
\t\t\t<styleUrl>#shighlight</styleUrl>\r\n\
\t\t</Pair>\r\n\
\t</StyleMap>\r\n\
\t<Style id=\"shighlight\">\r\n\
\t\t<IconStyle>\r\n\
\t\t\t<scale>1.18182</scale>\r\n\
\t\t\t<Icon>\r\n\
\t\t\t\t<href>https://www.coilgun.info/images/bullet.png</href>\r\n\
\t\t\t</Icon>\r\n\
\t\t</IconStyle>\r\n\
\t\t<BalloonStyle>\r\n\
\t\t</BalloonStyle>\r\n\
\t\t<ListStyle>\r\n\
\t\t</ListStyle>\r\n\
\t</Style>\r\n\
\t<Style id=\"snormal\">\r\n\
\t\t<IconStyle>\r\n\
\t\t\t<Icon>\r\n\
\t\t\t\t<href>https://www.coilgun.info/images/bullet.png</href>\r\n\
\t\t\t</Icon>\r\n\
\t\t</IconStyle>\r\n\
\t\t<BalloonStyle>\r\n\
\t\t</BalloonStyle>\r\n\
\t\t<ListStyle>\r\n\
\t\t</ListStyle>\r\n\
\t</Style>\r\n";

const char *KML_SUFFIX = "\
</Document>\r\n\
</kml>\r\n";

// ----- Beginning/end of a pushpin in a KML file -----
const char *PUSHPIN_PREFIX_PART1 = "\
\t<Placemark>\r\n\
\t\t<name>Start ";
// PUSHPIN_PREFIX_PART2 contains the date "mm/dd/yy"
const char *PUSHPIN_PREFIX_PART3 = "\
</name>\r\n\
\t\t<styleUrl>#m_ylw-pushpin0</styleUrl>\r\n\
\t\t<Point>\r\n\
\t\t\t<gx:drawOrder>1</gx:drawOrder>\r\n\
\t\t\t<coordinates>";

const char *PUSHPIN_SUFFIX = "</coordinates>\r\n\
\t\t</Point>\r\n\
\t</Placemark>\r\n";

// ----- Placemark template with timestamp -----
// From: https://developers.google.com/static/kml/documentation/TimeStamp_example.kml
const char *PLACE[7] = {
    "\t<Placemark>\r\n",
    "\t\t<name>%s</name>\r\n",
    "\t\t<description>%04d-%02d-%02d %02d:%02d:%02d GMT <br/> %s mph, %s°, %s', %d sat</description>\r\n",
    "\t\t<TimeStamp><when>%04d-%02d-%02dT%02d:%02d:%02dZ</when></TimeStamp>\r\n",
    "\t\t<Point><coordinates>%s,%s</coordinates></Point>\r\n",
    "\t\t<styleUrl>#crumb</styleUrl>\r\n",
    "\t</Placemark>\r\n"};

void Breadcrumbs::dumpHistoryKML() {

  logger.log(FILES, CONSOLE, KML_PREFIX);   // begin KML file
  bool startFound = false;                  // the first few items are typically "power up" events so look for first valid GPS event

  // loop through the GPS history buffer
  Location *item = begin();
  while (item) {
    // Location item = history[index];
    if (item->isGPS()) {
      if (!item->isEmpty()) {

        if (!startFound) {
          // this is first valid GPS event, so drop a KML pushpin at start of data
          // todo: look for "power up" events and drop another KML pushpin for each segment
          char pushpinDate[12];   // strlen("12/24/2023") = 10
          TimeElements time;      // https://github.com/PaulStoffregen/Time

          //         time_t time  -> tmElements_t
          // breakTime(item->timestamp, time);
          // snprintf(pushpinDate, sizeof(pushpinDate), "%02d/%02d/%02d", time.Month, time.Day, time.Year);

          time_t tm = item->timestamp;
          snprintf(pushpinDate, sizeof(pushpinDate), "%02d/%02d/%04d", month(tm), day(tm), year(tm));

          // It's too messy here to use snprintf() due to so many parts of the message
          // and some of them are floats, so we take a shortcut and directly print them.
          // All of this is required to go to the console.
          logger.print(PUSHPIN_PREFIX_PART1);
          logger.print(pushpinDate);
          logger.print(PUSHPIN_PREFIX_PART3);

          logger.print(item->loc.lng, 4);   // KML demands longitude first
          logger.print(",");
          logger.print(item->loc.lat, 4);   // then latitude
          logger.print(",0");               // then altitude
          logger.print(PUSHPIN_SUFFIX);     // end of KML pushpin

          startFound = true;
        }

        // PlaceMark with timestamp
        TimeElements time;   // https://github.com/PaulStoffregen/Time
        breakTime(item->timestamp, time);

        char sLat[12], sLng[12];
        floatToCharArray(sLat, sizeof(sLat), item->loc.lat, 5);
        floatToCharArray(sLng, sizeof(sLng), item->loc.lng, 5);

        char grid6[7];
        grid.calcLocator(grid6, item->loc.lat, item->loc.lng, 6);

        char sSpeed[12], sDirection[12], sAltitude[12];
        floatToCharArray(sSpeed, sizeof(sSpeed), item->speed, 0);
        floatToCharArray(sDirection, sizeof(sDirection), item->direction, 0);
        floatToCharArray(sAltitude, sizeof(sAltitude), item->altitude, 0);
        int numSats = item->numSatellites;

        char msg[128];
        // clang-format off
        snprintf(msg, sizeof(msg), PLACE[0]);               logger.print(msg);
        snprintf(msg, sizeof(msg), PLACE[1], grid6);        logger.print(msg);
        snprintf(msg, sizeof(msg), PLACE[2], 
            time.Year + 1970, time.Month, time.Day, 
            time.Hour, time.Minute, time.Second,
            sSpeed, sDirection, sAltitude, numSats);        logger.print(msg);
        snprintf(msg, sizeof(msg), PLACE[3], 
            time.Year + 1970, time.Month, time.Day, 
            time.Hour, time.Minute, time.Second);           logger.print(msg);
        snprintf(msg, sizeof(msg), PLACE[4], sLng, sLat);   logger.print(msg);
        snprintf(msg, sizeof(msg), PLACE[5]);               logger.print(msg);
        snprintf(msg, sizeof(msg), PLACE[6]);               logger.print(msg);
        // clang-format on
      }
    }
    item = next();
  }
  Serial.println(KML_SUFFIX);   // end KML file
}
