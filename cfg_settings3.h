/*
  File:     cfg_settings3.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the 'control panel' for one-time Griduino setup.
            Since it's not intended for a driver in motion, we can use 
            a smaller font and cram more stuff onto the screen.

            +-----------------------------------------+
            |              Settings 3                 |
            |                                         |
            | Distance        (o)[ Miles, inHg     ]  |. . .yRow1
            |                 ( )[ Kilometers, hPa ]  |. . .yRow2
            |                                         |
            |                                         |
            |                                         |
            |                                         |
            | v0.35, Feb 25 2021  09:41               |
            +-----------------------------------------+
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>         // TFT color display library
#include "constants.h"                // Griduino constants and colors
#include "model_gps.h"                // Model of a GPS for model-view-controller
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "view.h"                     // Base class for all views

// ========== extern ===========================================
extern Model* model;                  // "model" portion of model-view-controller

extern void showDefaultTouchTargets();// Griduino.ino

// ========== class ViewSettings3 ==============================
class ViewSettings3 : public View {
  public:
    // ---------- public interface ----------
    // This derived class must implement the public interface:
    ViewSettings3(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    {
      background = cBACKGROUND;       // every view can have its own background color
    }
    void updateScreen();
    void startScreen();
    void endScreen();
    bool onTouch(Point touch);

  protected:
    // ---------- local data for this derived class ----------
    // color scheme: see constants.h

    // vertical placement of text rows   ---label---         ---button---
    const int yRow1 = 84;             // "English",          "Miles, inHg"
    const int yRow2 = yRow1 + 50;     // "Metric",           "Kilometers, hPa"
    const int yRow9 = gScreenHeight - 12; // "v0.35, Feb 25 2021"

    #define col1 10                   // left-adjusted column of text
    #define xButton 160               // indented column of buttons
	
    // these are names for the array indexes, must be named in same order as array below
    enum txtSettings3 {
      SETTINGS=0, 
      ENGLISH,
      METRIC,
      COMPILED,
    };

    #define nTextUnits 4
    TextField txtSettings3[nTextUnits] = {
      //        text                  x, y     color
      TextField("3. Units",        col1, 20,   cHIGHLIGHT, ALIGNCENTER),// [SETTINGS]
      TextField("English",         col1,yRow1, cVALUE),                 // [ENGLISH]
      TextField("Metric",          col1,yRow2, cVALUE),                 // [METRIC]
      TextField(PROGRAM_VERSION ", " PROGRAM_COMPILED, 
                                   col1,yRow9, cLABEL, ALIGNCENTER),    // [COMPILED]
    };

    enum buttonID {
      eENGLISH = 0,
      eMETRIC,
    };
    #define nButtonsUnits 2
    FunctionButton settings3Buttons[nButtonsUnits] = {
      // label                  origin         size      touch-target     
      // text                     x,y           w,h      x,y           w,h  radius  color   functionID
      {"Miles, Feet, inHg", xButton,yRow1-26, 150,40, {112,yRow1-36, 204,56},  4,  cVALUE,  eENGLISH },  // [eENGLISH] set units Miles/inHg
      {"Kilometers, hPa",   xButton,yRow2-26, 150,40, {112,yRow2-31, 204,64},  4,  cVALUE,  eMETRIC  },  // [eMETRIC] set units Metric
    };

    // ----- helpers -----
    void setEnglish() {
      Serial.println("->->-> Clicked ENGLISH UNITS button.");
      model->setEnglish();
    }
    void setMetric() {
      Serial.println("->->-> Clicked METRIC UNITS button.");
      model->setMetric();
    }

};  // end class ViewSettings3

// ============== implement public interface ================
void ViewSettings3::updateScreen() {
  // called on every pass through main()

  // ----- show selected radio buttons by filling in the circle
  for (int ii=eENGLISH; ii<=eMETRIC; ii++) {
    FunctionButton item = settings3Buttons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);
    int buttonFillColor = cBACKGROUND;

    if (ii==eENGLISH && !model->gMetric) {
      buttonFillColor = cLABEL;
    } 
    if (ii==eMETRIC && model->gMetric) {
      buttonFillColor = cLABEL;
    }
    tft->fillCircle(xCenter, yCenter, 4, buttonFillColor);
  }
} // end updateScreen


void ViewSettings3::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                  // clear screen
  txtSettings3[0].setBackground(this->background);      // set background for all TextFields in this view
  TextField::setTextDirty( txtSettings3, nTextUnits );  // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALLEST);

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();          // optionally draw box around default button-touch areas
  showScreenBorder();                 // optionally outline visible area
  showScreenCenterline();             // optionally draw visual alignment bar

  // ----- draw text fields
  for (int ii=0; ii<nTextUnits; ii++) {
      txtSettings3[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii=0; ii<nButtonsUnits; ii++) {
    FunctionButton item = settings3Buttons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y+item.h/2+5);  // place text centered inside button
    tft->setTextColor(item.color);
    tft->print(item.text);

    #ifdef SHOW_TOUCH_TARGETS
    tft->drawRect(item.hitTarget.ul.x, item.hitTarget.ul.y,  // debug: draw outline around hit target
                  item.hitTarget.size.x, item.hitTarget.size.y, 
                  cTOUCHTARGET);
    #endif
  }

  // ----- draw outlines of radio buttons
  for (int ii=eENGLISH; ii<=eMETRIC; ii++) {
    FunctionButton item = settings3Buttons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);

    // outline the radio button
    // the active button will be indicated in updateScreen()
    tft->drawCircle(xCenter, yCenter, 7, cVALUE);
  }

  updateScreen();                     // update UI immediately, don't wait for laggy mainline loop
} // end startScreen()


void ViewSettings3::endScreen() {
  // Called once each time this view becomes INactive
  // This is a 'goodbye kiss' to do cleanup work
  // For the english/metric settings view, save our settings here instead of on each 
  // button press because writing to NVR is slow (0.5 sec) and would delay the user
  // while trying to press a button many times in a row.
  saveConfig();
}


bool ViewSettings3::onTouch(Point touch) {
  Serial.println("->->-> Touched settings screen.");
  bool handled = false;               // assume a touch target was not hit
  for (int ii=0; ii<nButtonsUnits; ii++) {
    FunctionButton item = settings3Buttons[ii];
    if (item.hitTarget.contains(touch)) {
        handled = true;               // hit!
        switch (item.functionIndex)   // do the thing
        {
          case eENGLISH:
              setEnglish();
              break;
          case eMETRIC:
              setMetric();
              break;
          default:
              Serial.print("Error, unknown function "); Serial.println(item.functionIndex);
              break;
        }
        updateScreen();               // update UI immediately, don't wait for laggy mainline loop
     }
  }
  return handled;                     // true=handled, false=controller uses default action
}
