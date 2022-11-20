// Please format this file with clang before check-in to GitHub
/*
  File: unit_test.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA
*/

//#include <Arduino.h>
#include "constants.h"          // Griduino constants and colors
#include "Adafruit_ILI9341.h"   // TFT color display library
#include "morse_dac.h"          // Morse code
#include "save_restore.h"       // Configuration data in nonvolatile RAM
#include "model_gps.h"          // Class Model (for model-view-controller)
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views
#include "logger.h"             // conditional printing to Serial port
#include "grid_helper.h"        // lat/long conversion routines
#include "date_helper.h"        // date/time conversions

// ========== extern ===========================================
extern void setFontSize(int font);   // TextField.cpp
extern void clearScreen();           // Griduino.ino

// ----- globals
extern Adafruit_ILI9341 tft;             // Griduino.ino
extern DACMorseSender dacMorse;          // Morse code
extern Model *model;                     // "model" portion of model-view-controller
extern const int numHistory;             // Griduino.ino, number of elements in history[]
extern ViewGrid gridView;                // Griduino.ino
extern Logger logger;                    // Griduino.ino
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

void testTimeDiff(const char *sExpected, time_t time1, time_t time2) {
  /* ... debug ...
  Serial.print("Check input: sExpected = ");
  Serial.print(sExpected);
  Serial.print(", time1 = ");
  Serial.print(time1);
  Serial.print(", time2 = ");
  Serial.println(time2);
  /* ... */

  if (time1 > time2) {
    Serial.println("testTimeDiff() error, time2 must be > time1");
    return;
  }
  long int nSeconds = (time2 - time1);
  long int nMinutes = (time2 - time1) / SECS_PER_MIN;
  long int nHours   = (time2 - time1) / SECS_PER_HOUR;
  long int nDays    = (time2 - time1) / SECS_PER_DAY;

  char sActual[16] = "todo";
  gridCrossingsView.calcTimeDiff(sActual, sizeof(sActual), time1, time2);

  Serial.print("Time difference: from ");
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
  
  /* ... removed, prints bogus numbers, dunno why ...
  char msg[256];
  snprintf(msg, sizeof(msg), "Time difference: from %d to %d is %d sec / %d min / %d hr / %d days, expected '%s', actual '%s'",
           time1, time2, nSeconds, nMinutes, nHours, nDays, sExpected, sActual);
  Serial.print(msg);
  /* ... */

  if (strncmp(sExpected, sActual, sizeof(sActual)) == 0) {
    Serial.println();   // success
  } else {
    Serial.println(" <-- Unequal");
  }
}
// =============================================================
// verify calculating time differences into human-friendly text
void verifyCalcTimeDiff() {
  Serial.print("-------- verifyCalcTimeDiff() at line ");
  Serial.println(__LINE__);

  //           expected  fromLongitude
  testTimeDiff("30s", now(), now() + 30);
  testTimeDiff("59s", now(), now() + 60 - 1);        // < 1 minute
  testTimeDiff("1m", now(), now() + 60 + 0);         // = 1 minute
  testTimeDiff("1m", now(), now() + 60 + 1);         // > 1 minute
  testTimeDiff("1m", now(), now() + 60 + 30);        // 1.5 minutes
  testTimeDiff("5m", now(), now() + 60 * 5);         // 5 minutes
  testTimeDiff("59m", now(), now() + 60 * 60 - 1);   // < 1 hour
  testTimeDiff("60m", now(), now() + 60 * 60 + 0);   // = 1 hour
  testTimeDiff("60m", now(), now() + 60 * 60 + 1);   // > 1 hour
  testTimeDiff("89m", now(), now() + 60 * 90 - 1);   // < 90 minutes
  testTimeDiff("90m", now(), now() + 60 * 90 + 0);   // = 90 minutes
  testTimeDiff("90m", now(), now() + 60 * 90 + 1);   // > 90 minutes
  testTimeDiff("99m", now(), now() + 60 * 99);       // 99 minutes
  testTimeDiff("119m", now(), now() + 60 * 119);     // almost 2 hours
  testTimeDiff("2h", now(), now() + 60 * 60 * 2);    // fractional hours, e.g. "47.9h" up to two days
  testTimeDiff("3h", now(), now() + 60 * 60 * 3);
  testTimeDiff("1d", now(), now() + 60 * 60 * 24);
  testTimeDiff("47.9h", now(), now() + SECS_PER_DAY * 2 - 1);   // < 2 days
  testTimeDiff("2d", now(), now() + SECS_PER_DAY * 2 + 0);      // = 2 days
  testTimeDiff("2d", now(), now() + SECS_PER_DAY * 2 + 1);      // > 2 days
  testTimeDiff("2.1d", now(), now() + SECS_PER_DAY * 2 + SECS_PER_DAY / 10);
  testTimeDiff("99d", now(), now() + SECS_PER_DAY * 99);
  testTimeDiff("99.9d", now(), now() + SECS_PER_DAY * 99 + SECS_PER_DAY * 10 / 9);
  testTimeDiff("100d", now(), now() + SECS_PER_DAY * 100);
  testTimeDiff("200d", now(), now() + SECS_PER_DAY * 100 * 2);
  // testTimeDiff("30s", now(), now() + 29);   // should fail
  // testTimeDiff("30s", now(), now() - 30);   // should fail
}

