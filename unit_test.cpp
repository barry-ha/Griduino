// Please format this file with clang before check-in to GitHub
/*
  File: unit_test.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA
*/

#include <Arduino.h>             // for Serial
#include "constants.h"           // Griduino constants and colors
#include "Adafruit_ILI9341.h"    // TFT color display library
#include "morse_dac.h"           // Morse code
#include "save_restore.h"        // Configuration data in nonvolatile RAM
#include "logger.h"              // conditional printing to Serial port
#include "model_breadcrumbs.h"   // breadcrumb trail
#include "model_gps.h"           // Class Model (for model-view-controller)
#include "TextField.h"           // Optimize TFT display text for proportional fonts
#include "view.h"                // Base class for all views
#include "grid_helper.h"         // lat/long conversion routines
#include "date_helper.h"         // date/time conversions

// ========== extern ===========================================
extern void setFontSize(int font);   // TextField.cpp
extern void clearScreen();           // Griduino.ino

// ----- globals
extern Adafruit_ILI9341 tft;             // Griduino.ino
extern DACMorseSender dacMorse;          // Morse code
extern Model *model;                     // "model" portion of model-view-controller
extern ViewGrid gridView;                // Griduino.ino
extern Logger logger;                    // Griduino.ino
extern Breadcrumbs trail;                // model of breadcrumb trail
extern void showDefaultTouchTargets();   // Griduino.ino
extern Grids grid;                       // grid_helper.h
extern Dates date;                       // date_helper.h

TextField txtTest("test", 1, 21, ILI9341_WHITE);

// =============================================================
// Testing routines in view_grid_crossings.h
// This relies on "TimeLib.h" which uses "time_t" to represent time.
// The basic unit of time (time_t) is the number of seconds since Jan 1, 1970,
// a compact 4-byte integer.
// time_t is typecast to 'unsigned long int', our compiler guarantees at least 32 bits
// https://github.com/PaulStoffregen/Time

#include "view_grid_crossings.h"              // List of time spent in each grid
extern ViewGridCrossings gridCrossingsView;   // view_grid_crossings.h

// =============================================================
int testNMEAtime(const time_t expected, uint8_t yr, uint8_t mo, uint8_t day,
                 uint8_t hr, uint8_t min, uint8_t sec, int line) {

  unsigned long ulExpected = expected;
  unsigned long ulActual   = model->NMEAtoTime_t(yr, mo, day, hr, min, sec);
  char msg[120];
  snprintf(msg, sizeof(msg), "[%d] Expected %lu, actual %lu from %02d-%02d-%02d %02d:%02d:%02d",
           line, ulExpected, ulActual, yr, mo, day, hr, min, sec);
  Serial.print(msg);
  int fails = 0;
  if (ulExpected == ulActual) {
    Serial.println();   // success
  } else {
    long diff = ulExpected - ulActual;
    Serial.print(" <-- Unequal, diff = ");
    Serial.println(diff);
    fails++;
    // todo: also display message on screen, e.g. "Unit test failure at line 71"
  }
  return fails;
}

int verifyNMEAtime() {
  logger.fencepost("unittest.cpp", "verifyNMEAtime", __LINE__);

  //                       s, m, h, dow, d, m, y
  const TimeElements day_1{0, 0, 0, 1, 1, 1, (2000 - 1970)};   // Jan 1, 2000 is the first day of NMEA time
  const time_t t0  = makeTime(day_1);                          // seconds offset from 1-1-1970 to 1-1-2000
  const time_t y2k = SECS_YR_2000;                             // t0 = y2k = the time (seconds) at the start of y2k

  const TimeElements test7{0, 0, 0, 1, 1, 1, (2001 - 1970)};   // Jan 1, 2001
  const time_t time_2001_1_1 = makeTime(test7);

  // clang-format off
  //                expected           uint8_t uint8_t uint8_t uint8_t uint8_t uint8_t
  //                 time_t,         nmea year, month,day, hour, minute, seconds
  int r = 0;
  r += testNMEAtime(0 + t0,                 00,   1,   1,   0,  0,  0, __LINE__);
  r += testNMEAtime(1 + t0,                 00,   1,   1,   0,  0,  1, __LINE__);
  r += testNMEAtime(60 + t0,                00,   1,   1,   0,  1,  0, __LINE__);
  r += testNMEAtime(60 * 60 + t0,           00,   1,   1,   1,  0,  0, __LINE__);
  r += testNMEAtime(SECS_PER_DAY + t0,      00,   1,   2,   0,  0,  0, __LINE__);   // Jan 2, 2000
  r += testNMEAtime(31 * SECS_PER_DAY + t0, 00,   2,   1,   0,  0,  0, __LINE__);   // Feb 1, 2000
  r += testNMEAtime(time_2001_1_1,          01,   1,   1,   0,  0,  0, __LINE__);   // Jan 1, 2001
  r += testNMEAtime(1669431993,             22,  11,  26,   3,  6, 33, __LINE__);   // Nov 26, 2022
  // clang-format on
  return r;
}

