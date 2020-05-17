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
  |      :         CN86  39.0 mi       : 123' |...gBottomGridY
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
//void drawProportionalText(int ulX, int ulY, String prevText, String newText, bool dirty);

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

// ========== helpers ==========================================
void drawGridOutline() {
  tft.drawRect(gMarginX, gMarginY, gBoxWidth, gBoxHeight, ILI9341_CYAN);
}
// ----- workers for "updateGridScreen()" ----- in the same order as called, below

// these are names for the array indexes for readability, must be named in same order as below
enum txtIndex {
  GRID4=0, GRID6, LATLONG, ALTITUDE, NUMSAT,
  N_COMPASS,   S_COMPASS,   E_COMPASS,   W_COMPASS,
  N_DISTANCE,  S_DISTANCE,  E_DISTANCE,  W_DISTANCE,
  N_GRIDNAME,  S_GRIDNAME,  E_GRIDNAME,  W_GRIDNAME,
  N_BOXDEGREES,S_BOXDEGREES,E_BOXDEGREES,W_BOXDEGREES,
};
                                            
TextField txtGrid[] = {
  //         text      x,y     color
  TextField("CN77",  101,101,  cGRIDNAME),      // GRID4: center of screen
  TextField("tt",    138,139,  cGRIDNAME),      // GRID6: center of screen
  TextField("47.1234,-123.5678", 4,223, cWARN), // LATLONG: left-adj on bottom row
  TextField("123'",  315,196,  cWARN, FLUSHRIGHT),  // ALTITUDE: just above bottom row
  TextField("99#",   315,221,  cWARN, FLUSHRIGHT),  // NUMSAT: lower right corner
  TextField( "N",    156, 47,  cCOMPASS ),      // N_COMPASS: centered left-right
  TextField( "S",    156,181,  cCOMPASS ),      // S_COMPASS
  TextField( "E",    232,114,  cCOMPASS ),      // E_COMPASS: centered top-bottom
  TextField( "W",     73,114,  cCOMPASS ),      // W_COMPASS
  TextField("17.1",  180, 20,  cDISTANCE),      // N_DISTANCE
  TextField("52.0",  180,207,  cDISTANCE),      // S_DISTANCE
  TextField("13.2",  256,130,  cDISTANCE),      // E_DISTANCE
  TextField("79.7",    0,130,  cDISTANCE),      // W_DISTANCE
  TextField("CN88",  102, 20,  cGRIDNAME),      // N_GRIDNAME
  TextField("CN86",  102,207,  cGRIDNAME),      // S_GRIDNAME
  TextField("CN97",  256,102,  cGRIDNAME),      // E_GRIDNAME
  TextField("CN77",    0,102,  cGRIDNAME),      // W_GRIDNAME
  TextField("48",     56, 44,  cBOXDEGREES, FLUSHRIGHT),  // N_BOXDEGREES
  TextField("47",     56,188,  cBOXDEGREES, FLUSHRIGHT),  // S_BOXDEGREES
  TextField("122",   243, 20,  cBOXDEGREES),              // E_BOXDEGREES
  TextField("124",    72, 20,  cBOXDEGREES, FLUSHRIGHT),  // W_BOXDEGREES
};
const int numTextGrid = sizeof(txtGrid)/sizeof(TextField);

void drawGridName(String newGridName) {
  // huge lettering of current grid square
  // two lines: "CN87" and "us" below it

  initFontSizeBig();

  String grid1_4 = newGridName.substring(0, 4);
  String grid5_6 = newGridName.substring(4, 6);

  txtGrid[GRID4].print(grid1_4);
  txtGrid[GRID6].print(grid5_6);
}

void drawPositionLL(String sLat, String sLong) {
  initFontSizeSystemSmall();

  // the message line shows either or a position (lat,long) or a message (waiting for GPS)
  char sTemp[27];       // why 27? Small system font will fit 26 characters on one row
  if (model.gHaveGPSfix) {
    char latitude[10], longitude[10];
    sLat.toCharArray(latitude, sizeof(latitude));
    sLong.toCharArray(longitude, sizeof(longitude));
    snprintf(sTemp, sizeof(sTemp), "%s, %s", latitude, longitude);
  } else {
    strcpy(sTemp, "Waiting for GPS");
  }
  txtGrid[LATLONG].print(sTemp);          // latitude-longitude

  if (model.gSatellites<10) {
    sprintf(sTemp, " %d#", model.gSatellites);
  } else {
    sprintf(sTemp, "%d#", model.gSatellites);
  }
  txtGrid[NUMSAT].print(sTemp);           // number of satellites

  int altFeet = model.gAltitude * feetPerMeters;
  sprintf(sTemp, "%d'", altFeet);
  txtGrid[ALTITUDE].print(sTemp);         // altitude
}

