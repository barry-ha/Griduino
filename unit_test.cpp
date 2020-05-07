/*
 * File: unit_test.cpp
 * 
 */

#include <arduino.h>
#include "constants.h"            // Griduino constants and colors
#ifdef RUN_UNIT_TESTS
#include "Adafruit_GFX.h"         // Core graphics display library
#include "Adafruit_ILI9341.h"     // TFT color display library
#include "morse_dac.h"            // Morse code
#include "save_restore.h"         // Configuration data in nonvolatile RAM
#include "model.cpp"              // Class Model (for model-view-controller)

// ----- extern
float nextGridLineEast(float longitudeDegrees);       // Griduino.ino
float nextGridLineWest(float longitudeDegrees);       // Griduino.ino
String calcLocator(double lat, double lon);           // Griduino.ino
String calcDistanceLat(double fromLat, double toLat); // Griduino.ino
String calcDistanceLong(double lat, double fromLong, double toLong);  // Griduino.ino
void initFontSizeBig();         // Griduino.ino
void initFontSizeSystemSmall(); // Griduino.ino
void drawProportionalText(int ulX, int ulY, String prevText, String newText, bool dirty);
void updateGridScreen();                    // view_grid.cpp
void startGridScreen();
//bool onTouchGrid(Point touch);

// ----- globals
extern Adafruit_ILI9341 tft;
extern int gCharWidth, gCharHeight;         // character cell size for TextSize(n)
extern DACMorseSender dacMorse;             // Morse code
extern Model model;

// ----- Testing "grid helper" routines in Griduino.ino
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

// ----- Testing "distance helper" routines in Griduino.cpp
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

// ----- Testing display routines in Griduino.ino
// ========== unit tests ========================
void runUnitTest() {
  tft.fillScreen(ILI9341_BLACK);

  // ----- announce ourselves
  initFontSizeBig();
  drawProportionalText(gCharWidth, gCharHeight, String(""), String("--Unit Test--"), true);

  initFontSizeSystemSmall();
  tft.setCursor(0, tft.height() - gCharHeight);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_YELLOW);
  tft.print("  --Open console monitor to see unit test results--");

  // ----- verify Morse code
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

  // ----- test save/restore Volume settings in SDRAM
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

  // ----- test save/restore GPS model state in SDRAM
  #define TEST_GPS_STATE_FILE    CONFIG_FOLDER "/test_gps.cfg"   // strictly 8.3 naming
  #define TEST_GPS_STATE_VERSION "Test v01"
  
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
  
  // ----- writing proportional font
  // visual test: if this code works correctly, each string will exactly erase the previous one
  initFontSizeBig();
  int waitTime = 400;    // fast=10 msec, easy-to-read=1000 msec

  // test that prev text is fully erased when new text written
  int middleRowY = tft.height()/2;
  drawProportionalText(24, middleRowY, String("MM88"),   String("abc"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("abc"),    String("defghi"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("defghi"), String("jklmnop"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("jklmnop"),String("xyz"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("xyz"),    String("qrstu"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("qrstu"),  String("vwxyz0"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("vwxyz0"), String(""), true);

  // plotting a series of pushpins (bread crumb trail)
  startGridScreen();        // box outline around grid
  
  float lat = 47.1;         // 10% inside of CN87
  float lon = -124.2;
  int steps = 600;          // number of loops
  float stepsize = 1.0 / 200.0; // number of degrees to move each loop
  for (int ii = 0; ii < steps; ii++) {
    model.gLatitude = lat + ii * stepsize;
    model.gLongitude = lon + ii * stepsize * 5/4;
    updateGridScreen();
    //delay(10);    // commented out to save boot time
  }
  delay(3000);    // commented out to save boot time

  // ----- deriving grid square from lat-long coordinates
  // Expected values from: https://www.movable-type.co.uk/scripts/latlong.html
  //              expected     lat        long
  testCalcLocator("CN87us",  47.753000, -122.28470);  // user must read console log for failure messages
  testCalcLocator("EM66pd",  36.165926, -86.723285);  // +,-
  testCalcLocator("OF86cx", -33.014673, 116.230695);  // -,+
  testCalcLocator("FD54oq", -55.315349, -68.794971);  // -,-
  testCalcLocator("PM85ge",  35.205535, 136.565790);  // +,+

  // ----- computing distance
  //              expected    fromLat     toLat
  testDistanceLat("30.1",    47.56441,   48.00000);     // from home to north, 48.44 km = 30.10 miles
  testDistanceLat("39.0",    47.56441,   47.00000);     //  "    "   "  south, 62.76 km = 39.00 miles
  //              expected   lat     fromLong    toLong
  testDistanceLong("13.2", 47.7531, -122.2845, -122.0000);   //  "    "   "  east,  x.xx km =  x.xx miles
  testDistanceLong("79.7", 47.7531, -122.2845, -124.0000);   //  "    "   "  west,  xx.x km = xx.xx miles
  testDistanceLong("52.9", 67.5000, -158.0000, -156.0000);   // width of BP17 Alaska, 85.1 km = 52.88 miles
  testDistanceLong("93.4", 47.5000, -124.0000, -122.0000);   // width of CN87 Seattle, 150.2 km = 93.33 miles
  testDistanceLong("113 ", 35.5000, -116.0000, -118.0000);   // width of DM15 California is >100 miles, 181 km = 112.47 miles
  testDistanceLong("138 ",  0.5000,  -80.0000,  -78.0000);   // width of FJ00 Ecuador is the largest possible, 222.4 km = 138.19 miles

  // ----- finding gridlines on E and W
  //                  expected  fromLongitude
  testNextGridLineEast(-122.0, -122.2836);
  testNextGridLineWest(-124.0, -122.2836);

  testNextGridLineEast(-120.0, -121.8888);
  testNextGridLineWest(-122.0, -121.8888);

  testNextGridLineEast( 14.0, 12.3456);
  testNextGridLineWest( 12.0, 12.3456);

  delay(4000);                      // give user time to inspect display appearance for unit test problems

}
#endif // RUN_UNIT_TESTS