// =============================================================
int testTimeDiff(const char *sExpected, time_t time1, time_t time2) {

  int fails = 0;
  if (time1 > time2) {
    Serial.println("testTimeDiff() error, time2 must be > time1");
    fails++;
    return fails;
  }
  long int nSeconds = (time2 - time1);
  long int nMinutes = (time2 - time1) / SECS_PER_MIN;
  long int nHours   = (time2 - time1) / SECS_PER_HOUR;
  long int nDays    = (time2 - time1) / SECS_PER_DAY;

  char sActual[16] = "todo";
  gridCrossingsView.calcTimeDiff(sActual, sizeof(sActual), time1, time2);

  Serial.print("Time difference from ");
  Serial.print(time1);
  Serial.print(" to ");
  Serial.print(time2);
  Serial.print(" is ");
  Serial.print(nSeconds);
  Serial.print("s = ");
  Serial.print(nMinutes);
  Serial.print("m = ");
  Serial.print(nHours);
  Serial.print("h = ");
  Serial.print(nDays);
  Serial.print("d. Expected '");
  Serial.print(sExpected);
  Serial.print("', actual '");
  Serial.print(sActual);
  Serial.print("'");

  /* ... removed, this will print bogus numbers, dunno why ...
  char msg[256];
  snprintf(msg, sizeof(msg), "Time difference: from %d to %d is %d sec / %d min / %d hr / %d days, expected '%s', actual '%s'",
           time1, time2, nSeconds, nMinutes, nHours, nDays, sExpected, sActual);
  Serial.print(msg);
  /* ... */

  if (strncmp(sExpected, sActual, sizeof(sActual)) == 0) {
    Serial.println();   // success
  } else {
    Serial.println(" <-- Unequal");
    fails++;
  }
  return fails;
}

// =============================================================
// verify calculating time differences into human-friendly text
int verifyCalcTimeDiff() {
  // goal to truncate fractional units to 1 decimal place (not rounding up)
  // ie, don't show "2.0 hours" until elapsed time is exactly 120 minutes or more
  logger.fencepost("unittest.cpp", "verifyCalcTimeDiff", __LINE__);
  int r = 0;

  // clang-format off
  //         expected, time1, time2
  r += testTimeDiff("30s",   now(), now() + 30);
  r += testTimeDiff("59s",   now(), now() + 60 - 1);                    // < 1 minute
  r += testTimeDiff("1m",    now(), now() + 60 + 0);                    // = 1 minute
  r += testTimeDiff("1m",    now(), now() + 60 + 1);                    // > 1 minute
  r += testTimeDiff("1m",    now(), now() + 60 + 30);                   // 1.5 minutes
  r += testTimeDiff("5m",    now(), now() + 60 * 5);                    // 5 minutes
  r += testTimeDiff("59m",   now(), now() + 60 * 60 - 1);               // < 1 hour
  r += testTimeDiff("60m",   now(), now() + 60 * 60 + 0);               // = 1 hour
  r += testTimeDiff("60m",   now(), now() + 60 * 60 + 1);               // > 1 hour
  r += testTimeDiff("89m",   now(), now() + 60 * 90 - 1);               // < 90 minutes
  r += testTimeDiff("90m",   now(), now() + 60 * 90 + 0);               // = 90 minutes
  r += testTimeDiff("90m",   now(), now() + 60 * 90 + 1);               // > 90 minutes
  r += testTimeDiff("1.6h",  now(), now() + 60 * 99);                   // 99 minutes fractional hours, e.g. "47.9h" up to two days
  r += testTimeDiff("1.9h",  now(), now() + 60 * 118);                  // < 2 hours
  r += testTimeDiff("1.9h",  now(), now() + 60 * 119);                  // < 2 hours
  r += testTimeDiff("2.0h",  now(), now() + 60 * 120);                  // = 2 hours
  r += testTimeDiff("3.0h",  now(), now() + 60 * 60 * 3);               // 3 hours
  r += testTimeDiff("12.0h", now(), now() + 60 * 60 * 12);              // 12 hours
  r += testTimeDiff("24.0h", now(), now() + 60 * 60 * 24);              // 24 hours
  r += testTimeDiff("47.9h", now(), now() + SECS_PER_DAY * 2 - 1);      // < 2 days
  r += testTimeDiff("2.0d",  now(), now() + SECS_PER_DAY * 2 + 0);      // = 2 days
  r += testTimeDiff("2.0d",  now(), now() + SECS_PER_DAY * 2 + 1);      // > 2 days
  r += testTimeDiff("2.1d",  now(), now() + SECS_PER_DAY * 2 + SECS_PER_DAY / 10);
  r += testTimeDiff("99.0d", now(), now() + SECS_PER_DAY * 99);
  r += testTimeDiff("99.9d", now(), now() + SECS_PER_DAY * 99 + SECS_PER_DAY * 9 / 10);
  r += testTimeDiff("100d",  now(), now() + SECS_PER_DAY * 100);
  r += testTimeDiff("200d",  now(), now() + SECS_PER_DAY * 200);
  r += testTimeDiff("999d",  now(), now() + SECS_PER_DAY * 999);        // 999 days = 2.7 years
  r += testTimeDiff("9999d", now(), now() + SECS_PER_DAY * 9999);       // 9999 days = 27.4 years
  // r += testTimeDiff("30s", now(), now() + 29);                       // should fail
  // r += testTimeDiff("30s", now(), now() - 30);                       // should fail
  // clang-format on
  return r;
}

