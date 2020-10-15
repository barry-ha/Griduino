/*
 * File: unit_test.cpp
 * 
 */

#include <arduino.h>
#include "constants.h"              // Griduino constants and colors
#ifdef RUN_UNIT_TESTS
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "morse_dac.h"              // Morse code
#include "save_restore.h"           // Configuration data in nonvolatile RAM
#include "model.cpp"                // Class Model (for model-view-controller)
#include "TextField.h"              // Optimize TFT display text for proportional fonts
#include "view.h"                   // Base class for all views

// ========== extern ==================================
extern float nextGridLineEast(float longitudeDegrees);       // Griduino.ino
extern float nextGridLineWest(float longitudeDegrees);       // Griduino.ino
extern void calcLocator(char* result, double lat, double lon, int precision); // Griduino.ino
extern void setFontSize(int font);             // Griduino.ino
extern void clearScreen();                     // Griduino.ino

// ----- globals
extern Adafruit_ILI9341 tft;
extern DACMorseSender dacMorse;         // Morse code
extern Model* model;
extern ViewGrid gridView;               // Griduino.ino

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
  Serial.print("Grid Crossing West: given = "); Serial.print(fLongitude);
  Serial.print(", expected = "); Serial.print(fExpected);
  Serial.print(", result = "); Serial.print(result);
  if (result == fExpected) {
    Serial.println("");
  }
  else {
    Serial.println(" <-- Unequal");
  }
}
void testCalcLocator(const char* sExpected, double lat, double lon) {
  // unit test helper function to display results
  char sResult[7];      // strlen("CN87us") = 6
  calcLocator(sResult, lat, lon, 6);
  Serial.print("Test: expected = "); Serial.print(sExpected);
  Serial.print(", gResult = "); Serial.print(sResult);
  if (strcmp(sResult, sExpected) == 0) {
    Serial.println("");
  }
  else {
    Serial.println(" <-- Unequal");
  }
}

