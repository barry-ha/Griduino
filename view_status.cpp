/* File: view_stat_screen.cpp

  This screen shows most of what the GPS knows about location and
  velocity in text format. Results are updated in real time.

  todo: This might be a good place to show: 
  1. "How far is it to the next grid line in this direction? 
  2. How long will it take to arrive at this speed?

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
#include <Adafruit_GFX.h>           // Core graphics display library
#include <Adafruit_ILI9341.h>       // TFT color display library
#include "constants.h"              // Griduino constant definitions
#include "model.cpp"                // "Model" portion of model-view-controller

// ========== extern ==================================
extern Adafruit_ILI9341 tft;        // Griduino.ino
extern int gTextSize;               // no such function as "tft.getTextSize()" so remember it on our own
extern int gCharWidth, gCharHeight; // character cell size for TextSize(n)
extern int gUnitFontWidth, gUnitFontHeight; // character cell size for TextSize(1)
extern Model model;                 // "model" portion of model-view-controller
void updateStatusScreen();          // forward reference in this same .cpp file

void showNameOfView(String sName, uint16_t fgd, uint16_t bkg);  // Griduino.ino
void initFontSizeSmall();           // Griduino.ino
void initFontSizeBig();             // Griduino.ino
int getOffsetToCenterText(String text); // Griduino.ino

// ------------ typedef's
typedef struct {
  int x;
  int y;
} Point;
typedef struct {
  char text[24];
  int x;
  int y;
  uint16_t color;
} Label;

// ========== constants ===============================
// placement of text rows
const int yRow1 = 28;
const int yRow2 = yRow1 + 32;
const int yRow3 = yRow2 + 44;
const int yRow4 = yRow3 + 44;
const int yRow5 = yRow4 + 32;
const int yRow6 = yRow5 + 40;
const int yRow7 = yRow6 + 20;

const int labelX = 8;       // indent labels, slight margin to left edge of screen
const int valueX = 124;     // indent values

// color scheme: see constants.h

const int numLabels = 4;
Label statLabels[numLabels] = {
  {"GMT:",        labelX, yRow1, cLABEL},
  {"Grid:",       labelX, yRow3, cLABEL},
  {"Altitude:",   labelX, yRow4, cLABEL},
  {"Satellites:", labelX, yRow5, cLABEL},
};

// ========== globals =================================

// ========== helpers =================================
void erasePrintProportionalText(int xx, int yy, int ww, String text, uint16_t cc) {
  // use this to specify the width to erase

  // find the height of erasure
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, xx, yy, &x1, &y1, &w, &h);

  tft.fillRect(x1-4, y1-2, ww, h+4, cBACKGROUND); // erase the requested width of old text
  //t.drawRect(x1-4, y1-2, ww, h+4, cWARN);       // debug: show what area was erased

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
  //t.drawRect(x1-4, y1-2, w+10, h+4, cWARN);       // debug: show what area was erased

  // print new text in same spot
  tft.setCursor(xx, yy);
  tft.setTextColor(cc);
  tft.print(text);
}

// ========== stat screen view ========================
void startStatScreen() {
  tft.fillScreen(cBACKGROUND);
  initFontSizeSmall();

  // ----- labels
  for (int ii=0; ii<numLabels; ii++) {
    Label item = statLabels[ii];
    tft.setCursor(item.x, item.y);
    tft.setTextColor(item.color);
    tft.print(item.text);
  }

  updateStatusScreen();               // fill in values immediately, don't wait for loop() to eventually get around to it

  //delay(4000);                    // no delay - the controller handles the schedule
  //tft.fillScreen(cBACKGROUND);     // no clear - this screen is visible until the next view clears it
}
void updateStatusScreen() {
  initFontSizeSmall();

  // ----- GMT time
  char sTime[10];         // strlen("19:54:14") = 8
  model.getTime(sTime);
  printProportionalText(valueX, yRow1, sTime, cVALUE);

  // ----- GMT date
  char sDate[15];         // strlen("Jan 12, 2020") = 13
  model.getDate(sDate, sizeof(sDate));
  printProportionalText(valueX, yRow2, sDate, cVALUE);

  // ----- grid square
  // brighter color because "grid" is important
  printProportionalText(valueX, yRow3, model.gsGridName, cHIGHLIGHT);
  
  // ----- altitude
  float altitude = model.gAltitude*feetPerMeters;
  String sValue = String(altitude, 0) + " ft";
  sValue.trim();        // remove leading blanks and whitespace
  //Serial.print("Status: altitude string = '"); Serial.print(sValue); Serial.println("'"); // debug, is there a leading space?
  printProportionalText(valueX, yRow4, sValue, cVALUE);

  // ----- satellites
  uint8_t numSatellites = model.gSatellites;
  uint16_t color;
  if (numSatellites > 0) {
    sValue = String(numSatellites);
    color = cVALUE;
  } else {
    sValue = String("None");
    color = cWARN;
  }
  printProportionalText(valueX, yRow5, sValue, color);
  
  // ----- center lat/long on its own row
  char sLatLong[22];      // strlen("-xxx.xxxx,-yyy.yyyy") = 19
  snprintf(sLatLong, 22, "%.4f,%.4f",
                model.gLatitude, model.gLongitude);
  int llX = getOffsetToCenterText(sLatLong);
  printProportionalText(llX, yRow6, sLatLong, cVALUE);

  // ----- values
  /* *****
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
bool onTouchStatus(Point touch) {
  Serial.println("->->-> Touched status screen.");
  return false;                     // ignore touch, let controller handle with default action
}