// =============================================================
// Testing "date helper" routines in date_helper.h
void testElapsedTime() {
}

// =============================================================
// Testing "grid helper" routines in grid_helper.h
int testNextGridLineEast(float fExpected, double fLongitude) {
  // unit test helper for finding grid line crossings
  int r        = 0;
  float result = grid.nextGridLineEast(fLongitude);
  // clang off
  Serial.print("Grid Crossing East: given = ");   ~Serial.print(fLongitude);
  Serial.print(", expected = ");                  ~Serial.print(fExpected);
  Serial.print(", result = ");                    ~Serial.print(result);
  // clang on
  if (result == fExpected) {
    ~Serial.println("");
  } else {
    ~Serial.println(" <-- Unequal");
    r++;
  }
  return r;
}

int testNextGridLineWest(float fExpected, double fLongitude) {
  // unit test helper for finding grid line crossings
  int r        = 0;
  float result = grid.nextGridLineWest(fLongitude);
  // clang off
  Serial.print("Grid Crossing West: given = ");   Serial.print(fLongitude);
  Serial.print(", expected = ");                  Serial.print(fExpected);
  Serial.print(", result = ");                    Serial.print(result);
  // clang on
  if (result == fExpected) {
    Serial.println("");
  } else {
    Serial.println(" <-- Unequal");
    r++;
  }
  return r;
}

int testCalcLocator4(const char *sExpected, double lat, double lon) {
  // unit test helper function to display results
  int r = 0;
  char sResult[7];   // strlen("CN87") = 4
  grid.calcLocator(sResult, lat, lon, 4);
  Serial.print("testCalcLocator4: given (");
  Serial.print(lat, 4);
  Serial.print(",");
  Serial.print(lon, 4);
  Serial.print(") expected = ");
  Serial.print(sExpected);
  Serial.print(", gResult = ");
  Serial.print(sResult);
  if (strcmp(sResult, sExpected) == 0) {
    Serial.println("");
  } else {
    Serial.println(" <-- Unequal");
    r++;
  }
  return r;
}

int testCalcLocator6(const char *sExpected, double lat, double lon) {
  // unit test helper function to display results
  int r = 0;
  char sResult[7];   // strlen("CN87us") = 6
  grid.calcLocator(sResult, lat, lon, 6);
  Serial.print("testCalcLocator6: given (");
  Serial.print(lat, 4);
  Serial.print(",");
  Serial.print(lon, 4);
  Serial.print(") expected = ");
  Serial.print(sExpected);
  Serial.print(", gResult = ");
  Serial.print(sResult);
  if (strcmp(sResult, sExpected) == 0) {
    Serial.println("");
  } else {
    Serial.println(" <-- Unequal");
    r++;
  }
  return r;
}