void drawCompassPoints() {
  initFontSizeSmall();
  for (int ii=N_COMPASS; ii<N_COMPASS+4; ii++) {
    txtGrid[ii].print();
  }
}
void drawBoxLatLong() {
  //return;   // disabled because it makes the screen too busy
            // also disabled because it always shows 47-48 and 122-124 degrees

  initFontSizeSmall();
  for (int ii=N_BOXDEGREES; ii<=W_BOXDEGREES; ii++) {
    txtGrid[ii].print();
  }
  int radius = 3;
  //                          x                          y                r     color
  tft.drawCircle(txtGrid[N_BOXDEGREES].x+7, txtGrid[N_BOXDEGREES].y-14, radius, cBOXDEGREES); // draw circle to represent "degrees"
  tft.drawCircle(txtGrid[S_BOXDEGREES].x+7, txtGrid[S_BOXDEGREES].y-14, radius, cBOXDEGREES);
  //t.drawCircle(txtGrid[E_BOXDEGREES].x+7, txtGrid[E_BOXDEGREES].y-14, radius, cBOXDEGREES);  // todo: where to put "degrees" on FLUSHLEFT number?
  tft.drawCircle(txtGrid[W_BOXDEGREES].x+7, txtGrid[W_BOXDEGREES].y-14, radius, cBOXDEGREES);

  /* *****
  tft.setTextColor(cCOMPASS, ILI9341_BLACK);

  tft.setCursor(gCharWidth * 1 / 2, gTopRowY);
  Serial.print("West degrees x,y = "); Serial.print(gCharWidth * 1 / 2); Serial.print(","); Serial.println(gTopRowY);
  tft.print("124");   // TODO: read this from the model

  tft.setCursor(gScreenWidth - gMarginX + gCharWidth, gTopRowY);
  Serial.print("East degrees x,y = "); Serial.print(gScreenWidth - gMarginX + gCharWidth); Serial.print(","); Serial.println(gTopRowY);
  tft.print("122");

  tft.setCursor(gCharWidth * 3 / 2, gMarginY + gCharHeight / 4);
  Serial.print("North degrees x,y = "); Serial.print(gCharWidth * 3 / 2); Serial.print(","); Serial.println(gMarginY + gCharHeight / 4);
  tft.print("48");

  tft.setCursor(gCharWidth * 3 / 2, gScreenHeight - gMarginY - gCharHeight);
  Serial.print("South degrees x,y = "); Serial.print(gCharWidth * 3 / 2); Serial.print(","); Serial.println(gScreenHeight - gMarginY - gCharHeight);
  tft.print("47");
  ***** */
}
void drawNeighborGridNames() {
  initFontSizeSmall();
  txtGrid[N_GRIDNAME].print(model.gsGridNorth);
  txtGrid[S_GRIDNAME].print(model.gsGridSouth);
  txtGrid[E_GRIDNAME].print(model.gsGridEast);
  txtGrid[W_GRIDNAME].print(model.gsGridWest);
}
void drawNeighborDistances() {
  initFontSizeSmall();
  txtGrid[N_DISTANCE].print(model.gsDistanceNorth);
  txtGrid[S_DISTANCE].print(model.gsDistanceSouth);
  txtGrid[E_DISTANCE].print(model.gsDistanceEast);
  txtGrid[W_DISTANCE].print(model.gsDistanceWest);
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

  double fracGridX = degreesX / gridWidthDegrees;  // E-W position as fraction of grid width, 0-1
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
  //drawCompassPoints();              // sprinkle N-S-E-W around grid square
  //drawBoxLatLong();                 // identify coordinates of grid square box
  drawNeighborGridNames();          // sprinkle names around outside box
  drawNeighborDistances();          // this is the main goal of the whole project
  plotPosition(model.gLatitude, model.gLongitude);      // pushpin
}
void startGridScreen() {
  // called once each time this view becomes active
  tft.fillScreen(ILI9341_BLACK);    // clear screen
  txtGrid[0].setBackground(ILI9341_BLACK);          // set background for all TextFields in this view
  TextField::setTextDirty( txtGrid, numTextGrid );
  Serial.print("numTextGrid = "); Serial.println(numTextGrid);  // debug
  initFontSizeSmall();

  drawGridOutline();                // box outline around grid
  //tft.drawRect(0, 0, gScreenWidth, gScreenHeight, ILI9341_BLUE);  // debug: border around screen
  model.grid4dirty = true;          // reset the "previous grid" to trigger the new one to show
  model.grid6dirty = true;
  //gDirtyView = true;                // force all text fields to be updated

  updateGridScreen();               // fill in values immediately, don't wait for the main loop to eventually get around to it
}
bool onTouchGrid(Point touch) {
  Serial.println("->->-> Touched grid detail screen.");
  return false;                     // true=handled, false=controller uses default action
}
