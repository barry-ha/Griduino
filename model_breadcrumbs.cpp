// Please format this file with clang before check-in to GitHub
/*
  File:     model_breadcrumbs.cpp - file handling

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

*/

#include <Arduino.h>             // for "strncpy" and others
#include "constants.h"           // Griduino constants and colors
#include "grid_helper.h"         // lat/long conversion routines
#include "date_helper.h"         // date/time conversions
#include "save_restore.h"        // Configuration data in nonvolatile RAM
#include "model_breadcrumbs.h"   // breadcrumb trail

void floatToCharArray(char *result, int maxlen, double fValue, int decimalPlaces);   // Griduino.ino

// ----- save GPS history[] to non-volatile memory as CSV file -----
const char HISTORY_FILE[25]    = CONFIG_FOLDER "/gpshistory.csv";   // CONFIG_FOLDER
const char HISTORY_VERSION[25] = "GPS Breadcrumb Trail v2";         // <-- always change version when changing data format

void Breadcrumbs::deleteFile() {
  SaveRestoreStrings config(HISTORY_FILE, HISTORY_VERSION);
  config.deleteFile(HISTORY_FILE);
  Serial.println("Breadcrumb trail erased and file deleted");
}

void Breadcrumbs::dumpHistoryGPS(int limit) {
  // limit = for unit tests, how many entries to dump from 0..limit
  Serial.print("\nMaximum saved records = ");
  Serial.println(capacity);

  Serial.print("Current number of records saved = ");
  int count = getHistoryCount();
  Serial.println(count);

  if (limit) {
    logger.info("Limited to first %d records", limit);
  } else {
    limit = capacity;   // default to all records
  }

  logger.info("Next record to be written = %d", head);

  // time_t tm = now();                           // debug: show current time in seconds
  // Serial.print("now() = ");                    // debug
  // Serial.print(tm);                            // debug
  // Serial.println(" seconds since 1-1-1970");   // debug

  // char sDate[24];                                        // show current time decoded
  // date.datetimeToString(sDate, sizeof(sDate), tm);       // date_helper.h
  // char msg[40];                                          // sizeof("Today is 12-31-2022  12:34:56 GMT") = 32
  // snprintf(msg, sizeof(msg), "now() = %s GMT", sDate);   //
  // logger.info(msg);                                      //

  Serial.println("Record, Type, Date GMT, Time GMT, Grid, Lat, Long, Alt(m), Speed(mph), Direction(Degrees), Sats");
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
    floatToCharArray(sSpeed, sizeof(sSpeed), item->speed, 1);
    floatToCharArray(sDirection, sizeof(sDirection), item->direction, 1);
    floatToCharArray(sAltitude, sizeof(sAltitude), item->altitude, 0);
    uint8_t nSats = item->numSatellites;

    char out[128];
    if (item->isPUP()) {
      snprintf(out, sizeof(out), "%d, %s, %s, %s",
               ii, item->recordType, sDate, sTime);

    } else if (item->isFirstValidTime()) {
      snprintf(out, sizeof(out), "%d, %s, %s, %s, , , , , , , %d",
               ii, item->recordType, sDate, sTime, nSats);

    } else if (item->isGPS() || item->isAcquisitionOfSignal() || item->isLossOfSignal()) {
      //                           1   2   3   4   5   6   7   8   9  10  11
      snprintf(out, sizeof(out), "%d, %s, %s, %s, %s, %s, %s, %s, %s, %s, %d",
               ii, item->recordType, sDate, sTime, grid6, sLat, sLng, sSpeed, sDirection, sAltitude, nSats);

    } else {
      snprintf(out, sizeof(out), "%d, --> Type '%s' unknown: ", ii, item->recordType);
      Serial.print(out);
      //                           1   2   3   4   5   6   7   8   9  10
      snprintf(out, sizeof(out), "%s, %s, %s, %s, %s, %s, %s, %s, %s, %d",
               item->recordType, sDate, sTime, grid6, sLat, sLng, sSpeed, sDirection, sAltitude, nSats);
    }
    Serial.println(out);
    ii++;
    item = next();
  }

  int remaining = getHistoryCount() - limit;
  if (remaining > 0) {
    logger.info("... and %d more", remaining);
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
  snprintf(msg, sizeof(msg), "\nFile:,%s\nData format:,%s\nGriduino:,%s\nCompiled:,%s",
           HISTORY_FILE, HISTORY_VERSION, PROGRAM_VERSION, PROGRAM_COMPILED);
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
      logger.error(msg);

    } else if (!Location::isValidRecordType(loc->recordType)) {
      // record type must always be one of the allowed 3-char values
      snprintf(msg, sizeof(msg), "[%d] Invalid record type: %s,%s,%s,%s,%s,%s,%s,%s,%s,%d",
               ii, loc->recordType, sDate, sTime, sGrid6, sLat, sLng, sAlt, sSpeed, sAngle, numSatellites);
      logger.error(msg);

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
      logger.info(". EOF");
      done = true;
      break;
    } else if (count < 0) {
      int err = config.getError();
      logger.error(". File error %d", err);   // 1=write, 2=read
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
      float fAltitude     = atof(strtok(NULL, comma));
      float fSpeed        = atof(strtok(NULL, comma));
      float fDirection    = atof(strtok(NULL, comma));
      uint8_t nSatellites = atoi(strtok(NULL, comma));

      // save this value from CSV file into breadcrumb trail buffer
      PointGPS whereAmI{fLatitude, fLongitude};

      TimeElements tm{iSecond, iMinute, iHour, 0, iDay, iMonth, iYear2};
      time_t csvTime = makeTime(tm);   // convert time elements into time_t

      Location csvloc{"xxx", whereAmI, csvTime, nSatellites, fSpeed, fDirection, fAltitude};
      strncpy(csvloc.recordType, sType, sizeof(sType));

      // echo info for debug
      //snprintf(msg, sizeof(msg), ". CSV string[%2d] = \"%s\"", csv_line_number, original_line);   // debug
      //Serial.println(msg);                                                                        // debug
      //csvloc.printLocation(csv_line_number);                                                      // debug

      remember(csvloc);
      items_restored++;
    } else {
      snprintf(msg, sizeof(msg), ". CSV string[%2d] = \"%s\" - ignored",
               csv_line_number, original_line);   // debug
      logger.warning(msg);                        // debug
    }
    csv_line[0] = 0;
    csv_line_number++;
  }
  logger.info(". Restored %d breadcrumbs from %d lines in CSV file", items_restored, csv_line_number);

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
  char sOldest[24], sNewest[24];   // strln("2023-11-22 12:34:56") = 19
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

