/*
   File:    view_status.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This screen shows most of what the GPS knows about location
            and velocity in text format. Results are updated in real time.

            Todo: Add "internal temperature: 97F" readout
            Add "local time zone: -7h" readout

            +-----------------------------------+
            |         GMT:  19:54:14            |...yRow1
            |               Jan 15, 2020        |...yRow2
            |        Grid:  CN87us              |...yRow3
            |    Altitude:  xx.x ft             |...yRow4
            |  Satellites:  None                |...yRow5
            |          47.0753, -122.2847       |...yRow6
            |   Waiting for GPS                 |...yRow7?
            |                                   |
            +------------:--:-------------------+
                    labelX  valueX
*/

#include <Arduino.h>
#include "Adafruit_ILI9341.h"         // TFT color display library
#include "constants.h"                // Griduino constants and colors
#include "model.cpp"                  // "Model" portion of model-view-controller
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "view.h"                     // Base class for all views

// ========== extern ===========================================
extern Model* model;                  // "model" portion of model-view-controller

// ========== class ViewStatus =================================
class ViewStatus : public View {
  public:
    // ---------- public interface ----------
    // This derived class must implement the public interface:
    ViewStatus(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);

  protected:
    // ---------- local data for this derived class ----------
    // color scheme: see constants.h

    // vertical placement of text rows
    const int yRow1 = 24;
    const int yRow2 = yRow1 + 32;
    const int yRow3 = yRow2 + 44;
    const int yRow4 = yRow3 + 44;
    const int yRow5 = yRow4 + 32;
    const int yRow6 = yRow5 + 40;
    const int yRow7 = yRow6 + 24;

    const int labelX = 114;               // right-align labels, near their values
    const int valueX = 140;               // left-align values

    // ----- static screen text
    #define nStatusLabels 4
    TextField txtLabels[nStatusLabels] = {
      { "GMT:",        labelX, yRow1, cLABEL, ALIGNRIGHT },
      { "Grid:",       labelX, yRow3, cLABEL, ALIGNRIGHT },
      { "Altitude:",   labelX, yRow4, cLABEL, ALIGNRIGHT },
      { "Satellites:", labelX, yRow5, cLABEL, ALIGNRIGHT },
    };

    // ----- dynamic screen text
    #define nStatusValues 6
    TextField txtValues[nStatusValues] = {
      TextField(valueX, yRow1, cVALUE),     // [0] GMT time
      TextField(valueX, yRow2, cVALUE),     // [1] GMT date
      TextField(valueX, yRow3, cHIGHLIGHT), // [2] Grid6, brighter color because "grid" is important
      TextField(valueX, yRow4, cVALUE),     // [3] Altitude
      TextField(valueX, yRow5, cVALUE),     // [4] Satellites
      TextField(  64,   yRow6, cVALUE),     // [5] lat/long
    };

};  // end class ViewStatus

// ============== implement public interface ================
void ViewStatus::updateScreen() {
  // called on every pass through main()

  setFontSize(12);

  // ----- GMT time
  char sTime[10];                     // strlen("19:54:14") = 8
  model->getTime(sTime);
  txtValues[0].print(sTime);

  // ----- GMT date
  char sDate[15];                     // strlen("Jan 12, 2020") = 13
  model->getDate(sDate, sizeof(sDate));
  txtValues[1].print(sDate);

  // ----- grid square
  char sGrid[10];                     // strlen("CN87us") = 6
  calcLocator(sGrid, model->gLatitude, model->gLongitude, 6);
  txtValues[2].print(sGrid);
  
  // ----- altitude
  float altitude = model->gAltitude*feetPerMeters;
  String sValue = String(altitude, 0) + " ft";
  sValue.trim();                      // remove leading blanks and whitespace
  char sAltitude[12];                 // strlen("12345 ft") = 8
  sValue.toCharArray(sAltitude, sizeof(sAltitude)-1);
  txtValues[3].print(sAltitude);

  // ----- satellites
  uint8_t numSatellites = model->gSatellites;
  uint16_t color;
  char sSatellites[6];                // strlen("none") = 4, strlen("4") = 1
  if (numSatellites > 0) {
    txtValues[4].color = cVALUE;
    txtValues[4].print(numSatellites);
  } else {
    txtValues[4].color = cWARN;
    txtValues[4].print("None");
  }

  // ----- center lat/long on its own row
  char sLatLong[22];                  // strlen("-xxx.xxxx,-yyy.yyyy") = 19
  snprintf(sLatLong, 22, "%.4f, %.4f",
                model->gLatitude, model->gLongitude);
  txtValues[5].print(sLatLong);
}

void ViewStatus::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(cBACKGROUND);     // clear screen
  txtValues[0].setBackground(cBACKGROUND);              // set background for all TextFields in this view
  TextField::setTextDirty( txtLabels, nStatusLabels );
  TextField::setTextDirty( txtValues, nStatusValues );  // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALL);

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showScreenBorder();                 // optionally outline visible area

  // ----- draw text fields
  for (int ii=0; ii<nStatusLabels; ii++) {
    txtLabels[ii].print();
  }

  updateScreen();                     // fill in values immediately, don't wait for the main loop to eventually get around to it

  #ifdef SHOW_SCREEN_CENTERLINE
    // show centerline at      x1,y1              x2,y2             color
    tft->drawLine( tft->width()/2,0,  tft->width()/2,tft->height(), cWARN); // debug
  #endif
}

bool ViewStatus::onTouch(Point touch) {
  Serial.println("->->-> Touched status screen.");
  return false;                       // true=handled, false=controller uses default action
}