// =============================================================
// Testing "date helper" routines in date_helper.h
void testElapsedTime() {
}

// =============================================================
// Testing "grid helper" routines in grid_helper.h
void testNextGridLineEast(float fExpected, double fLongitude) {
  // unit test helper for finding grid line crossings
  float result = grid.nextGridLineEast(fLongitude);
  //~Serial.print("Grid Crossing East: given = "); //~Serial.print(fLongitude);
  //~Serial.print(", expected = "); //~Serial.print(fExpected);
  //~Serial.print(", result = "); //~Serial.print(result);
  if (result == fExpected) {
    //~Serial.println("");
  } else {
    //~Serial.println(" <-- Unequal");
  }
}

void testNextGridLineWest(float fExpected, double fLongitude) {
  // unit test helper for finding grid line crossings
  float result = grid.nextGridLineWest(fLongitude);
  Serial.print("Grid Crossing West: given = ");
  Serial.print(fLongitude);
  Serial.print(", expected = ");
  Serial.print(fExpected);
  Serial.print(", result = ");
  Serial.print(result);
  if (result == fExpected) {
    Serial.println("");
  } else {
    Serial.println(" <-- Unequal");
  }
}

void testCalcLocator6(const char *sExpected, double lat, double lon) {
  // unit test helper function to display results
  char sResult[7];   // strlen("CN87us") = 6
  grid.calcLocator(sResult, lat, lon, 6);
  Serial.print("Test: given (");
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
  }
}

void testCalcLocator8(const char *sExpected, double lat, double lon) {
  // unit test helper function to display results
  char sResult[9];   // strlen("CN87us00") = 8
  grid.calcLocator(sResult, lat, lon, 8);
  Serial.print("Test: expected = ");
  Serial.print(sExpected);
  Serial.print(", gResult = ");
  Serial.print(sResult);
  if (strcmp(sResult, sExpected) == 0) {
    Serial.println("");
  } else {
    Serial.println(" <-- Unequal");
  }
}