// ----- Beginning/end of each KML file -----
const char *KML_PREFIX = "\r\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<kml xmlns=\"http://www.opengis.net/kml/2.2\"\
 xmlns:gx=\"http://www.google.com/kml/ext/2.2\"\
 xmlns:kml=\"http://www.opengis.net/kml/2.2\"\
 xmlns:atom=\"http://www.w3.org/2005/Atom\">\r\n\
<Document>\r\n\
  <name>Griduino Track</name>\r\n\
  <Style id=\"gstyle\">\r\n\
    <LineStyle>\r\n\
      <color>ffffff00</color>\r\n\
      <width>4</width>\r\n\
    </LineStyle>\r\n\
  </Style>\r\n\
  <StyleMap id=\"gstyle0\">\r\n\
    <Pair>\r\n\
      <key>normal</key>\r\n\
      <styleUrl>#gstyle1</styleUrl>\r\n\
    </Pair>\r\n\
    <Pair>\r\n\
      <key>highlight</key>\r\n\
      <styleUrl>#gstyle</styleUrl>\r\n\
    </Pair>\r\n\
  </StyleMap>\r\n\
  <Style id=\"gstyle1\">\r\n\
    <LineStyle>\r\n\
      <color>ffffff00</color>\r\n\
      <width>4</width>\r\n\
    </LineStyle>\r\n\
  </Style>\r\n";

const char *KML_SUFFIX = "\
</Document>\r\n\
</kml>\r\n";

// ----- Beginning/end of a pushpin in a KML file -----
const char *PUSHPIN_PREFIX_PART1 = "\
  <Placemark>\r\n\
    <name>Start ";
// PUSHPIN_PREFIX_PART2 contains the date "mm/dd/yy"
const char *PUSHPIN_PREFIX_PART3 = "\
    </name>\r\n\
    <styleUrl>#m_ylw-pushpin0</styleUrl>\r\n\
    <Point>\r\n\
      <gx:drawOrder>1</gx:drawOrder>\r\n\
      <coordinates>";

const char *PUSHPIN_SUFFIX = "</coordinates>\r\n\
    </Point>\r\n\
  </Placemark>\r\n";

