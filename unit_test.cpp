/*
 * File: unit_test.cpp
 * 
 */

#include <arduino.h>
#include "constants.h"              // Griduino constants and colors
#ifdef RUN_UNIT_TESTS
#include "Adafruit_GFX.h"           // Core graphics display library
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "morse_dac.h"              // Morse code
#include "save_restore.h"           // Configuration data in nonvolatile RAM
#include "model.cpp"                // Class Model (for model-view-controller)
#include "TextField.h"              // Optimize TFT display text for proportional fonts

// ----- extern
float nextGridLineEast(float longitudeDegrees);       // Griduino.ino
float nextGridLineWest(float longitudeDegrees);       // Griduino.ino
String calcLocator(double lat, double lon);           // Griduino.ino
String calcDistanceLat(double fromLat, double toLat); // Griduino.ino
String calcDistanceLong(double lat, double fromLong, double toLong);  // Griduino.ino
void initFontSizeBig();                     // Griduino.ino
void initFontSizeSmall();                   // Griduino.ino
void initFontSizeSystemSmall();             // Griduino.ino
void drawProportionalText(int ulX, int ulY, String prevText, String newText, bool dirty);
void clearScreen();                         // Griduino.ino

void updateGridScreen();                    // view_grid.cpp
void startGridScreen();                     // view_grid.cpp

// ----- globals
extern Adafruit_ILI9341 tft;
extern int gCharWidth, gCharHeight;         // character cell size for TextSize(n)
extern DACMorseSender dacMorse;             // Morse code
extern Model model;

TextField txtTest("test", 1,21, ILI9341_WHITE);

// =============================================================
// Testing "grid helper" routines in Griduino.ino
void testNextGridLineEast(float fExpected, double fLongitude) {
  // unit test helper for finding grid line crossings
  float result = nextGridLineEast(fLongitude);
  //~Serial.print("Grid Crossing East: given = "); //~Serial.print(fLongitude);
  //~Serial.print(", expected = "); //~Serial.print(fExpected);
  //~Serial.print(", result = "); //~Serial.print(result);
  if (result == fExpected) {
    //~Serial.println("");
  }
  else {
    //~Serial.println(" <-- Unequal");
  }
}
void testNextGridLineWest(float fExpected, double fLongitude) {
  // unit test helper for finding grid line crossings
  float result = nextGridLineWest(fLongitude);
  //~Serial.print("Grid Crossing West: given = "); //~Serial.print(fLongitude);
  //~Serial.print(", expected = "); //~Serial.print(fExpected);
  //~Serial.print(", result = "); //~Serial.print(result);
  if (result == fExpected) {
    //~Serial.println("");
  }
  else {
    //~Serial.println(" <-- Unequal");
  }
}
void testCalcLocator(String sExpected, double lat, double lon) {
  // unit test helper function to display results
  String sResult;
  sResult = calcLocator(lat, lon);
  //~Serial.print("Test: expected = "); //~Serial.print(sExpected);
  //~Serial.print(", gResult = "); //~Serial.print(sResult);
  if (sResult == sExpected) {
    //~Serial.println("");
  }
  else {
    //~Serial.println(" <-- Unequal");
  }
}

