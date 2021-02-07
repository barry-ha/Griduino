/*
  File:     view_altimeter.h
  
  Version history: 
            2021-02-06 merged from examples/altimeter into a Griduino view
            2021-01-30 added support for BMP390 and latest Adafruit_BMP3XX library
            2020-05-12 updated TouchScreen code
            2020-03-06 created 0.9

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Display altimeter readings on a 3.2" display. 
            Show comparison of Barometric result to GPS result.
            - Barometer reading requires frequent calibration with a known altitude or sea level pressure.
            - GPS result can vary by a few hundred feet depending on satellites overhead.
            Use the touchscreen to enter yor current sea-level barometer setting.

            +-----------------------------------------+
            | *              Altimeter                |
            |                                         |
            | Barometer:      17.8 feet               |. . .yRow1
            | GPS (5#):      123.4 feet               |. . .yRow2
            |                                         |
            | Enter your current local                |. . .yRow3
            | pressure at sea level:                  |. . .yRow4
            +-------+                         +-------+
            |   ^   |     1016.7 hPa          |   v   |. . .yRow5
            +-------+-------------------------+-------+
*/

#include <Adafruit_ILI9341.h>         // TFT color display library
//#include "TouchScreen.h"              // Touchscreen built in to 3.2" Adafruit TFT display
#include "constants.h"                // Griduino constants and colors
#include "model_gps.h"                // Model of a GPS for model-view-controller
#include "model_baro.h"               // Model of a barometer that stores 3-day history
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "view.h"                     // Base class for all views

// ========== extern ===========================================
extern Model* model;                  // "model" portion of model-view-controller
extern BarometerModel baroModel;      // singleton instance of the barometer model

extern void showDefaultTouchTargets();// Griduino.ino

// ========== class ViewAltimeter ==============================
class ViewAltimeter : public View {
  public:
    // ---------- public interface ----------
    // This derived class must implement the public interface:
    ViewAltimeter(Adafruit_ILI9341* vtft, int vid)  // ctor 
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

    float gSeaLevelPressure = 1017.4;   // default starting value; adjustable by touchscreen
    float inchesHg;
    float Pa;
    float hPa;
    float feet;
    float tempF;
    float tempC;
    float altMeters;
    float altFeet;

    void showReadings();
    void drawButtons();

    // ========== text screen layout ===================================

    // vertical placement of text rows   ---label---         ---button---
    const int yRow1 = 84;             // barometer's altitude
    const int yRow2 = yRow1 + 40;     // GPS's altitude
    const int yRow3 = 172;
    const int yRow4 = yRow3 + 20;
    const int yRow9 = gScreenHeight - 14; // sea level pressure

    const int col1 = 10;              // left-adjusted column of text
    const int col2 = 200;             // right-adjusted numbers
    const int col3 = col2 + 10;
    const int colCenter = 160;        // half of screen width
	
    // these are names for the array indexes, must be named in same order as array below
    enum txtIndex {
      eTitle=0,
      BARO_LABEL, BARO_VALUE, BARO_UNITS,
      GPS_LABEL,  GPS_VALUE,  GPS_UNITS,
	  PROMPT1, PROMPT2,
	  SEA_LEVEL,
    };
    #define nTextAltimeter 10
    TextField txtAltimeter[nTextAltimeter] = {
      //        text                  x, y     color
      TextField("Altitude",        col1, 20,   cTITLE, ALIGNCENTER, eFONTSMALLEST),// [eTitle]
      TextField("Barometer:",      col1,yRow1, cLABEL, ALIGNLEFT, eFONTSMALL),  // [BARO_LABEL]
      TextField("12.3",            col2,yRow1, cVALUE, ALIGNRIGHT,eFONTSMALL),  // [BARO_VALUE]
      TextField("feet",            col3,yRow1, cLABEL, ALIGNLEFT, eFONTSMALL),  // [BARO_UNITS]
      TextField("GPS (4#):",       col1,yRow2, cLABEL, ALIGNLEFT, eFONTSMALL),  // [GPS_LABEL]
      TextField("4567.8",          col2,yRow2, cVALUE, ALIGNRIGHT,eFONTSMALL),  // [GPS_VALUE]
      TextField("feet",            col3,yRow2, cLABEL, ALIGNLEFT, eFONTSMALL),  // GPS_UNITS]
      TextField("Enter your current local", col1,yRow3, cLABEL, ALIGNLEFT, eFONTSMALLEST),// [PROMPT1]
      TextField("pressure at sea level:",   col1,yRow4, cLABEL, ALIGNLEFT, eFONTSMALLEST),// [PROMPT2]
      TextField("1016.7 hPa", colCenter,yRow9, cVALUE, ALIGNCENTER, eFONTSMALLEST),       // [SEA_LEVEL]
    };