// ----- Icon for breadcrumbs along route -----
const char *BREADCRUMB_STYLE = "\
  <Style id=\"crumb\">\r\n\
    <IconStyle>\r\n\
      <Icon><href>https://www.coilgun.info/images/bullet.png</href></Icon>\r\n\
      <hotSpot x=\".5\" y=\".5\" xunits=\"fraction\" yunits=\"fraction\"/>\r\n\
    </IconStyle>\r\n\
  </Style>\r\n";

// ----- Placemark template with timestamp -----
// From: https://developers.google.com/static/kml/documentation/TimeStamp_example.kml
const char *PLACE[12] = {
    "  <Placemark>\r\n",
    "    <description>\r\n",
    "      <center>\r\n",
    "        <h2><a target=\"_blank\" href=\"https://maps.google.com/maps?q=%s,%s\">%s</a></h2>\r\n",
    "          %04d-%02d-%02d <br/> %02d:%02d:%02d GMT\r\n",
    "          %s mph, %s deg, %s m, %d sat\r\n",
    "      </center>\r\n",
    "    </description>\r\n",
    "    <TimeStamp><when>%04d-%02d-%02dT%02d:%02d:%02dZ</when></TimeStamp>\r\n",
    "    <Point><coordinates>%s,%s</coordinates></Point>\r\n",
    "    <styleUrl>#crumb</styleUrl>\r\n",
    "  </Placemark>\r\n"};

void Breadcrumbs::dumpHistoryKML() {

  Serial.print(KML_PREFIX);         // begin KML file
  Serial.print(BREADCRUMB_STYLE);   // add breadcrumb icon style
  bool startFound = false;          // the first few items are typically "power up" events so look for first valid GPS event

  // loop through the GPS history buffer
  Location *item = begin();
  while (item) {
    // Location item = history[index];
    if (item->isGPS()) {
      if (!item->isEmpty()) {

        if (!startFound) {
          // this is first valid GPS event, so drop a KML pushpin at start of data
          // todo: look for "power up" events and drop another KML pushpin for each segment
          char pushpinDate[10];   // strlen("12/24/21") = 9
          TimeElements time;      // https://github.com/PaulStoffregen/Time
          breakTime(item->timestamp, time);
          snprintf(pushpinDate, sizeof(pushpinDate), "%02d/%02d/%02d", time.Month, time.Day, time.Year);

          Serial.print(PUSHPIN_PREFIX_PART1);
          Serial.print(pushpinDate);
          Serial.print(PUSHPIN_PREFIX_PART3);

          Serial.print(item->loc.lng, 4);   // KML demands longitude first
          Serial.print(",");
          Serial.print(item->loc.lat, 4);
          Serial.print(",0");
          Serial.print(PUSHPIN_SUFFIX);   // end of KML pushpin
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
        floatToCharArray(sSpeed, sizeof(sSpeed), item->speed, 1);
        floatToCharArray(sDirection, sizeof(sDirection), item->direction, 1);
        floatToCharArray(sAltitude, sizeof(sAltitude), item->altitude, 1);
        int numSats = item->numSatellites;

        char msg[128];
        // clang-format off
        snprintf(msg, sizeof(msg), PLACE[0]);     Serial.print(msg);
        snprintf(msg, sizeof(msg), PLACE[1]);     Serial.print(msg);
        snprintf(msg, sizeof(msg), PLACE[2]);     Serial.print(msg);
        snprintf(msg, sizeof(msg), PLACE[3], sLat, sLng, grid6);  Serial.print(msg);
        snprintf(msg, sizeof(msg), PLACE[4], time.Year + 1970, time.Month, time.Day, time.Hour, time.Minute, time.Second);  Serial.print(msg);
        snprintf(msg, sizeof(msg), PLACE[5], sSpeed, sDirection, sAltitude, numSats);  Serial.print(msg);
        snprintf(msg, sizeof(msg), PLACE[6]);     Serial.print(msg);
        snprintf(msg, sizeof(msg), PLACE[7]);     Serial.print(msg);
        snprintf(msg, sizeof(msg), PLACE[8], time.Year + 1970, time.Month, time.Day, time.Hour, time.Minute, time.Second);  Serial.print(msg);
        snprintf(msg, sizeof(msg), PLACE[9], sLng, sLat);  Serial.print(msg);
        snprintf(msg, sizeof(msg), PLACE[10]);    Serial.print(msg);
        snprintf(msg, sizeof(msg), PLACE[11]);    Serial.print(msg);
        // clang-format on
      }
    }
    item = next();
  }
  Serial.println(KML_SUFFIX);   // end KML file
}
