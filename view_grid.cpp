/*
  File: view_grid_screen.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This sketch runs a GPS display for your vehicle's dashboard to
            show your position in the Maidenhead Grid Square, with distances
            to nearby squares. This is intended for ham radio rovers.

  +-------------------------------------------+
  |     124        CN88  30.1 mi      122     |...gTopRowY
  |   48 +-----------------------------+......|...gMarginY
  |      |                          *  |      |
  | CN77 |         CN87                | CN97 |...gMiddleRowY
  | 61.2 |          us                 | 37.1 |
  |      |                             |      |
  |   47 +-----------------------------+      |
  | 123' :         CN86  39.0 mi       :      |...gBottomGridY
  | 47.5644, -122.0378                 :   5# |...gMessageRowY
  +------:---------:-------------------:------+
         :         :                   :
       +gMarginX  gIndentX          -gMarginX
*/

#include <Arduino.h>
#include "Adafruit_GFX.h"           // Core graphics display library
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors
#include "model.cpp"                // "Model" portion of model-view-controller
#include "TextField.h"              // Optimize TFT display text for proportional fonts

// ========== extern ===========================================
extern Adafruit_ILI9341 tft;        // Griduino.ino
extern int gCharWidth, gCharHeight; // character cell size for TextSize(n)
extern Model model;                 // "model" portion of model-view-controller

void initFontSizeBig();             // Griduino.ino
void initFontSizeSmall();           // Griduino.ino
void initFontSizeSystemSmall();     // Griduino.ino
void drawProportionalText(int ulX, int ulY, String prevText, String newText, bool dirty);

// ============== constants ====================================
const int gMarginX = 70;            // define space for grid outline on screen
const int gMarginY = 26;            // and position text relative to this outline
const int gBoxWidth = 180;          // ~= (gScreenWidth - 2*gMarginX);
const int gBoxHeight = 160;         // ~= (gScreenHeight - 3*gMarginY);

// vertical placement of text rows
const int gTopRowY = 20;            // ~= (gMarginY - gCharHeight - 2);
const int gMiddleRowY = 104;        // ~= (gScreenHeight - gCharHeight) / 2;
const int gBottomGridY = 207;       // ~= (gScreenHeight - gCharHeight - 3*gCharHeight);
const int gMessageRowY = 215;       // ~= (gScreenHeight - gCharHeight -1);

// ========== globals ==========================================
String prevGridNorth = "";
String prevGridSouth = "";
String prevGridEast = "";
String prevGridWest = "";
String prevDistNorth = "9999";      // this value is used for first erasure, so init to a large numeral
String prevDistSouth = "9999";
String prevDistEast = "9999";
String prevDistWest = "9999";
bool gDirtyView = true;             // force all text fields to be updated

// ========== helpers ==========================================
void drawGridOutline() {
  tft.drawRect(gMarginX, gMarginY, gBoxWidth, gBoxHeight, ILI9341_CYAN);
}
// ----- workers for "updateGridScreen()" ----- in the same order as called, below

TextField txtGrid4(101,101, cGRIDNAME);
TextField txtGrid6(138,139, cGRIDNAME);

void drawGridName(String newGridName) {
  // huge lettering of current grid square
  // two lines: "CN87" and "us" below it

  initFontSizeBig();

  String grid1_4 = newGridName.substring(0, 4);
  String grid5_6 = newGridName.substring(4, 6);

  txtGrid4.print(grid1_4);
  txtGrid6.print(grid5_6);
}

TextField txtAlt("123'",              4,194, cWARN);    // just above bottom row
TextField txtLL("47.1234,-123.5678",  4,223, cWARN);    // about centered on bottom row
TextField txtNumSat("99#",          280,223, cWARN);    // lower right corner
void drawPositionLL(String sLat, String sLong) {
  //      |1...+....1....+....2....+.|
  //      | ---------,----------  ---|
  // e.g. | -123.4567, -123.4567  10#|
  // e.g. |   47.1234, -122.1234   5#|
  initFontSizeSystemSmall();

  // the message line shows either or a position (lat-long) or a message (waiting for GPS)
  char sTemp[27];       // why 27? Small system font will fit 26 characters on one row
  if (model.gHaveGPSfix) {
    char latitude[10], longitude[10];
    sLat.toCharArray(latitude, sizeof(latitude));
    sLong.toCharArray(longitude, sizeof(longitude));
    snprintf(sTemp, sizeof(sTemp), "%s, %s", latitude, longitude);
  } else {
    strcpy(sTemp, "Waiting for GPS");
  }
  txtLL.print(sTemp);               // latitude-longitude

  if (model.gSatellites<10) {
    sprintf(sTemp, " %d#", model.gSatellites);
  } else {
    sprintf(sTemp, "%d#", model.gSatellites);
  }
  txtNumSat.print(sTemp);           // number of satellites

  int altFeet = model.gAltitude * feetPerMeters;
  sprintf(sTemp, "%d'", altFeet);
  txtAlt.print(sTemp);              // altitude
}