// =============================================================
// Testing "distance helper" routines in Griduino.cpp
void testDistanceLat(double expected, double fromLat, double toLat) {
  // unit test helper function to calculate N-S distances
  double distance = grid.calcDistanceLat(fromLat, toLat, model->gMetric);
  Serial.print("N-S Distance Test: expected = ");
  Serial.print(expected, 2);
  Serial.print(", result = ");
  Serial.println(distance, 4);
}
void testDistanceLong(double expected, double lat, double fromLong, double toLong) {
  // unit test helper function to calculate E-W distances
  double result = grid.calcDistanceLong(lat, fromLong, toLong, model->gMetric);
  Serial.print("E-W Distance Test: expected = ");
  Serial.print(expected, 2);
  Serial.print(", result = ");
  Serial.print(result, 2);
  if ((expected / 1.01) <= result && result <= (expected * 1.01)) {
    Serial.println("");
  } else {
    Serial.println(" <-- Unequal");
  }
}
// =============================================================
// verify Morse Code
void verifyMorseCode() {
  // Serial.print("-------- verifyMorseCode() at line "); Serial.println(__LINE__);
  logger.fencepost("unit_test.cpp verifyMorseCode", __LINE__);
  dacMorse.setup();
  dacMorse.dump();

  Serial.print("\nStarting dits\n");
  for (int ii = 1; ii <= 40; ii++) {
    //~Serial.print(ii); //~Serial.print(" ");
    if (ii % 10 == 0)   //~Serial.print("\n");
      dacMorse.send_dit();
  }
  Serial.print("Finished dits\n");
  dacMorse.send_word_space();

  // ----- test dit-dah
  Serial.print("\nStarting dit-dah\n");
  for (int ii = 1; ii <= 20; ii++) {
    //~Serial.print(ii); //~Serial.print(" ");
    if (ii % 10 == 0)   //~Serial.print("\n");
      dacMorse.send_dit();
    dacMorse.send_dah();
  }
  dacMorse.send_word_space();
  Serial.print("Finished dit-dah\n");
}
// =============================================================
// verify Save/Restore Volume settings in SDRAM
void verifySaveRestoreVolume() {
  Serial.print("-------- verifySaveRestoreVolume() at line ");
  Serial.println(__LINE__);

#define TEST_CONFIG_FILE    CONFIG_FOLDER "/test.cfg"   // strictly 8.3 names
#define TEST_CONFIG_VERSION "Test v02"
#define TEST_CONFIG_VALUE   5
  int writeValue = TEST_CONFIG_VALUE;
  int readValue  = 0;   // different from "writeValue"

  // sample data to read/write
  SaveRestore configWrite(TEST_CONFIG_FILE, TEST_CONFIG_VERSION);
  SaveRestore configRead(TEST_CONFIG_FILE, TEST_CONFIG_VERSION);

  // test writing data to SDRAM -- be sure to watch serial console log
  if (configWrite.writeConfig((byte *)&writeValue, sizeof(writeValue))) {
    Serial.println("Success, integer stored to SDRAM");
  } else {
    Serial.println("ERROR! Unable to save integer to SDRAM");
  }

  // test reading same data back from SDRAM
  if (configRead.readConfig((byte *)&readValue, sizeof(readValue))) {
    Serial.println("Success, integer restored from SDRAM");
    if (readValue == writeValue) {
      Serial.println("Success, correct value was restored");
    } else {
      Serial.println("ERROR! The value restored did NOT match the value saved");
    }
  } else {
    Serial.println("ERROR! Unable to restore integer from SDRAM");
  }
  configWrite.remove(TEST_CONFIG_FILE);
}