// =============================================================
// Testing "distance helper" routines in Griduino.cpp
void testDistanceLat(double expected, double fromLat, double toLat) {
  // unit test helper function to calculate N-S distances
  double distance = model->calcDistanceLat(fromLat, toLat);
  Serial.print("N-S Distance Test: expected = "); Serial.print(expected,2);
  Serial.print(", result = "); Serial.println(distance, 4);
}
void testDistanceLong(double expected, double lat, double fromLong, double toLong) {
  // unit test helper function to calculate E-W distances
  double result = model->calcDistanceLong(lat, fromLong, toLong);
  Serial.print("E-W Distance Test: expected = "); Serial.print(expected,2);
  Serial.print(", result = "); Serial.print(result,2);
  if ((expected/1.01)<=result && result <= (expected*1.01)) {
    Serial.println("");
  }
  else {
    Serial.println(" <-- Unequal");
  }
}
// =============================================================
// verify Morse Code
void verifyMorseCode() {
  Serial.print("-------- verifyMorseCode() at line "); Serial.println(__LINE__);
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
  Serial.print("-------- verifySaveRestoreVolume() at line "); Serial.println(__LINE__);

  #define TEST_CONFIG_FILE    CONFIG_FOLDER "/test.cfg"   // strictly 8.3 names
  #define TEST_CONFIG_VERSION "Test v02"
  #define TEST_CONFIG_VALUE   5
  int writeValue = TEST_CONFIG_VALUE;
  int readValue = 0;    // different from "writeValue"

  // sample data to read/write
  SaveRestore configWrite(TEST_CONFIG_FILE, TEST_CONFIG_VERSION);
  SaveRestore configRead(TEST_CONFIG_FILE, TEST_CONFIG_VERSION);

  if (configWrite.writeConfig( (byte*) &writeValue, sizeof(writeValue))) {  // test writing data to SDRAM -- be sure to watch serial console log
    Serial.println("Success, integer stored to SDRAM");
  } else {
    Serial.println("ERROR! Unable to save integer to SDRAM");
  }

  if (configRead.readConfig( (byte*) &readValue, sizeof(readValue))) {    // test reading same data back from SDRAM
    Serial.println("Success, integer restored from SDRAM");
    if (readValue == writeValue) {
      Serial.println("Success, correct value was restored");
    } else {
      Serial.println("ERROR! The value restored did NOT match the value saved");
    }
  } else {
    Serial.println("ERROR! Unable to restore integer from SDRAM");
  }
  configWrite.remove( TEST_CONFIG_FILE );
}
// =============================================================
void verifySaveRestoreArray() {
  Serial.print("-------- verifySaveRestoreArray() at line "); Serial.println(__LINE__);

  #define TEST_ARRAY_FILE   CONFIG_FOLDER "/testarry.cfg"
  #define TEST_ARRAY_VERS   "Array v02"
  int iData[21];
  const int numData = sizeof(iData)/sizeof(int);
  for (int ii=0; ii<numData; ii++) {
    iData[ii] = ii;
  }
  
  SaveRestore writeArray(TEST_ARRAY_FILE, TEST_ARRAY_VERS);
  SaveRestore readArray(TEST_ARRAY_FILE, TEST_ARRAY_VERS);

  if (writeArray.writeConfig( (byte*) &iData, sizeof(iData))) {  // test writing data to SDRAM -- be sure to watch serial console log
    Serial.println("Success, array stored to SDRAM");
  } else {
    Serial.println("ERROR! Failed to save array to SDRAM");
  }

  int iResult[numData];
  if (readArray.readConfig( (byte*) &iResult, sizeof(iResult))) {    // test reading same data back from SDRAM
    Serial.println("Success, array retrieved from SDRAM");
    bool success = true;    // assume successful comparisons
    for (int ii=0; ii<numData; ii++) {
      if (iResult[ii] != iData[ii]) {
        success = false;
        char temp[100];
        snprintf(temp, sizeof(temp), 
                "ERROR! Array index (%d) value restored (%d) did NOT match the value saved (%d)", 
                                     ii,             iResult[ii],                    iData[ii]);
        Serial.println(temp);
      }
    }
    if (success) {
      Serial.println("Success, all values correct in array restored from SDRAM");
    }
  } else {
    Serial.println("ERROR! Unable to restore from SDRAM");
  }
  writeArray.remove( TEST_ARRAY_FILE );
}
// =============================================================
// verify save/restore GPS model state in SDRAM
void verifySaveRestoreGPSModel() {
  #define TEST_GPS_STATE_FILE   CONFIG_FOLDER "/test_gps.cfg"   // strictly 8.3 naming
  #define TEST_GPS_STATE_VERS   "Test v02"
  Serial.print("-------- verifySaveRestoreGPSModel() at line "); Serial.println(__LINE__);

  Model gpsModel;     // sample data to read/write, a different object than used in model.cpp

  if (gpsModel.save()) {
    Serial.println("Success, GPS model stored to SDRAM");
  } else {
    Serial.print("Unit test error: Failed to save GPS model to SDRAM, line "); Serial.println(__LINE__);
  }

  if (gpsModel.restore()) {    // test reading same data back from SDRAM
    Serial.println("Success, GPS model restored from SDRAM");
  } else {
    Serial.print("Unit test error: Failed to retrieve GPS model from SDRAM, line "); Serial.println(__LINE__);
  }
}
// =============================================================
// verify painting individual bread crumbs (locations)
void verifyBreadCrumbs() {
  // plotting a series of pushpins (bread crumb trail)
  Serial.print("-------- verifyBreadCrumbs() at line "); Serial.println(__LINE__);
  
  // move the model to known location (CN87)
  //model->gsGridName = "CN87";
  model->gLatitude = 47.737451;    // CN87
  model->gLongitude = -122.274711; // CN87

  // initialize the canvas that we will draw upon
  gridView.startScreen();               // clear and draw normal screen
  txtTest.print();
  txtTest.dirty = true;
  txtTest.print();

  delay(3000);    // time for serial monitor to connect, and human to look at TFT display

  float xPixelsPerDegree = gBoxWidth / gridWidthDegrees;    // grid square = 2.0 degrees wide E-W
  float yPixelsPerDegree = gBoxHeight / gridHeightDegrees;  // grid square = 1.0 degrees high N-S

  // ----- plot each corner, starting upper left, moving clockwise
  model->gLatitude = 48.0 - 0.1;     // upper left
  model->gLongitude = -124.0 + 0.1;
  gridView.updateScreen();
  delay(500);

  model->gLatitude = 48.0 - 0.1;     // upper right
  model->gLongitude = -122.0 - 0.1;
  gridView.updateScreen();
  delay(500);

  model->gLatitude = 47.0 + 0.1;     // lower right
  model->gLongitude = -122.0 - 0.1;
  gridView.updateScreen();
  delay(500);

  model->gLatitude = 47.0 + 0.1;     // lower left
  model->gLongitude = -124.0 + 0.1;
  gridView.updateScreen();
  delay(500);
}
// =============================================================
// verify painting a trail of bread crumbs (locations)
void verifyBreadCrumbTrail1() {
  Serial.print("-------- verifyBreadCrumbTrail1() at line "); Serial.println(__LINE__);
  
  // initialize the canvas to draw on
  gridView.startScreen();            // clear and draw normal screen
  txtTest.dirty = true;
  txtTest.print();

  // test 2: loop through locations that cross this grid
  float lat = 47.0 - 0.2;         // 10% outside of CN87
  float lon = -124.0 - 0.1;
  int steps = model->numHistory;   // number of loops
  float stepsize = 1.0 / 250.0;   // number of degrees to move each loop

  model->clearHistory();
  for (int ii = 0; ii < steps; ii++) {
    PointGPS item{ model->gLatitude = lat + (ii * stepsize),         // "plus" goes upward (north)
                   model->gLongitude = lon + (ii * stepsize * 5/4)}; // "plus" goes rightward (east)
    model->remember( item, GPS.hour, GPS.minute, GPS.seconds );
  }

  //dumpHistory();              // did it remember? dump history to monitor
  gridView.updateScreen();
  model->clearHistory();     // clean up so it is not re-displayed by main program
}
// =============================================================
// GPS test helper
void generateSineWave(Model* pModel) {

  // fill history table with a sine wave
  double startLat = 47.5;             // 10% outside of CN87
  double startLong = -124.0 - 0.6;
  int steps = pModel->numHistory;     // number of loops
  double stepsize = (125.5 - 121.5)/steps;   // degrees longitude to move each loop
  double amplitude = 0.65;            // degrees latitude, maximum sine wave

  for (int ii = 0; ii < steps; ii++) {
    float longitude = startLong + (ii * stepsize);
    float latitude = startLat + amplitude*sin(longitude * 150 / degreesPerRadian);
    PointGPS location{ latitude, longitude };
    pModel->remember( location, GPS.hour, GPS.minute, GPS.seconds );
  }
  //Serial.println("---History as known by generateSineWave()...");
  //pModel->dumpHistory();            // did it remember? (go review serial console)
}
// =============================================================
// verify painting a trail of bread crumbs (locations)
void verifyBreadCrumbTrail2() {
  Serial.print("-------- verifyBreadCrumbTrail2() at line "); Serial.println(__LINE__);
  
  // initialize the canvas to draw on
  gridView.startScreen();           // clear and draw normal screen
  txtTest.dirty = true;             // paint big "Test" in upper left
  txtTest.print();

  model->clearHistory();
  generateSineWave(model);          // fill GPS model with known test data

  Serial.println(". History as known by verifyBreadCrumbTrail2()...");
  model->dumpHistory();             // did it remember? (go review serial console)
  gridView.updateScreen();          // does it look like a sine wave? (go look at TFT display)
}
// =============================================================
// save GPS route to non-volatile memory
void verifySaveTrail() {
  Serial.print("-------- verifySaveTrail() at line "); Serial.println(__LINE__);

  Model testModel;                // create an instance of the model for this test
  
  generateSineWave(&testModel);   // generate known data to be saved

  // this is automatically saved to non-volatile memory by the model->remember()
}
// =============================================================
// restore GPS route from non-volatile memory
void verifyRestoreTrail() {
  Serial.print("-------- verifyRestoreTrail() at line "); Serial.println(__LINE__);
  Serial.println("todo");
}
// =============================================================
// deriving grid square from lat-long coordinates
void verifyDerivingGridSquare() {
  Serial.print("-------- verifyDerivingGridSquare() at line "); Serial.println(__LINE__);

  // Expected values from: https://www.movable-type.co.uk/scripts/latlong.html
  //              expected     lat        long
  testCalcLocator("CN87us",  47.753000, -122.28470);  // read console log for failure messages
  testCalcLocator("CN85uk",  45.4231,   -122.2847 );  //
  testCalcLocator("EM66pd",  36.165926, -86.723285);  // +,-
  testCalcLocator("OF86cx", -33.014673, 116.230695);  // -,+
  testCalcLocator("FD54oq", -55.315349, -68.794971);  // -,-
  testCalcLocator("PM85ge",  35.205535, 136.565790);  // +,+
}
// =============================================================
// verify computing distance
void verifyComputingDistance() {
  Serial.print("-------- verifyComputingDistance() at line "); Serial.println(__LINE__);

  //              expected    fromLat     toLat
  testDistanceLat(  30.1,    47.56441,   48.00000);         // from home to north, 48.44 km = 30.10 miles
  testDistanceLat(  39.0,    47.56441,   47.00000);         //  "    "   "  south, 62.76 km = 39.00 miles
  //              expected   lat     fromLong    toLong
  testDistanceLong(  13.2, 47.7531, -122.2845, -122.0000);  //  "    "   "  east,  x.xx km =  x.xx miles
  testDistanceLong(  79.7, 47.7531, -122.2845, -124.0000);  //  "    "   "  west,  xx.x km = xx.xx miles
  testDistanceLong(  52.9, 67.5000, -158.0000, -156.0000);  // width of BP17 Alaska, 85.1 km = 52.88 miles
  testDistanceLong(  93.4, 47.5000, -124.0000, -122.0000);  // width of CN87 Seattle, 150.2 km = 93.33 miles
  testDistanceLong( 113.0, 35.5000, -116.0000, -118.0000);  // width of DM15 California is >100 miles, 181 km = 112.47 miles
  testDistanceLong( 138.0,  0.5000,  -80.0000,  -78.0000);  // width of FJ00 Ecuador is the largest possible, 222.4 km = 138.19 miles
}
// =============================================================
// verify finding gridlines on E and W
void verifyComputingGridLines() {
  Serial.print("-------- verifyComputingGridLines() at line "); Serial.println(__LINE__);

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
  Serial.print("Counting "); Serial.print(iSeconds); Serial.println(" sec...");
  setFontSize(0);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
  for (int ii=iSeconds; ii>0; ii--) {
    tft.setCursor(2, gScreenHeight-16);
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
  tft.print( "--Unit Test--");

  setFontSize(0);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(0, tft.height() - 12); // move to bottom row
  tft.print("  --Open console monitor to see unit test results--");
  delay(1000);

  //verifyMorseCode();                            // verify Morse code
  verifySaveRestoreVolume();    countDown( 5);  // verify save/restore an integer setting in SDRAM
  verifySaveRestoreArray();     countDown(10);  // verify save/restore an array in SDRAM
  verifySaveRestoreGPSModel();  countDown(10);  // verify save/restore GPS model state in SDRAM
  //verifyWritingProportionalFont();              // verify writing proportional font
  
  verifyBreadCrumbs();          countDown(5);   // verify pushpins near the four corners
  verifyBreadCrumbTrail1();     countDown(5);   // verify painting the bread crumb trail
  verifyBreadCrumbTrail2();     countDown(10);  // verify painting the bread crumb trail
  verifySaveTrail();            countDown(10);  // save GPS route to non-volatile memory
  verifyRestoreTrail();         countDown(5);   // restore GPS route from non-volatile memory

  verifyDerivingGridSquare();   countDown(5);   // verify deriving grid square from lat-long coordinates
  verifyComputingDistance();        // verify computing distance
  verifyComputingGridLines();       // verify finding gridlines on E and W

  countDown(10);                    // give user time to inspect display appearance for unit test problems

  model->clearHistory();             // clean up our mess after unit test
  Serial.print("-------- End Unit Test at line "); Serial.println(__LINE__);
}
#endif // RUN_UNIT_TESTS