int testCalcLocator8(const char *sExpected, double lat, double lon) {
  // unit test helper function to display results
  int r = 0;
  char sResult[9];   // strlen("CN87us00") = 8
  grid.calcLocator(sResult, lat, lon, 8);
  Serial.print("testCalcLocator8: (");
  Serial.print(lat, 4);
  Serial.print(",");
  Serial.print(lon, 4);
  Serial.print(") expected = ");
  Serial.print(sExpected);
  Serial.print(", gResult = ");
  Serial.print(sResult);
  if (strcmp(sResult, sExpected) == 0) {
    Serial.println("");
  } else {
    Serial.println(" <-- Unequal");
    r++;
  }
  return r;
}

// =============================================================
// Testing "distance helper" routines in Griduino.cpp
int testDistanceLat(double expected, double fromLat, double toLat) {
  // unit test helper function to calculate N-S distances
  double distance = grid.calcDistanceLat(fromLat, toLat, model->gMetric);
  Serial.print("N-S Distance Test: expected = ");
  Serial.print(expected, 2);
  Serial.print(", result = ");
  Serial.println(distance, 4);
  return 0;   // todo
}
int testDistanceLong(double expected, double lat, double fromLong, double toLong) {
  // unit test helper function to calculate E-W distances
  int r         = 0;
  double result = grid.calcDistanceLong(lat, fromLong, toLong, model->gMetric);
  Serial.print("E-W Distance Test: expected = ");
  Serial.print(expected, 2);
  Serial.print(", result = ");
  Serial.print(result, 2);
  if ((expected / 1.01) <= result && result <= (expected * 1.01)) {
    Serial.println("");
  } else {
    Serial.println(" <-- Unequal");
    r++;
  }
  return r;
}
// =============================================================
// verify Morse Code
int verifyMorseCode() {
  logger.fencepost("unittest.cpp", "verifyMorseCode", __LINE__);
  logger.info("Connect speaker and listen");
  dacMorse.setup();
  dacMorse.dump();

  logger.info("\nStarting dits");
  for (int ii = 1; ii <= 40; ii++) {
    //~Serial.print(ii); //~Serial.print(" ");
    if (ii % 10 == 0)   //~Serial.print("\n");
      dacMorse.send_dit();
  }
  logger.info("Finished dits");
  dacMorse.send_word_space();

  // ----- test dit-dah
  logger.info("Starting dit-dah");
  for (int ii = 1; ii <= 20; ii++) {
    //~Serial.print(ii); //~Serial.print(" ");
    if (ii % 10 == 0)   //~Serial.print("\n");
      dacMorse.send_dit();
    dacMorse.send_dah();
  }
  dacMorse.send_word_space();
  logger.info("Finished dit-dah");
  // Indicate success - this test is not able to detect errors
  // To validate morse code, connect speaker and listen
  return 0;
}
// =============================================================
// verify Save/Restore Volume settings in SDRAM
int verifySaveRestoreVolume() {
  logger.fencepost("unittest.cpp", "verifySaveRestoreVolume", __LINE__);
  logger.info("Please watch the TFT display");

#define TEST_CONFIG_FILE    CONFIG_FOLDER "/test.cfg"   // strictly 8.3 names
#define TEST_CONFIG_VERSION "Test v02"
#define TEST_CONFIG_VALUE   5
  int writeValue = TEST_CONFIG_VALUE;
  int readValue  = 0;   // different from "writeValue"
  int fails      = 0;   // assume no errors

  // sample data to read/write
  SaveRestore configWrite(TEST_CONFIG_FILE, TEST_CONFIG_VERSION);
  SaveRestore configRead(TEST_CONFIG_FILE, TEST_CONFIG_VERSION);

  // test writing data to SDRAM -- be sure to watch serial console log
  if (configWrite.writeConfig((byte *)&writeValue, sizeof(writeValue))) {
    Serial.println("Success, integer stored to SDRAM");
  } else {
    Serial.println("ERROR! Unable to save integer to SDRAM");
    fails++;
  }

  // test reading same data back from SDRAM
  if (configRead.readConfig((byte *)&readValue, sizeof(readValue))) {
    Serial.println("Success, integer restored from SDRAM");
    if (readValue == writeValue) {
      Serial.println("Success, correct value was restored");
    } else {
      Serial.println("ERROR! The value restored did NOT match the value saved");
      fails++;
    }
  } else {
    Serial.println("ERROR! Unable to restore integer from SDRAM");
    fails++;
  }
  configWrite.remove(TEST_CONFIG_FILE);
  return fails;
}