// =============================================================
void verifySaveRestoreArray() {
  Serial.print("-------- verifySaveRestoreArray() at line ");
  Serial.println(__LINE__);

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
  }

  int iResult[numData];
  if (readArray.readConfig((byte *)&iResult, sizeof(iResult))) {   // test reading same data back from SDRAM
    Serial.println("Success, array retrieved from SDRAM");
    bool success = true;   // assume successful comparisons
    for (int ii = 0; ii < numData; ii++) {
      if (iResult[ii] != iData[ii]) {
        success = false;
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
  }
  writeArray.remove(TEST_ARRAY_FILE);
}
// =============================================================
// verify save/restore GPS model state in SDRAM
void verifySaveRestoreGPSModel() {
#define TEST_GPS_STATE_FILE CONFIG_FOLDER "/test_gps.cfg"   // strictly 8.3 naming
#define TEST_GPS_STATE_VERS "Test v02"
  Serial.print("-------- verifySaveRestoreGPSModel() at line ");
  Serial.println(__LINE__);

  Model gpsModel;   // sample data to read/write, a different object than used in model_gps.h

  if (gpsModel.save()) {
    Serial.println("Success, GPS model stored to SDRAM");
  } else {
    Serial.print("Unit test error: Failed to save GPS model to SDRAM, line ");
    Serial.println(__LINE__);
  }

  if (gpsModel.restore()) {   // test reading same data back from SDRAM
    Serial.println("Success, GPS model restored from SDRAM");
  } else {
    Serial.print("Unit test error: Failed to retrieve GPS model from SDRAM, line ");
    Serial.println(__LINE__);
  }
}
// =============================================================
// verify painting individual bread crumbs (locations)
void verifyBreadCrumbs() {
  // plotting a series of pushpins (bread crumb trail)
  Serial.print("-------- verifyBreadCrumbs() at line ");
  Serial.println(__LINE__);

  // move the model to known location (CN87)
  // model->gsGridName = "CN87";
  model->gLatitude  = 47.737451;     // CN87
  model->gLongitude = -122.274711;   // CN87

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
}
// =============================================================
// verify painting a trail of bread crumbs (locations)
void verifyBreadCrumbTrail1() {
  Serial.print("-------- verifyBreadCrumbTrail1() at line ");
  Serial.println(__LINE__);

  // initialize the canvas to draw on
  gridView.startScreen();   // clear and draw normal screen
  txtTest.dirty = true;
  txtTest.print();

  // test 2: loop through locations that cross this grid
  float lat      = 47.0 - 0.2;     // 10% outside of CN87
  float lon      = -124.0 - 0.1;   //
  int steps      = numHistory;     // number of loops
  float stepSize = 1.0 / 250.0;    // number of degrees to move each loop

  model->clearHistory();
  for (int ii = 0; ii < steps; ii++) {
    PointGPS location{model->gLatitude  = lat + (ii * stepSize),            // "plus" goes upward (north)
                      model->gLongitude = lon + (ii * stepSize * 5 / 4)};   // "plus" goes rightward (east)
    time_t stamp = now();                                                   // doesn't matter what timestamp is actually stored during tests
    model->remember(location, stamp);
  }

  // dumpHistory();          // did it remember? dump history to monitor
  gridView.updateScreen();   //
  model->clearHistory();     // clean up so it is not re-displayed by main program
}
// =============================================================
// GPS test helper
void generateSineWave(Model *pModel) {

  // fill history table with a sine wave
  double startLat  = 47.5;   // 10% outside of CN87
  double startLong = -124.0 - 0.6;
  int steps        = numHistory;                // number of loops
  double stepSize  = (125.5 - 121.5) / steps;   // degrees longitude to move each loop
  double amplitude = 0.65;                      // degrees latitude, maximum sine wave

  for (int ii = 0; ii < steps; ii++) {
    float longitude = startLong + (ii * stepSize);
    float latitude  = startLat + amplitude * sin(longitude * 150 / degreesPerRadian);
    PointGPS location{latitude, longitude};
    time_t stamp = now();   // doesn't matter what timestamp is actually stored during tests
    pModel->remember(location, stamp);
  }
  // Serial.println("---History as known by generateSineWave()...");
  // pModel->dumpHistory();            // did it remember? (go review serial console)
}
// =============================================================
// verify painting a trail of bread crumbs (locations)
void verifyBreadCrumbTrail2() {
  Serial.print("-------- verifyBreadCrumbTrail2() at line ");
  Serial.println(__LINE__);

  // initialize the canvas to draw on
  gridView.startScreen();   // clear and draw normal screen
  txtTest.dirty = true;     // paint big "Test" in upper left
  txtTest.print();

  model->clearHistory();
  generateSineWave(model);   // fill GPS model with known test data

  Serial.println(". History as known by verifyBreadCrumbTrail2()...");
  model->dumpHistoryGPS();   // did it remember? (go review serial console)
  gridView.updateScreen();   // does it look like a sine wave? (go look at TFT display)
}
// =============================================================
// save GPS route to non-volatile memory
void verifySaveTrail() {
  Serial.print("-------- verifySaveTrail() at line ");
  Serial.println(__LINE__);

  Model testModel;   // create an instance of the model for this test

  generateSineWave(&testModel);   // generate known data to be saved

  // this is automatically saved to non-volatile memory by the model->remember()
}
// =============================================================
// restore GPS route from non-volatile memory
void verifyRestoreTrail() {
  Serial.print("-------- verifyRestoreTrail() at line ");
  Serial.println(__LINE__);
  Serial.println("todo");
}
// =============================================================
// deriving grid square from lat-long coordinates
void verifyDerivingGridSquare() {
  Serial.print("-------- verifyDerivingGridSquare() at line ");
  Serial.println(__LINE__);

  // Expected values from: https://www.movable-type.co.uk/scripts/latlong.html
  //              expected     lat        long
  testCalcLocator6("CN87us", 47.753000, -122.28470);    // read console log for failure messages
  testCalcLocator6("CN85uk", 45.4231, -122.2847);       //
  testCalcLocator6("EM66pd", 36.165926, -86.723285);    // +,-
  testCalcLocator6("OF86cx", -33.014673, 116.230695);   // -,+
  testCalcLocator6("FD54oq", -55.315349, -68.794971);   // -,-
  testCalcLocator6("PM85ge", 35.205535, 136.565790);    // +,+
  //              expected     lat        long
  testCalcLocator8("CN87us00", 47.753000, -122.28470);    // read console log for failure messages
  testCalcLocator8("CN85uk00", 45.4231, -122.2847);       //
  testCalcLocator8("EM66pd00", 36.165926, -86.723285);    // +,-
  testCalcLocator8("OF86cx00", -33.014673, 116.230695);   // -,+
  testCalcLocator8("FD54oq00", -55.315349, -68.794971);   // -,-
  testCalcLocator8("PM85ge00", 35.205535, 136.565790);    // +,+
}
// =============================================================
// verify computing distance
void verifyComputingDistance() {
  Serial.print("-------- verifyComputingDistance() at line ");
  Serial.println(__LINE__);

  //              expected    fromLat     toLat
  testDistanceLat(30.1, 47.56441, 48.00000);   // from home to north, 48.44 km = 30.10 miles
  testDistanceLat(39.0, 47.56441, 47.00000);   //  "    "   "  south, 62.76 km = 39.00 miles
  //              expected   lat     fromLong    toLong
  testDistanceLong(13.2, 47.7531, -122.2845, -122.0000);    //  "    "   "  east,  x.xx km =  x.xx miles
  testDistanceLong(79.7, 47.7531, -122.2845, -124.0000);    //  "    "   "  west,  xx.x km = xx.xx miles
  testDistanceLong(52.9, 67.5000, -158.0000, -156.0000);    // width of BP17 Alaska, 85.1 km = 52.88 miles
  testDistanceLong(93.4, 47.5000, -124.0000, -122.0000);    // width of CN87 Seattle, 150.2 km = 93.33 miles
  testDistanceLong(113.0, 35.5000, -116.0000, -118.0000);   // width of DM15 California is >100 miles, 181 km = 112.47 miles
  testDistanceLong(138.0, 0.5000, -80.0000, -78.0000);      // width of FJ00 Ecuador is the largest possible, 222.4 km = 138.19 miles
}
// =============================================================
// verify finding grid lines on E and W
void verifyComputingGridLines() {
  Serial.print("-------- verifyComputingGridLines() at line ");
  Serial.println(__LINE__);

  //                  expected  fromLongitude
  testNextGridLineEast(-122.0, -122.2836);
  testNextGridLineWest(-124.0, -122.2836);

  testNextGridLineEast(-120.0, -121.8888);
  testNextGridLineWest(-122.0, -121.8888);

  testNextGridLineEast(14.0, 12.3456);
  testNextGridLineWest(12.0, 12.3456);
}
// =============================================================
void countDown(int iSeconds) {
  Serial.print("Counting ");
  Serial.print(iSeconds);
  Serial.println(" sec...");
  setFontSize(0);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
  for (int ii = iSeconds; ii > 0; ii--) {
    tft.setCursor(2, gScreenHeight - 16);
    tft.print(" Wait ");
    tft.print(ii);
    tft.print(" ");
    delay(1000);
  }
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

  verifyCalcTimeDiff();         // verify human-friendly time intervals
  countDown(5);                 //
  // verifyWritingProportionalFont();   // verify writing proportional font
  verifyMorseCode();            // verify Morse code
  verifySaveRestoreVolume();    // verify save/restore an integer setting in SDRAM
  countDown(5);                 //
  verifySaveRestoreArray();     // verify save/restore an array in SDRAM
  countDown(5);                 //
  // verifySaveRestoreGPSModel();   // verify save/restore GPS model state in SDRAM
  // countDown(5);                  //
  verifyBreadCrumbs();          // verify pushpins near the four corners
  countDown(5);                 //
  verifyBreadCrumbTrail1();     // verify painting the bread crumb trail
  countDown(5);                 //
  verifyBreadCrumbTrail2();     // verify painting the bread crumb trail
  countDown(5);                 //
  // verifySaveTrail();         // save GPS route to non-volatile memory
  // countDown(5);              //
  // verifyRestoreTrail();      // restore GPS route from non-volatile memory
  // countDown(5);              //
  verifyDerivingGridSquare();   // verify deriving grid square from lat-long coordinates
  countDown(5);                 //
  verifyComputingDistance();    // verify computing distance
  verifyComputingGridLines();   // verify finding grid lines on E and W
  countDown(5);                 // give user time to inspect display appearance for unit test problems

  model->clearHistory();   // clean up our mess after unit test

  Serial.print("-------- End Unit Test at line ");
  Serial.println(__LINE__);
}