// =============================================================
// Testing "distance helper" routines in Griduino.cpp
void testDistanceLat(String sExpected, double fromLat, double toLat) {
  // unit test helper function to calculate N-S distances
  String sResult = calcDistanceLat(fromLat, toLat);
  Serial.print("N-S Distance Test: expected = "); Serial.print(sExpected);
  Serial.print(", result = "); Serial.println(sResult);
}
void testDistanceLong(String sExpected, double lat, double fromLong, double toLong) {
  // unit test helper function to calculate E-W distances
  String sResult = calcDistanceLong(lat, fromLong, toLong);
  Serial.print("E-W Distance Test: expected = "); Serial.print(sExpected);
  Serial.print(", result = "); Serial.print(sResult);
  if (sResult == sExpected) {
    Serial.println("");
  }
  else {
    Serial.println(" <-- Unequal");
  }
}
// =============================================================
// verify Morse Code
void verifyMorseCode() {
  dacMorse.setup();
  dacMorse.dump();
  
  Serial.print("\nStarting dits\n");
  for (int ii=1; ii<=40; ii++) {
    //~Serial.print(ii); //~Serial.print(" ");
    if (ii%10 == 0) //~Serial.print("\n");
    dacMorse.send_dit();
  }
  Serial.print("Finished dits\n");
  dacMorse.send_word_space();
  
  // ----- test dit-dah
  Serial.print("\nStarting dit-dah\n");
  for (int ii=1; ii<=20; ii++) {
    //~Serial.print(ii); //~Serial.print(" ");
    if (ii%10 == 0) //~Serial.print("\n");
    dacMorse.send_dit();
    dacMorse.send_dah();  
  }
  dacMorse.send_word_space();
  Serial.print("Finished dit-dah\n");
}
// =============================================================
// verify Save/Restore Volume settings in SDRAM
void verifySaveRestoreVolume() {
  #define TEST_CONFIG_FILE    CONFIG_FOLDER "/test.cfg"   // strictly 8.3 names
  #define TEST_CONFIG_VERSION "Test v01"
  #define TEST_CONFIG_VALUE   5

  // sample data to read/write
  SaveRestore configWrite(TEST_CONFIG_FILE, TEST_CONFIG_VERSION, TEST_CONFIG_VALUE);
  SaveRestore configRead(TEST_CONFIG_FILE, TEST_CONFIG_VERSION);

  if (configWrite.writeConfig()) {  // test writing data to SDRAM -- be sure to watch serial console log
    Serial.println("Success, config stored to SDRAM");
  } else {
    Serial.println("ERROR! Unable to save config to SDRAM");
  }

  if (configRead.readConfig()) {    // test reading same data back from SDRAM
    Serial.println("Success, configuration restored from SDRAM");
  } else {
    Serial.println("ERROR! Unable to restore from SDRAM");
  }
}
// =============================================================
// verify save/restore GPS model state in SDRAM
void verifySaveRestoreGPSModel() {
  #define TEST_GPS_STATE_FILE    CONFIG_FOLDER "/test_gps.cfg"   // strictly 8.3 naming
  #define TEST_GPS_STATE_VERSION "Test v01"
  Serial.print("verifySaveRestoreGPSModel() at line "); Serial.println(__LINE__);

  // sample data to read/write
  Model gpsmodel;

  if (gpsmodel.save()) {
    Serial.println("Success, GPS model stored to SDRAM");
  } else {
    Serial.println("ERROR! Failed to save GPS model to SDRAM");
  }

  if (gpsmodel.restore()) {    // test reading same data back from SDRAM
    Serial.println("Success, GPS model restored from SDRAM");
  } else {
    Serial.println("ERROR! Failed to retrieve GPS model from SDRAM");
  }
}
// =============================================================
// verify writing proportional font
void verifyWritingProportionalFont() {
  // visual test: if this code works correctly, each string will exactly erase the previous one
  Serial.print("verifyWritingProportionalFont() at line "); Serial.println(__LINE__);

  initFontSizeBig();
  int waitTime = 400;    // fast=10 msec, easy-to-read=1000 msec

  // visual test: check that prev text is fully erased when new text written
  int middleRowY = tft.height()/2;
  drawProportionalText(24, middleRowY, String("MM88"),   String("abc"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("abc"),    String("defghi"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("defghi"), String("jklmnop"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("jklmnop"),String("xyz"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("xyz"),    String("qrstu"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("qrstu"),  String("vwxyz0"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("vwxyz0"), String(""), true);
}
// =============================================================
// verify painting individual bread crumbs (locations)
void verifyBreadCrumbs() {
  // plotting a series of pushpins (bread crumb trail)
  Serial.print("verifyBreadCrumbs() at line "); Serial.println(__LINE__);
  
  // move the model to known location (CN87)
  model.gsGridName = "CN87";
  model.gLatitude = 47.737451;    // CN87
  model.gLongitude = -122.274711; // CN87

  // initialize the canvas that we will draw upon
  startGridScreen();              // clear and draw normal screen
  txtTest.print();
  txtTest.dirty = true;
  txtTest.print();

  delay(3000);    // time for serial monitor to connect, and human to look at TFT display

  const int gBoxWidth = 180;          // same as in view_grid.cpp
  const int gBoxHeight = 160;         // same as in view_grid.cpp

  float xPixelsPerDegree = gBoxWidth / gridWidthDegrees;    // grid square = 2.0 degrees wide E-W
  float yPixelsPerDegree = gBoxHeight / gridHeightDegrees;  // grid square = 1.0 degrees high N-S

  // ----- plot each corner, starting upper left, moving clockwise
  model.gLatitude = 48.0 - 0.1;     // upper left
  model.gLongitude = -124.0 + 0.1;
  updateGridScreen();
  delay(500);

  model.gLatitude = 48.0 - 0.1;     // upper right
  model.gLongitude = -122.0 - 0.1;
  updateGridScreen();
  delay(500);

  model.gLatitude = 47.0 + 0.1;     // lower right
  model.gLongitude = -122.0 - 0.1;
  updateGridScreen();
  delay(500);

  model.gLatitude = 47.0 + 0.1;     // lower left
  model.gLongitude = -124.0 + 0.1;
  updateGridScreen();
  delay(500);
}
// =============================================================
// verify painting a trail of bread crumbs (locations)
void verifyBreadCrumbTrail1() {
  Serial.print("verifyBreadCrumbTrail1() at line "); Serial.println(__LINE__);
  
  // initialize the canvas to draw on
  startGridScreen();              // clear and draw normal screen
  txtTest.dirty = true;
  txtTest.print();

  // test 2: loop through locations that cross this grid
  float lat = 47.0 - 0.2;         // 10% outside of CN87
  float lon = -124.0 - 0.1;
  int steps = model.numHistory;   // number of loops
  float stepsize = 1.0 / 250.0;   // number of degrees to move each loop

  model.clearHistory();
  for (int ii = 0; ii < steps; ii++) {
    PointGPS item{ model.gLatitude = lat + (ii * stepsize),         // "plus" goes upward (north)
                   model.gLongitude = lon + (ii * stepsize * 5/4)}; // "plus" goes rightward (east)
    model.remember( item, GPS.hour, GPS.minute, GPS.seconds );
  }

  //dumpHistory();              // did it remember? dump history to monitor
  updateGridScreen();
  model.clearHistory();     // clean up so it is not re-displayed by main program
}
// =============================================================
// verify painting a trail of bread crumbs (locations)
void verifyBreadCrumbTrail2() {
  Serial.print("verifyBreadCrumbTrail2() at line "); Serial.println(__LINE__);
  
  // initialize the canvas to draw on
  startGridScreen();              // clear and draw normal screen
  txtTest.dirty = true;
  txtTest.print();

  // fill history table with a sine wave
  double startLat = 47.5;              // 10% outside of CN87
  double startLong = -124.0 - 0.6;
  int steps = model.numHistory;   // number of loops
  double stepsize = (125.5 - 121.5) / steps;   // degrees longitude to move each loop
  double amplitude = 0.65;         // degrees latitude, maximum sine wave

  model.clearHistory();
  for (int ii = 0; ii < steps; ii++) {
    float longitude = startLong + (ii * stepsize);
    float latitude = startLat + amplitude*sin(longitude * 150 / degreesPerRadian);
    PointGPS location{latitude,longitude};
    model.remember( location, GPS.hour, GPS.minute, GPS.seconds );
  }
  //model.dumpHistory();             // did it remember? dump history to console
  updateGridScreen();
  model.clearHistory();     // clean up so it is not re-displayed by main program
}
// =============================================================
// deriving grid square from lat-long coordinates
void verifyDerivingGridSquare() {
  // Expected values from: https://www.movable-type.co.uk/scripts/latlong.html
  //              expected     lat        long
  testCalcLocator("CN87us",  47.753000, -122.28470);  // user must read console log for failure messages
  testCalcLocator("EM66pd",  36.165926, -86.723285);  // +,-
  testCalcLocator("OF86cx", -33.014673, 116.230695);  // -,+
  testCalcLocator("FD54oq", -55.315349, -68.794971);  // -,-
  testCalcLocator("PM85ge",  35.205535, 136.565790);  // +,+
}
// =============================================================
// verify computing distance
void verifyComputingDistance() {
  //              expected    fromLat     toLat
  testDistanceLat("30.1",    47.56441,   48.00000);         // from home to north, 48.44 km = 30.10 miles
  testDistanceLat("39.0",    47.56441,   47.00000);         //  "    "   "  south, 62.76 km = 39.00 miles
  //              expected   lat     fromLong    toLong
  testDistanceLong("13.2", 47.7531, -122.2845, -122.0000);  //  "    "   "  east,  x.xx km =  x.xx miles
  testDistanceLong("79.7", 47.7531, -122.2845, -124.0000);  //  "    "   "  west,  xx.x km = xx.xx miles
  testDistanceLong("52.9", 67.5000, -158.0000, -156.0000);  // width of BP17 Alaska, 85.1 km = 52.88 miles
  testDistanceLong("93.4", 47.5000, -124.0000, -122.0000);  // width of CN87 Seattle, 150.2 km = 93.33 miles
  testDistanceLong("113 ", 35.5000, -116.0000, -118.0000);  // width of DM15 California is >100 miles, 181 km = 112.47 miles
  testDistanceLong("138 ",  0.5000,  -80.0000,  -78.0000);  // width of FJ00 Ecuador is the largest possible, 222.4 km = 138.19 miles
}
// =============================================================
// verify finding gridlines on E and W
void verifyComputingGridLines() {
  //                  expected  fromLongitude
  testNextGridLineEast(-122.0, -122.2836);
  testNextGridLineWest(-124.0, -122.2836);

  testNextGridLineEast(-120.0, -121.8888);
  testNextGridLineWest(-122.0, -121.8888);

  testNextGridLineEast( 14.0, 12.3456);
  testNextGridLineWest( 12.0, 12.3456);
}
// =============================================================
void countDown(int iSeconds) {
  initFontSizeSystemSmall();
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
  for (int ii=iSeconds; ii>0; ii--) {
    tft.setCursor(2, gScreenHeight-gCharHeight-2);
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
  initFontSizeBig();
  drawProportionalText(gCharWidth, gCharHeight, String(""), String("--Unit Test--"), true);

  initFontSizeSystemSmall();
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(0, tft.height() - gCharHeight); // move to bottom row
  tft.print("  --Open console monitor to see unit test results--");
  delay(1000);

  verifyMorseCode();                // verify Morse code
  verifySaveRestoreVolume();        // verify save/restore Volume settings in SDRAM
  verifySaveRestoreGPSModel();      // verify save/restore GPS model state in SDRAM
  verifyWritingProportionalFont();  // verify writing proportional font
  
  verifyBreadCrumbs();         countDown(4);   // verify pushpins near the four corners
  verifyBreadCrumbTrail1();    countDown(5);  // verify painting the bread crumb trail
  verifyBreadCrumbTrail2();    countDown(10);  // verify painting the bread crumb trail

  verifyDerivingGridSquare(); countDown(4);   // verify deriving grid square from lat-long coordinates
  verifyComputingDistance();        // verify computing distance
  verifyComputingGridLines();       // verify finding gridlines on E and W

  countDown(10);                    // give user time to inspect display appearance for unit test problems
}
#endif // RUN_UNIT_TESTS