const int numCompass = 4;
TextField txtCompass[numCompass] = {
  //        text      x,y      color     background
  TextField( "N",   156,  47,  cCOMPASS, ILI9341_BLACK ),  // centered left-right
  TextField( "S",   156, 181,  cCOMPASS, ILI9341_BLACK ),
  TextField( "E",   232, 114,  cCOMPASS, ILI9341_BLACK ),  // centered top-bottom
  TextField( "W",    73, 114,  cCOMPASS, ILI9341_BLACK ),
};
void drawCompassPoints() {
  initFontSizeSmall();
  for (int ii=0; ii<numCompass; ii++) {
    txtCompass[ii].print();
  }
}
void drawLatLong() {
  return;   // disabled because it makes the screen too busy
            // also disabled because its screen text positioning need updating
            // also disabled because it always shows 47-48 and 122-124 degrees

  initFontSizeSmall();
  tft.setTextColor(cCOMPASS, ILI9341_BLACK);

  tft.setCursor(gCharWidth * 1 / 2, gTopRowY);
  tft.print("124");   // TODO: read this from the model

  tft.setCursor(gScreenWidth - gMarginX + gCharWidth, gTopRowY);
  tft.print("122");

  tft.setCursor(gCharWidth * 3 / 2, gMarginY + gCharHeight / 4);
  tft.print("48");

  tft.setCursor(gCharWidth * 3 / 2, gScreenHeight - gMarginY - gCharHeight);
  tft.print("47");
}
void drawNeighborGridNames() {
  initFontSizeSmall();
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);

  int indentX = gMarginX + gCharWidth*1;
  drawProportionalText(indentX, gTopRowY,     prevGridNorth, model.gsGridNorth, gDirtyView); // NORTH grid, e.g. "CN88"
  drawProportionalText(indentX, gBottomGridY, prevGridSouth, model.gsGridSouth, gDirtyView); // SOUTH grid, e.g. "CN86"

  indentX = gScreenWidth - gCharWidth*4;
  drawProportionalText(indentX, gMiddleRowY,  prevGridEast,  model.gsGridEast,  gDirtyView); // EAST  grid, e.g. "CN97"
  drawProportionalText(0,       gMiddleRowY,  prevGridWest,  model.gsGridWest,  gDirtyView); // WEST  grid, e.g. "CN77"

  prevGridNorth = model.gsGridNorth;
  prevGridSouth = model.gsGridSouth;
  prevGridWest  = model.gsGridWest;
  prevGridEast  = model.gsGridEast;
}
void drawNeighborDistances() {
  initFontSizeSmall();
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);

  int indentX = gMarginX + gCharWidth*6;
  drawProportionalText(indentX, gTopRowY,     prevDistNorth, model.gsDistanceNorth, gDirtyView); // NORTH distance, e.g. "17.3"
  drawProportionalText(indentX, gBottomGridY, prevDistSouth, model.gsDistanceSouth, gDirtyView); // SOUTH distance, e.g. "63.4"

  indentX = gScreenWidth - 4 * gCharWidth;
  int indentY = gMiddleRowY + gCharHeight + 4;
  drawProportionalText(indentX, indentY, prevDistEast, model.gsDistanceEast,  gDirtyView); // EAST distance, e.g. "15.5"
  drawProportionalText(0,       indentY, prevDistWest, model.gsDistanceWest,  gDirtyView); // WEST distance, e.g. "81.4"

  prevDistNorth = model.gsDistanceNorth;
  prevDistSouth = model.gsDistanceSouth;
  prevDistEast  = model.gsDistanceEast;
  prevDistWest  = model.gsDistanceWest;
}
void plotPosition(double fLat, double fLong) {
  // put a pushpin inside the grid's box, proportional to your position within the grid
  // input:  fLat, fLong - double precision float

  if (fLat == 0.0) return;

  int radius = 2;   // size of pushpin

  const float gridWidthDegrees = 2.0;
  const float gridHeightDegrees = 1.0;

  float degreesX = fLong - nextGridLineWest(fLong);
  float degreesY = fLat - nextGridLineSouth(fLat);

  double fracGridX = degreesX / gridWidthDegrees; // E-W position as fraction of grid width, 0-1
  double fracGridY = degreesY / gridHeightDegrees; // N-S position as fraction of grid height, 0-1

  // our drawing canvas is a box the size of the screen, minus an outside margin on all sides reserved for text
  // TFT screen coordinate system origin (0,0) in upper left corner, positive numbers go right/down
  // and the real-world lat-long system origin (0,0) in lower left corner, positive numbers go right/up
  // breadcrumb trail must fit the box drawn by drawGridOutline() e.g. (gMarginX, gMarginY, gBoxWidth, gBoxHeight)
  int llCanvasX = gMarginX;                   // canvas lower left corner (pixels)
  int llCanvasY = gMarginY + gBoxHeight;

  int canvasWidth = gBoxWidth;                // canvas dimensions (pixels)
  int canvasHeight = gBoxHeight;

  int plotX = llCanvasX + canvasWidth * fracGridX; // pushpin location on canvas (pixels)
  int plotY = llCanvasY - canvasHeight * fracGridY;

  /***
    Serial.print("   ");
    Serial.print("Plot: lat-long("); Serial.print(fLong);       Serial.print(","); Serial.print(fLat);         Serial.print(")");
    Serial.print(", degrees(");      Serial.print(degreesX);    Serial.print(","); Serial.print(degreesY);     Serial.print(")");
    //rial.print(", canvas(");       Serial.print(canvasWidth); Serial.print(","); Serial.print(canvasHeight); Serial.print(")");
    Serial.print(", fracGridXY(");   Serial.print(fracGridX);   Serial.print(","); Serial.print(fracGridY);    Serial.print(")");
    Serial.print(", plotXY(");       Serial.print(plotX);       Serial.print(","); Serial.print(plotY);        Serial.print(")");
    Serial.println(" ");
  ***/

  tft.fillCircle(plotX, plotY, radius, ILI9341_BLACK);  // erase the circle's background
  tft.drawCircle(plotX, plotY, radius, ILI9341_ORANGE); // draw new circle
  tft.drawPixel(plotX, plotY, ILI9341_WHITE);           // with a cherry in the middle
}
// ========== grid screen view =================================
void updateGridScreen() {
  // called on every pass through main()
  drawGridName(model.gsGridName);   // huge letters centered on screen
  drawPositionLL(model.gsLatitude, model.gsLongitude);  // lat-long of current position
  drawCompassPoints();              // sprinkle N-S-E-W around grid square
  drawLatLong();                    // identify coordinates of grid square
  drawNeighborGridNames();          // sprinkle names around outside box
  drawNeighborDistances();          // this is the main goal of the whole project
  plotPosition(model.gLatitude, model.gLongitude);      // pushpin
  gDirtyView = false;
}
void startGridScreen() {
  // called once each time this view becomes active
  tft.fillScreen(ILI9341_BLACK);    // clear screen
  txtCompass[0].setBackground(ILI9341_BLACK);          // set background for all TextFields in this view
  TextField::setTextDirty( txtCompass, numCompass );   // make sure all fields get re-printed on screen change
  txtGrid4.dirty = true;
  txtGrid6.dirty = true;
  txtAlt.dirty = true;
  txtLL.dirty = true;
  txtNumSat.dirty = true;
  initFontSizeSmall();

  drawGridOutline();                // box outline around grid
  //tft.drawRect(0, 0, gScreenWidth, gScreenHeight, ILI9341_BLUE);  // debug: border around screen
  model.grid4dirty = true;          // reset the "previous grid" to trigger the new one to show
  model.grid6dirty = true;
  gDirtyView = true;                // force all text fields to be updated

  updateGridScreen();               // fill in values immediately, don't wait for the main loop to eventually get around to it
}
bool onTouchGrid(Point touch) {
  Serial.println("->->-> Touched grid detail screen.");
  return false;                     // true=handled, false=controller uses default action
}