// =============================================================
int verifySaveRestoreArray() {
  logger.fencepost("unittest.cpp", "verifySaveRestoreArray", __LINE__);
  int fails = 0;   // assume no errors detected

#define TEST_ARRAY_FILE CONFIG_FOLDER "/testarry.cfg"
#define TEST_ARRAY_VERS "Array v02"
  int iData[21];
  const int numData = sizeof(iData) / sizeof(int);
  for (int ii = 0; ii < numData; ii++) {
    iData[ii] = ii;
  }

  SaveRestore writeArray(TEST_ARRAY_FILE, TEST_ARRAY_VERS);
  SaveRestore readArray(TEST_ARRAY_FILE, TEST_ARRAY_VERS);

  if (writeArray.writeConfig((byte *)&iData, sizeof(iData))) {   // test writing data to SDRAM -- be sure to watch serial console log
    Serial.println("Success, array stored to SDRAM");
  } else {
    Serial.println("ERROR! Failed to save array to SDRAM");
    fails++;
  }

  int iResult[numData];
  if (readArray.readConfig((byte *)&iResult, sizeof(iResult))) {   // test reading same data back from SDRAM
    Serial.println("Success, array retrieved from SDRAM");
    bool success = true;   // assume successful comparisons
    for (int ii = 0; ii < numData; ii++) {
      if (iResult[ii] != iData[ii]) {
        success = false;
        fails++;
        char temp[100];
        snprintf(temp, sizeof(temp),
                 "ERROR! Array index (%d) value restored (%d) did NOT match the value saved (%d)",
                 ii, iResult[ii], iData[ii]);
        Serial.println(temp);
      }
    }
    if (success) {
      Serial.println("Success, all values correct in array restored from SDRAM");
    }
  } else {
    Serial.println("ERROR! Unable to restore from SDRAM");
    fails++;
  }
  writeArray.remove(TEST_ARRAY_FILE);
  return fails;
}
// =============================================================
// verify save/restore GPS model state in SDRAM
int verifySaveRestoreGPSModel() {
  logger.fencepost("unittest.cpp", "verifySaveRestoreGPSModel", __LINE__);
  int fails = 0;   // assume no errors detected

#define TEST_GPS_STATE_FILE CONFIG_FOLDER "/test_gps.cfg"   // strictly 8.3 naming
#define TEST_GPS_STATE_VERS "Test v02"

  Model gpsModel;   // sample data to read/write, a different object than used in model_gps.h

  if (gpsModel.save()) {
    Serial.println("Success, GPS model stored to SDRAM");
  } else {
    Serial.print("Unit test error: Failed to save GPS model to SDRAM, line ");
    Serial.println(__LINE__);
    fails++;
  }

  if (gpsModel.restore()) {   // test reading same data back from SDRAM
    Serial.println("Success, GPS model restored from SDRAM");
  } else {
    Serial.print("Unit test error: Failed to retrieve GPS model from SDRAM, line ");
    Serial.println(__LINE__);
    fails++;
  }
  return fails;
}
// =============================================================
// verify painting individual bread crumbs (locations)
int verifyBreadCrumbs() {
  // plotting a series of pushpins (bread crumb trail)
  logger.fencepost("unittest.cpp", "verifyBreadCrumbs", __LINE__);
  int fails = 0;   // assume no errors detected

  // move the model to known location (CN87)
  // model->gsGridName = "CN87";
  model->gLatitude  = 47.737451;     // CN87
  model->gLongitude = -122.274711;   // CN87

  // reduce the frequency of saving to memory
  trail.saveInterval = 20;  // 100;   // default 2 is too often

  // initialize the canvas that we will draw upon
  gridView.startScreen();   // clear and draw normal screen
  txtTest.print();
  txtTest.dirty = true;
  txtTest.print();

  delay(3000);   // time for serial monitor to connect, and human to look at TFT display

  float xPixelsPerDegree = gBoxWidth / gridWidthDegrees;     // grid square = 2.0 degrees wide E-W
  float yPixelsPerDegree = gBoxHeight / gridHeightDegrees;   // grid square = 1.0 degrees high N-S

  // ----- plot each corner, starting upper left, moving clockwise
  model->gLatitude  = 48.0 - 0.1;   // upper left
  model->gLongitude = -124.0 + 0.1;
  gridView.updateScreen();
  delay(500);

  model->gLatitude  = 48.0 - 0.1;   // upper right
  model->gLongitude = -122.0 - 0.1;
  gridView.updateScreen();
  delay(500);

  model->gLatitude  = 47.0 + 0.1;   // lower right
  model->gLongitude = -122.0 - 0.1;
  gridView.updateScreen();
  delay(500);

  model->gLatitude  = 47.0 + 0.1;   // lower left
  model->gLongitude = -124.0 + 0.1;
  gridView.updateScreen();
  delay(500);
  trail.saveInterval = 2;   // restore setting
  return fails;             //
}
// =============================================================
// verify painting a trail of bread crumbs (locations)
int verifyBreadCrumbTrail1() {
  logger.fencepost("unittest.cpp", "verifyBreadCrumbTrail1", __LINE__);
  int r = 0;

  // initialize the canvas to draw on
  gridView.startScreen();   // clear and draw normal screen
  txtTest.dirty = true;
  txtTest.print();

  // test 2: loop through locations that cross this grid
  float lat      = 47.0 - 0.2;     // 10% outside of CN87
  float lon      = -124.0 - 0.1;   //
  int steps      = 15;             // = trail.numHistory;     // number of loops
  float stepSize = 15.0 / 250.0;   // number of degrees to move each loop

  trail.clearHistory();
  
  trail.rememberPUP();      // test "power up" record type

  // test GPS record type
  for (int ii = 0; ii < steps; ii++) {
    PointGPS latLong{model->gLatitude  = lat + (ii * stepSize),            // "plus" goes upward (north)
                     model->gLongitude = lon + (ii * stepSize * 5 / 4)};   // "plus" goes rightward (east)
    time_t stamp = now();
    delay(100);
    // doesn't matter what timestamp/sats/speed/direction/altitude is actually stored during tests
    Location loc{rGPS, latLong, stamp, 5, 10.0, 45.0, 123.0};
    trail.remember(loc);
  }

  // dumpHistory();          // did it remember? dump history to monitor
  trail.rememberPUP();       // test another "power up" record type
  gridView.updateScreen();   //
  trail.dumpHistoryGPS();    // also dump history to console
  trail.clearHistory();      // clean up so it is not re-displayed by main program
  return r;
}
// =============================================================
// GPS test helper
void generateSineWave(Model *pModel) {

  // fill history table with a sine wave
  double startLat  = 47.5;   // 10% outside of CN87
  double startLong = -124.0 - 0.6;
  // don't use "steps = trail.numHistory" because remember() will write each one to SdFat,
  // filling the console log and taking a very long time
  int steps        = 13;                        // number of loops, pref prime number
  double stepSize  = (125.3 - 123.7) / steps;   // degrees longitude to move each loop
  double amplitude = 0.65;                      // degrees latitude, maximum sine wave

  for (int ii = 0; ii < steps; ii++) {
    float longitude = startLong + (ii * stepSize);
    float latitude  = startLat + amplitude * sin(longitude * 150 / degreesPerRadian);
    PointGPS latLong{latitude, longitude};
    time_t stamp = now();
    // doesn't matter what timestamp/sats/speed/direction/altitude is actually stored during tests
    Location loc{rGPS, latLong, stamp, 5, 10.0, 45.0, 123.0};
    trail.remember(loc);
  }
  // Serial.println("---History as known by generateSineWave()...");
  // pModel->dumpHistory();            // did it remember? (go review serial console)
}
// =============================================================
// verify painting a trail of bread crumbs (locations)
int verifyBreadCrumbTrail2() {
  logger.fencepost("unittest.cpp", "verifyBreadCrumbTrail2", __LINE__);
  int r = 0;

  // initialize the canvas to draw on
  gridView.startScreen();   // clear and draw normal screen
  txtTest.dirty = true;     // paint big "Test" in upper left
  txtTest.print();

  trail.clearHistory();
  trail.rememberPUP();       // test "power up" record type
  generateSineWave(model);   // fill GPS model with known test data

  Serial.println(". History as known by verifyBreadCrumbTrail2()...");
  trail.dumpHistoryGPS();    // did it remember? (go review serial console)
  gridView.updateScreen();   // does it look like a sine wave? (go look at TFT display)
  return r;
}
// =============================================================
// save GPS route to non-volatile memory
int verifySaveTrail() {
  logger.fencepost("unittest.cpp", "verifySaveTrail", __LINE__);

  Model testModel;   // create an instance of the model for this test

  generateSineWave(&testModel);   // generate known data to be saved

  // this is automatically saved to non-volatile memory by the trail.remember()
  return 0;   // this routine does not detect errors, it exists to check for clean compile
}
// =============================================================
// restore GPS route from non-volatile memory
int verifyRestoreTrail() {
  logger.fencepost("unittest.cpp", "verifyRestoreTrail", __LINE__);
  logger.info("todo");
  return 0;   // todo
}
// =============================================================
// deriving grid square from lat-long coordinates
int verifyDerivingGridSquare() {
  logger.fencepost("unittest.cpp", "verifyDerivingGridSquare", __LINE__);
  int fails = 0;

  // Expected values from: https://www.movable-type.co.uk/scripts/latlong.html
  //                        expected  lat   long
  fails += testCalcLocator4("AA00", -89.5, -179.0);      // south pole
  fails += testCalcLocator4("BC12", -67.5, -157.0);      // southern ocean
  fails += testCalcLocator4("FD64", -55.222, -67.385);   // southern tip of south america
  fails += testCalcLocator4("LG89", -20.519, 57.962);    // mauritius island
  fails += testCalcLocator4("JP99", 69.635, 18.558);     // tromso, norway
  //                        expected  lat    long
  fails += testCalcLocator4("CN87", 47.001, -123.999);   // sw corner of CN87 is CN87aa00
  fails += testCalcLocator4("CN87", 47.999, -123.999);   // se corner of CN87 is CN87ax09
  fails += testCalcLocator4("CN87", 47.001, -122.001);   // nw corner of CN87 is CN87xa90
  fails += testCalcLocator4("CN87", 47.999, -122.001);   // ne corner of CN87 is CN87xx99
  //                        expected    lat        long
  fails += testCalcLocator6("CN87us", 47.753000, -122.28470);    // read console log for failure messages
  fails += testCalcLocator6("CN85uk", 45.423100, -122.2847);     //
  fails += testCalcLocator6("EM66pd", 36.165926, -86.723285);    // +,-
  fails += testCalcLocator6("OF86cx", -33.014673, 116.230695);   // -,+
  fails += testCalcLocator6("FD54oq", -55.315349, -68.794971);   // -,-
  fails += testCalcLocator6("PM85ge", 35.205535, 136.565790);    // +,+
  //                        expected     lat        long
  fails += testCalcLocator8("CN87us00", 47.75191, -122.329514);    // read console log for failure messages
  fails += testCalcLocator8("CN87us10", 47.75191, -122.321181);    //
  fails += testCalcLocator8("CN87us90", 47.75191, -122.254514);    //
  fails += testCalcLocator8("CN87us01", 47.756076, -122.329514);   //
  fails += testCalcLocator8("CN87us09", 47.78941, -122.329514);    //
  //                        expected     lat        long
  fails += testCalcLocator8("CN87us91", 47.756076, -122.254514);   // read console log for failure messages
  fails += testCalcLocator8("CN87us92", 47.760243, -122.254514);   //
  fails += testCalcLocator8("CN87us93", 47.764410, -122.254514);   //
  fails += testCalcLocator8("CN87us94", 47.768576, -122.254514);   //
  fails += testCalcLocator8("CN87us95", 47.772743, -122.254514);   //
  fails += testCalcLocator8("CN87us96", 47.776910, -122.254514);   //
  fails += testCalcLocator8("CN87us97", 47.781076, -122.254514);   //
  fails += testCalcLocator8("CN87us98", 47.784243, -122.254514);   //
  fails += testCalcLocator8("CN87us99", 47.789410, -122.254514);   //
  //                        expected     lat        long
  fails += testCalcLocator8("CN85uk51", 45.4231, -122.2847);       // read console log for failure messages
  fails += testCalcLocator8("EM66pd39", 36.165926, -86.723285);    // +,-
  fails += testCalcLocator8("OF86cx76", -33.014673, 116.230695);   // -,+
  fails += testCalcLocator8("FD54oq44", -55.315349, -68.794971);   // -,-
  fails += testCalcLocator8("PM85ge79", 35.205535, 136.565790);    // +,+
  return fails;
}
// =============================================================
// verify computing distance
int verifyComputingDistance() {
  logger.fencepost("unittest.cpp", "verifyComputingDistance", __LINE__);
  int r = 0;

  //              expected    fromLat     toLat
  r += testDistanceLat(30.1, 47.56441, 48.00000);   // from home to north, 48.44 km = 30.10 miles
  r += testDistanceLat(39.0, 47.56441, 47.00000);   //  "    "   "  south, 62.76 km = 39.00 miles
  //              expected   lat     fromLong    toLong
  r += testDistanceLong(13.2, 47.7531, -122.2845, -122.0000);    //  "    "   "  east,  x.xx km =  x.xx miles
  r += testDistanceLong(79.7, 47.7531, -122.2845, -124.0000);    //  "    "   "  west,  xx.x km = xx.xx miles
  r += testDistanceLong(52.9, 67.5000, -158.0000, -156.0000);    // width of BP17 Alaska, 85.1 km = 52.88 miles
  r += testDistanceLong(93.4, 47.5000, -124.0000, -122.0000);    // width of CN87 Seattle, 150.2 km = 93.33 miles
  r += testDistanceLong(113.0, 35.5000, -116.0000, -118.0000);   // width of DM15 California is >100 miles, 181 km = 112.47 miles
  r += testDistanceLong(138.0, 0.5000, -80.0000, -78.0000);      // width of FJ00 Ecuador is the largest possible, 222.4 km = 138.19 miles
  return r;
}
// =============================================================
// verify finding grid lines on E and W
int verifyComputingGridLines() {
  logger.fencepost("unittest.cpp", "verifyComputingGridLines", __LINE__);
  int r = 0;

  //                       expected  fromLongitude
  r += testNextGridLineEast(-122.0, -122.2836);
  r += testNextGridLineWest(-124.0, -122.2836);

  r += testNextGridLineEast(-120.0, -121.8888);
  r += testNextGridLineWest(-122.0, -121.8888);

  r += testNextGridLineEast(14.0, 12.3456);
  r += testNextGridLineWest(12.0, 12.3456);
  return r;
}
// =============================================================
void countDown(int iSeconds) {
  Serial.print("Wait ");
  setFontSize(0);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
  for (int ii = iSeconds; ii > 0; ii--) {
    Serial.print(ii);   // to console
    // Serial.print(" ");
    tft.setCursor(2, gScreenHeight - 16);
    tft.print(" Wait ");   // to TFT display
    tft.print(ii);
    tft.print(" ");
    delay(1000);
  }
  Serial.println();
}
// ================ main unit test =============================
void runUnitTest() {
  tft.fillScreen(ILI9341_BLACK);

  // ----- announce ourselves
  setFontSize(24);
  tft.setCursor(12, 38);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("--Unit Test--");
  Serial.println("--Unit Test--");

  setFontSize(0);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(0, tft.height() - 12);   // move to bottom row
  tft.print("  --Open console monitor to see unit test results--");
  delay(1000);

  int f = 0;
/*****
  f += verifyNMEAtime();              // verify conversions from GPS' time (NMEA) to time_t
  countDown(5);                       //
  f += verifyCalcTimeDiff();          // verify human-friendly time intervals
  countDown(5);                       //
  f += verifyMorseCode();             // verify Morse code
  f += verifySaveRestoreVolume();     // verify save/restore an integer setting in SDRAM
  countDown(5);                       //
  f += verifySaveRestoreArray();      // verify save/restore an array in SDRAM
  countDown(5);                       //
  f += verifySaveRestoreGPSModel();   // verify save/restore GPS model state in SDRAM
  countDown(5);                       //
  f += verifyBreadCrumbs();           // verify pushpins near the four corners
  countDown(5);                       //
*****/
  f += verifyBreadCrumbTrail1();      // verify painting the bread crumb trail
  countDown(5);                       //
  f += verifyBreadCrumbTrail2();      // verify painting the bread crumb trail
  countDown(5);                       //
  f += verifySaveTrail();             // save GPS route to non-volatile memory
  countDown(5);                       //
  f += verifyRestoreTrail();          // restore GPS route from non-volatile memory
  countDown(5);                       //
/*****
  f += verifyDerivingGridSquare();    // verify deriving grid square from lat-long coordinates
  countDown(5);                       //
  f += verifyComputingDistance();     // verify computing distance
  f += verifyComputingGridLines();    // verify finding grid lines on E and W
  countDown(5);                       // give user time to inspect display appearance for unit test problems
*****/
  trail.clearHistory();               // clean up our mess after unit test
  trail.rememberPUP();

  logger.fencepost("unittest.cpp", "End Unit Test", __LINE__);
  if (f) {
    Serial.println("====================");
    Serial.print(f);
    Serial.println(" failures");
  } else {
    Serial.println("100% successful");
  }
}
