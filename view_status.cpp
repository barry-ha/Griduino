/* File: view_stat_screen.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This screen shows most of what the GPS knows about location
            and velocity in text format. Results are updated in real time.

            +----------------------------+
            | GMT:    19:54:14           |...yRow1
            |         Wed, Jan 15        |...yRow2
            | Grid:   CN87us             |...yRow3
            | Alt:    xx.x ft            |...yRow4
            | Satellites: None           |...yRow5
            |   47.0753, -122.2847       |...yRow6
            | Waiting for GPS            |...yRow7?
            +-:-------:------------------+
              labelX  valueX
*/

#include <Arduino.h>
#include "Adafruit_GFX.h"           // Core graphics display library
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors
#include "model.cpp"                // "Model" portion of model-view-controller
#include "TextField.h"              // Optimize TFT display text for proportional fonts

// ========== extern ===========================================
extern Adafruit_ILI9341 tft;        // Griduino.ino
extern int gTextSize;               // no such function as "tft.getTextSize()" so remember it on our own
extern int gCharWidth, gCharHeight; // character cell size for TextSize(n)
extern int gUnitFontWidth, gUnitFontHeight; // character cell size for TextSize(1)
extern Model model;                 // "model" portion of model-view-controller

void initFontSizeBig();             // Griduino.ino
void initFontSizeSmall();           // Griduino.ino
int getOffsetToCenterText(String text); // Griduino.ino

// ============== constants ====================================
// vertical placement of text rows
const int yRow1 = 28;
const int yRow2 = yRow1 + 32;
const int yRow3 = yRow2 + 44;
const int yRow4 = yRow3 + 44;
const int yRow5 = yRow4 + 32;
const int yRow6 = yRow5 + 40;
const int yRow7 = yRow6 + 20;

// color scheme: see constants.h

const int labelX = 8;       // indent labels, slight margin to left edge of screen
const int valueX = 124;     // indent values

// ----- static screen text
const int numLabels = 4;
TextField txtLabels[numLabels] = {
  TextField("GMT:",        labelX, yRow1, cLABEL),
  TextField("Grid:",       labelX, yRow3, cLABEL),
  TextField("Altitude:",   labelX, yRow4, cLABEL),
  TextField("Satellites:", labelX, yRow5, cLABEL),
};

// ----- dynamic screen text
const int numText = 6;
TextField txtValues[numText] = {
  TextField(valueX, yRow1, cVALUE),     // 0. GMT time
  TextField(valueX, yRow2, cVALUE),     // 1. GMT date
  TextField(valueX, yRow3, cHIGHLIGHT), // 2. Grid6, brighter color because "grid" is important
  TextField(valueX, yRow4, cVALUE),     // 3. Altitude
  TextField(valueX, yRow5, cVALUE),     // 4. Satellites
  TextField(  64,   yRow6, cVALUE),     // 5. lat/long
};

// ========== helpers ==========================================
void erasePrintProportionalText(int xx, int yy, int ww, String text, uint16_t cc) {
  // TODO: remove this function, it is replaced by TextField object
  // use this to specify the width to erase

  // find the height of erasure
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, xx, yy, &x1, &y1, &w, &h);

  tft.fillRect(x1-4, y1-2, ww, h+4, cBACKGROUND); // erase the requested width of old text
  //tft.drawRect(x1-4, y1-2, ww, h+4, cWARN);       // debug: show what area was erased

  // print new text in same spot
  tft.setCursor(xx, yy);
  tft.setTextColor(cc);
  tft.print(text);
}
void printProportionalText(int xx, int yy, String text, uint16_t cc) {
  // Note about proportional fonts:
  // 1. Text origin is bottom left corner
  // 2. Rect origin is upper left corner
  // 2. Printing text does not clear its own background

  // erase old text, 
  // and a few extra pixels around it, in case it was longer than the new text
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, xx, yy, &x1, &y1, &w, &h);

  tft.fillRect(x1-4, y1-2, w+10, h+4, cBACKGROUND); // erase the old text
  tft.drawRect(x1-4, y1-2, w+10, h+4, cWARN);       // debug: show what area was erased

  // print new text in same spot
  tft.setCursor(xx, yy);
  tft.setTextColor(cc);
  tft.print(text);
}