    enum buttonID {
      ePressurePlus,
      ePressureMinus,
    };
    #define nPressureButtons 2
    FunctionButton pressureButtons[nPressureButtons] = {
      // For "sea level pressure" we have rather small modest +/- buttons, meant to visually
      // fade a little into the background. However, we want touch-targets larger than the
      // button's outlines to make them easy to press.
      //
      // 3.2" display is 320 x 240 pixels, landscape, (y=239 reserved for activity bar)
      //
      // label  origin   size      touch-target
      // text    x,y      w,h      x,y      w,h   radius  color     functionID
      {  "+",   60,202,  38,32, {  1,158, 159,89},  4,  cTEXTCOLOR, ePressurePlus  },  // Up
      {  "-",  226,202,  38,32, {161,158, 159,89},  4,  cTEXTCOLOR, ePressureMinus },  // Down
    };

    // ----- screen layout
    // When using default system fonts, screen pixel coordinates will identify top left of character cell
    const int xLabel = 8;                 // indent labels, slight margin on left edge of screen

    // ======== barometer and temperature helpers ==================
    /* *****
    void getBaroData() {
      if (!baro.performReading()) {
        Serial.println("Error, failed to read barometer");
      }
      // continue anyway, for demo
      tempC = baro.temperature;
      tempF= tempC * 9 / 5 + 32;
      Pa = baro.pressure;
      hPa = Pa / 100;
      inchesHg = 0.0002953 * Pa;
      altMeters = baro.readAltitude(gSeaLevelPressure);
      altFeet = altMeters * FEET_PER_METER;
    }
    ***** */

    // ----- helpers -----
    void increaseSeaLevelPressure() {
      gSeaLevelPressure += 0.1;
    }

    void decreaseSeaLevelPressure() {
      gSeaLevelPressure -= 0.1;
    }

};  // end class ViewAltimeter

// ========== screen helpers ===================================
void ViewAltimeter::showReadings() {
  // called on every pass through main()

  // todo

}

void ViewAltimeter::drawButtons() {

  // todo

}

// ============== implement public interface ================
void ViewAltimeter::updateScreen() {

  // todo

} // end updateScreen


void ViewAltimeter::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                  // clear screen
  txtAltimeter[0].setBackground(this->background);      // set background for all TextFields in this view
  TextField::setTextDirty( txtAltimeter, nTextAltimeter );  // make sure all fields get re-printed on screen change
  //setFontSize(eFONTSMALLEST);

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();          // optionally draw boxes around button-touch area
  showMyTouchTargets(pressureButtons, nPressureButtons);
  showScreenBorder();                 // optionally outline visible area
  showScreenCenterline();             // optionally draw visual alignment bar

  // ----- draw text fields
  for (int ii=0; ii<nTextAltimeter; ii++) {
      txtAltimeter[ii].print();
  }

  // ----- draw buttons
  //setFontSize(12);
  for (int ii=0; ii<nPressureButtons; ii++) {
    FunctionButton item = pressureButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y+item.h/2+5);
    tft->setTextColor(item.color);
    tft->print(item.text);
  }

  updateScreen();                     // update UI immediately, don't wait for laggy mainline loop
} // end startScreen()


bool ViewAltimeter::onTouch(Point touch) {
  Serial.println("->->-> Touched altimeter screen.");
  bool handled = false;               // assume a touch target was not hit
  for (int ii=0; ii<nPressureButtons; ii++) {
    FunctionButton item = pressureButtons[ii];
    if (item.hitTarget.contains(touch)) {
        handled = true;               // hit!
        switch (item.functionIndex)   // do the thing
        {
          case ePressurePlus:
              increaseSeaLevelPressure();
              break;
          case ePressureMinus:
              decreaseSeaLevelPressure();
              break;
          default:
              Serial.print("Error, unknown function "); Serial.println(item.functionIndex);
              break;
        }
        updateScreen();               // show the result
     }
  }
  return handled;                     // true=handled, false=controller uses default action
} // end onTouch()
