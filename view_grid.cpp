/*
  File: view_grid_screen.cpp

  This sketch runs a GPS display for your vehicle's dashboard to show
  your position in the Maidenhead Grid Square, with distances to
  nearby squares. This is intended for ham radio rovers.

  +-------------------------------------------+
  |     124        CN88  30.1 mi      122     |...gTopRowY
  |   48 +-----------------------------+......|...gMarginY
  |      |                          *  |      |
  | CN77 |         CN87                | CN97 |...gMiddleRowY
  | 61.2 |                             | 37.1 |
  |      |                             |      |
  |   47 +-----------------------------+      |
  |      :         CN86  39.0 mi       :      |...gBottomGridY
  | 123' : 47.5644 :  -122.0378        :      |...gMessageRowY
  +------:---------:-------------------:------+
         :         :                   :
       +gMarginX  gIndentX          -gMarginX
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
extern int gCharWidth, gCharHeight;         // character cell size for TextSize(n)
extern Model model;                 // "model" portion of model-view-controller

void initFontSizeBig();             // Griduino.ino
void initFontSizeSmall();           // Griduino.ino
void initFontSizeSystemSmall();     // Griduino.ino
int getOffsetToCenterText(String text); // Griduino.ino
void drawProportionalText(int ulX, int ulY, String prevText, String newText, bool dirty);

// ------------ typedef's
typedef struct {
  int x;
  int y;
} Point;

// ========== constants ===============================
const int gMarginX = 70;            // define space for grid outline on screen
const int gMarginY = 26;            // and position text relative to this outline
const int gBoxWidth = 180;                // ~= (gScreenWidth - 2*gMarginX);
const int gBoxHeight = 160;               // ~= (gScreenHeight - 3*gMarginY);

// vertical placement of text rows
const int gTopRowY = 20;            // ~= (gMarginY - gCharHeight - 2);
const int gMiddleRowY = 104;        // ~= (gScreenHeight - gCharHeight) / 2;
const int gBottomGridY = 207;       // ~= (gScreenHeight - gCharHeight - 3*gCharHeight);
const int gMessageRowY = 215;       // ~= (gScreenHeight - gCharHeight -1);

// ========== globals =================================
String prevGrid1_4, prevGrid5_6;
String prevGridNorth = "";
String prevGridSouth = "";
String prevGridEast = "";
String prevGridWest = "";
String prevDistNorth = "9999";      // this value is used for first erasure, so init to a large numeral
String prevDistSouth = "9999";
String prevDistEast = "9999";
String prevDistWest = "9999";
bool gDirtyView = true;             // force all text fields to be updated

// ========== helpers =================================
void drawGridOutline() {
  tft.drawRect(gMarginX, gMarginY, gBoxWidth, gBoxHeight, ILI9341_CYAN);
}
// ----- workers for "updateGridScreen()" ----- in the same order as called, below
void drawGridName(String newGridName) {
  // huge lettering of current grid square
  // two lines: "CN87" and "us" below it
  // Note about proportional fonts:
  // 1. The origin is bottom left corner
  // 2. Printing text does not clear its own background
  int rectX, rectY, rectWidth, rectHeight;
  initFontSizeBig();

  //~Serial.print("drawGridName("); Serial.print(newGridName); Serial.println(")");  // debug

  // --- line 1 ---
  int stringlen = 4;     // = strlen(gsGridName)
  String grid1_4 = newGridName.substring(0, 4);
  String grid5_6 = newGridName.substring(4, 6);

  // Compute upper-left corner position of text
  rectWidth = stringlen * gCharWidth;           // "gCharWidth" is my approximation of average proportional font width
  rectHeight = gCharHeight;                     // "gCharHeight" is an approximate proportional font height,
                                                // taking into account both capital letters and descenders
  rectX = (gScreenWidth - rectWidth) / 2;       // center text left-right (ul corner X)
  rectY = (gScreenHeight / 2) - (rectHeight / 2); // center text top-bottom (ul corner Y)

  const int g4x = rectX;                        // figure out where to put the giant Grid4 text
  const int g4y = rectY;
  const int g6x = rectX + gCharWidth * 8 / 6;   // the Grid6 letters are approx centered below Grid4
  const int g6y = rectY + gCharHeight;

  tft.setTextColor(ILI9341_GREEN);
  if (model.grid4dirty) {
    drawProportionalText(g4x, g4y, prevGrid1_4, grid1_4, model.grid4dirty);
    model.grid4dirty = false;
  }
  if (model.grid6dirty) {
    drawProportionalText(g6x, g6y, prevGrid5_6, grid5_6, model.grid6dirty);
    model.grid6dirty = false;
  }

  // remember what was written so it can be erased accurately and efficiently at next grid-crossing
  prevGrid1_4 = grid1_4;
  prevGrid5_6 = grid5_6;
}
void drawPositionLL(String sLat, String sLong) {
  initFontSizeSystemSmall();
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);

  int x0 = 0*gCharWidth;        // indent alt-lat-long on bottom row
  int y0 = gScreenHeight - gCharHeight;

  // the message line shows either or a position (lat-long) or a message (waiting for GPS)
  if (model.gHaveGPSfix) {
  
    int altFeet = model.gAltitude * feetPerMeters;
  
    tft.setCursor(x0, y0);
    //String message = altFeet + "',  " + sLat + ", " + sLong;
    tft.print(altFeet);
    tft.print("'   ");
    tft.print(sLat);      // show latitude on row 1
    tft.print(", ");
    tft.print(sLong);
  } else {
    tft.setCursor(x0, y0);
    tft.print("Waiting for GPS");
  }
}
void drawLatLong() {
  return;   // disabled because it makes the screen too busy
            // also disabled because its screen text positioning need updating
            // also disabled because it always shows 47-48 and 122-124 degrees

  initFontSizeSmall();
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);

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
  // test data: N 17.3 mi, S 63.4, E 15.5 to CN97as, W 81.4 to CN77xs
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
// ========== grid screen view ================
void updateGridScreen() {
  // called on every pass through main()
  drawGridName(model.gsGridName);   // huge letters centered on screen
  drawPositionLL(model.gsLatitude, model.gsLongitude);  // lat-long of current position
  drawLatLong();                    // identify box
  drawNeighborGridNames();          // sprinkle names around outside box
  drawNeighborDistances();          // this is the main goal of the whole project
  plotPosition(model.gLatitude, model.gLongitude);      // pushpin
  gDirtyView = false;
}
void startGridScreen() {
  // called once when view becomes active
  initFontSizeSmall();

  tft.fillScreen(ILI9341_BLACK);    // clear screen
  drawGridOutline();                // box outline around grid
  //tft.drawRect(0, 0, gScreenWidth, gScreenHeight, ILI9341_BLUE);  // debug: border around screen
  model.grid4dirty = true;          // reset the "previous grid" to trigger the new one to show
  model.grid6dirty = true;
  gDirtyView = true;                // force all text fields to be updated

  updateGridScreen();
}
bool onTouchGrid(Point touch) {
  Serial.println("->->-> Touched grid detail screen.");
  return false;                     // ignore touch, let controller handle with default action
}