// ========== start status screen view =========================
void updateStatusScreen() {
  initFontSizeSmall();

  // ----- GMT time
  char sTime[10];         // strlen("19:54:14") = 8
  model.getTime(sTime);
  txtValues[0].print(sTime);

  // ----- GMT date
  char sDate[15];         // strlen("Jan 12, 2020") = 13
  model.getDate(sDate, sizeof(sDate));
  txtValues[1].print(sDate);

  // ----- grid square
  char sGrid[10];
  String tmp = model.gsGridName;
  tmp.toCharArray(sGrid, sizeof(sGrid)-1);
  txtValues[2].print(sGrid);
  
  // ----- altitude
  float altitude = model.gAltitude*feetPerMeters;
  String sValue = String(altitude, 0) + " ft";
  sValue.trim();        // remove leading blanks and whitespace
  char sAltitude[12];   // strlen("12345 ft") = 8
  sValue.toCharArray(sAltitude, sizeof(sAltitude)-1);
  txtValues[3].print(sAltitude);

  // ----- satellites
  uint8_t numSatellites = model.gSatellites;
  uint16_t color;
  char sSatellites[6];     // strlen("none") = 4, strlen("4") = 1
  if (numSatellites > 0) {
    txtValues[4].color = cVALUE;
    txtValues[4].print(numSatellites);
  } else {
    txtValues[4].color = cWARN;
    txtValues[4].print("None");
  }
  
  // ----- center lat/long on its own row
  char sLatLong[22];      // strlen("-xxx.xxxx,-yyy.yyyy") = 19
  snprintf(sLatLong, 22, "%.4f,%.4f",
                model.gLatitude, model.gLongitude);
  txtValues[5].print(sLatLong);

  // ----- values
  /* ***** todo
  
  sValue = String(model.gSpeed, 0);
  sValue += String(" mph @ "); 
  sValue += String(model.gAngle, 0);
  printProportionalText(valueX, yRow5, sValue, cVALUE);
  // += String(" deg");         // <-- don't use text (too big)
  // += String(char(248));      // <-- can't use ASCII degree symbol (not in this proportional font)
  int xx = tft.getCursorX() + 6;
  int yy = tft.getCursorY() - 14;
  tft.drawCircle(xx, yy, 3, cVALUE);    // simulate degrees with a circle

  // ----- some items share bottom row
  tft.drawLine(0,yRow5+14, gScreenWidth,yRow5+14, cSEPARATOR);
  if (model.gHaveGPSfix) {
    sValue = String("Yes");
    printProportionalText(labelX, yRow7, "Yes", cVALUE);
  } else {
    printProportionalText(labelX, yRow7, "No ", cWARN);
  }

  sValue = String(model.gSatellites);
  printProportionalText(valueX, yRow7, sValue, cVALUE);

  sValue = String(model.gAltitude, 0) + " ft";
  printProportionalText(190, yRow7, sValue, cVALUE);
  ***** */
}
void startStatScreen() {
  // called once each time this view becomes active
  tft.fillScreen(cBACKGROUND);      // clear screen
  txtValues[0].setBackground(cBACKGROUND);                   // set background for all TextFields in this view
  TextField::setTextDirty( txtValues, numText );             // make sure all fields get re-printed on screen change
  initFontSizeSmall();

  // ----- labels
  for (int ii=0; ii<numLabels; ii++) {
    txtLabels[ii].print();
  }

  updateStatusScreen();             // fill in values immediately, don't wait for the main loop to eventually get around to it
}
bool onTouchStatus(Point touch) {
  Serial.println("->->-> Touched status screen.");
  return false;                     // true=handled, false=controller uses default action
}
