/*
   File:    view_status.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Show the grid square's characteritics and how it is
            displayed on the screen. It gives the user a sense of 
            how to interpret the bread crumb trail, and how far to
            to within a 6-digit grid square.

            +-----------------------------------+
            | *      Grid Size and Scale      > |...yRow1
            |                                   |...yRow2 (unused)
            |     Size CN87:  93 x 69 mi        |...yRow3
            |   Size CN87us:  3.9 x 2.9 mi      |...yRow4
            |                                   |
            |   Scale:  1 pixel = 0.5 miles     |...yRow5
            |                                   |
            |    Jan 01, 2020  01:01:01  GMT    |...yRow7
            +------------:--:-------------------+
                    labelX  valueX
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>         // TFT color display library
#include "constants.h"                // Griduino constants and colors
#include "model_gps.h"                // "Model" portion of model-view-controller
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "view.h"                     // Base class for all views

// ========== extern ===========================================
extern Model* model;                  // "model" portion of model-view-controller
void floatToCharArray(char* result, int maxlen, double fValue, int decimalPlaces);  // Griduino.ino
extern void showDefaultTouchTargets();// Griduino.ino

// ========== class ViewStatus =================================
class ViewStatus : public View {
  public:
    // ---------- public interface ----------
    // This derived class must implement the public interface:
    ViewStatus(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    {
      background = cBACKGROUND;       // every view can have its own background color
    }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);

  protected:
    // ---------- local data for this derived class ----------
    // color scheme: see constants.h

    // vertical placement of text rows
    const int space = 30;
    const int half = space/2;

    const int yRow1 = 18;
    const int yRow2 = yRow1 + space;
    const int yRow3 = yRow2 + half;
    const int yRow4 = yRow3 + space;
    const int yRow5 = yRow4 + space+half;
    const int yRow6 = yRow5 + space;
    const int yRow7 = 226;                // GMT date on bottom row, "226" will match other views

    const int labelX = 122;               // right-align labels, near their values
    const int valueX = 140;               // left-align values

    const int label2 = 100;
    const int value2 = 118;

    // ----- screen text
    // names for the array indexes, must be named in same order as array below
    enum txtIndex {
      TITLE=0,
      GRID4, SIZE4,
      GRID6, SIZE6, 
      SCALE_LABEL, SCALE,
      GMT_DATE, GMT_TIME, GMT
    };

    // ----- static + dynamic screen text
    #define nStatusValues 10
    TextField txtValues[nStatusValues] = {
      {"Grid Size and Scale",  -1, yRow1, cTITLE,     ALIGNCENTER, eFONTSMALLEST},  // [TITLE] view title, centered
      {"CN87:",            labelX, yRow3, cLABEL,     ALIGNRIGHT,  eFONTSMALL},     // [GRID4]
      {"101 x 69 mi",      valueX, yRow3, cHIGHLIGHT, ALIGNLEFT,   eFONTSMALL},     // [SIZE4]
      {"CN87us:",          labelX, yRow4, cLABEL,     ALIGNRIGHT,  eFONTSMALL},     // [GRID6]
      {"4.4 x 3.3 mi",     valueX, yRow4, cVALUE,     ALIGNLEFT,   eFONTSMALL},     // [SIZE6]
      {"Screen:",          label2, yRow5, cLABEL,     ALIGNRIGHT,  eFONTSMALL},     // [SCALE_LABEL]
      {"1 pixel = 0.7 mi", value2, yRow5, cVALUE,     ALIGNLEFT,   eFONTSMALL},     // [SCALE]
      {"Apr 26, 2021",        130, yRow7, cFAINT,     ALIGNRIGHT,  eFONTSMALLEST},  // [GMT_DATE]
      {"02:34:56",            148, yRow7, cFAINT,     ALIGNLEFT,   eFONTSMALLEST},  // [GMT_TIME]
      {"GMT",                 232, yRow7, cFAINT,     ALIGNLEFT,   eFONTSMALLEST},  // [GMT]
    };

};  // end class ViewStatus

// ============== implement public interface ================
void ViewStatus::updateScreen() {
  // called on every pass through main()

  setFontSize(12);
  char sUnits[] = "mi";
  if (model->gMetric) {
    strcpy(sUnits, "km");
  }

  // ----- 4-digit grid size
  char sGrid[10];                         // strlen("CN87us") = 6
  calcLocator(sGrid, model->gLatitude, model->gLongitude, 4);
  strcat(sGrid, ":");
  txtValues[GRID4].print(sGrid);

  // all North-South distances are the same but we'll calculate it anyway
  float nextNorth = model->nextGridLineNorth();
  float nextSouth = model->nextGridLineSouth();
  float nextEast = model->nextGridLineEast();
  float nextWest = model->nextGridLineWest();
  int nsDistance = (int) round(model->calcDistanceLat(nextNorth, nextSouth));
  int ewDistance = (int) round(model->calcDistanceLong(model->gLatitude, nextEast, nextWest));

  char msg[33];
  snprintf(msg, sizeof(msg), "%d x %d %s", ewDistance, nsDistance, sUnits);
  txtValues[SIZE4].print(msg);
  
  // ----- 6-digit grid size
  calcLocator(sGrid, model->gLatitude, model->gLongitude, 6);
  strcat(sGrid, ":");
  txtValues[GRID6].print(sGrid);

  nextNorth = model->nextGrid6North();
  nextSouth = model->nextGrid6South();
  nextEast = model->nextGrid6East();
  nextWest = model->nextGrid6West();
  float fNS = model->calcDistanceLat(nextNorth, nextSouth);
  float fEW = model->calcDistanceLong(model->gLatitude, nextEast, nextWest);
  char sNS[10], sEW[10];
  floatToCharArray(sNS, 8, fNS, 1);
  floatToCharArray(sEW, 8, fEW, 1);
  snprintf(msg, sizeof(msg), "%s x %s %s", sEW, sNS, sUnits);
  txtValues[SIZE6].print(msg);

  // ----- scale of our screen map and the breadcrumb trail
  // averaging together the scale in each direction
  // this is the minimum real-world travel required to turn on the next pixel
  const double minLong = gridWidthDegrees / gBoxWidth;  // longitude degrees from one pixel to the next
  const double minLat = gridHeightDegrees / gBoxHeight; // latitude degrees from one pixel to the next

  float ewScale = model->calcDistanceLong(model->gLatitude, 0.0, minLong);
  float nsScale = model->calcDistanceLat(0.0, minLat);
  float scale = (ewScale + nsScale)/2;
  char sScale[10];
  floatToCharArray(sScale, 8, scale, 1);
  snprintf(msg, sizeof(msg), "1 pixel = %s %s", sScale, sUnits);
  txtValues[SCALE].print(msg);

  // ----- GMT date & time
  char sDate[15];                     // strlen("Jan 12, 2020") = 13
  char sTime[10];                     // strlen("19:54:14") = 8
  model->getDate(sDate, sizeof(sDate));
  model->getTime(sTime);
  txtValues[GMT_DATE].print(sDate);
  txtValues[GMT_TIME].print(sTime);
  txtValues[GMT].print();
} // end updateScreen

void ViewStatus::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                  // clear screen
  txtValues[0].setBackground(this->background);         // set background for all TextFields in this view
  TextField::setTextDirty( txtValues, nStatusValues );  // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALL);

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();          // optionally draw boxes around button-touch area
  showScreenBorder();                 // optionally outline visible area
  showScreenCenterline();             // optionally draw visual alignment bar

  // ----- draw fields that have static text
  txtValues[TITLE].print();
  txtValues[SCALE_LABEL].print();
  updateScreen();                     // fill in values immediately, don't wait for the main loop to eventually get around to it

  #ifdef SHOW_SCREEN_CENTERLINE
    // show centerline at      x1,y1              x2,y2             color
    tft->drawLine( tft->width()/2,0,  tft->width()/2,tft->height(), cWARN); // debug
  #endif
}


bool ViewStatus::onTouch(Point touch) {
  Serial.println("->->-> Touched status screen.");
  return false;                       // true=handled, false=controller uses default action
} // end onTouch()
