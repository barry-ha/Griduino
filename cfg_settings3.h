/*
  File:     cfg_settings3.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the 'control panel' for one-time Griduino setup.
            Since it's not intended for a driver in motion, we can use 
            a smaller font and cram more stuff onto the screen.

            +-----------------------------------+
            |            Settings 3             |
            |                                   |
            | Distance      (o)[ Miles        ] |
            |               ( )[ Kilometers   ] |
            |                                   |
            |                                   |
            |                                   |
            |                                   |
            | v0.28, Dec 19 2020  07:56         |
            +-----------------------------------+
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>         // TFT color display library
#include "constants.h"                // Griduino constants and colors
#include "model_gps.h"                // "Model" portion of model-view-controller
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
    bool onTouch(Point touch);

  protected:
    // ---------- local data for this derived class ----------
    // color scheme: see constants.h

    // vertical placement of text rows   ---label---         ---button---
    const int yRow1 = 70;             // "Distance",         "Miles"
    const int yRow2 = yRow1 + 50;     //                     "Kilometers"
    const int yRow9 = gScreenHeight - 12; // "v0.27, Oct 31 2020"

    #define col1 10                   // left-adjusted column of text
    #define xButton 160               // indented column of buttons

    enum txtSettings3 {
      SETTINGS=0, 
      DISTANCE,
      COMPILED,
    };
    #define nTextUnits 3
    TextField txtSettings3[nTextUnits] = {
      //        text                  x, y     color
      TextField("Settings 3",      col1, 20,   cHIGHLIGHT, ALIGNCENTER),// [SETTINGS]
      TextField("Distance",        col1,yRow1, cVALUE),                 // [DISTANCE]
      TextField(PROGRAM_VERSION ", " PROGRAM_COMPILED, 
                                   col1,yRow9, cLABEL, ALIGNCENTER),    // [COMPILED]
    };

    // ----- helpers -----
    void fMiles() {
      Serial.println("->->-> Clicked DISTANCE MILES button.");
      model->setMiles();
    }
    void fKilometers() {
      Serial.println("->->-> Clicked DISTANCE KILOMETERS button.");
      model->setKilometers();
    }

    enum buttonID {
      eMILES = 0,
      eKILOMETERS,
    };
    #define nButtonsUnits 2
    FunctionButton settings3Buttons[nButtonsUnits] = {
      // label             origin         size      touch-target     
      // text                x,y           w,h      x,y      w,h  radius  color   function
      {"Miles",        xButton,yRow1-26, 130,40, {130, 30, 180,60},  4,  cVALUE,  eMILES      },   // [eMILES] set units English
      {"Kilometers",   xButton,yRow2-26, 130,40, {130, 90, 180,60},  4,  cVALUE,  eKILOMETERS },   // [eKILOMETERS] set units Metric
    };

};  // end class ViewSettings3

// ============== implement public interface ================
void ViewSettings3::updateScreen() {
  // called on every pass through main()

  // ----- show selected radio buttons by filling in the circle
  for (int ii=eMILES; ii<=eKILOMETERS; ii++) {
    FunctionButton item = settings3Buttons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);
    int buttonFillColor = cBACKGROUND;

    if (ii==eMILES && !model->gMetric) {
      buttonFillColor = cLABEL;
    } 
    if (ii==eKILOMETERS && model->gMetric) {
      buttonFillColor = cLABEL;
    }
    tft->fillCircle(xCenter, yCenter, 4, buttonFillColor);
  }
}


void ViewSettings3::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                  // clear screen
  txtSettings3[0].setBackground(this->background);      // set background for all TextFields in this view
  TextField::setTextDirty( txtSettings3, nTextUnits );  // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALLEST);

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();          // optionally draw boxes around button-touch area
  showScreenBorder();                 // optionally outline visible area

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
  for (int ii=eMILES; ii<=eKILOMETERS; ii++) {
    FunctionButton item = settings3Buttons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);

    // outline the radio button
    // the active button will be indicated in updateScreen()
    tft->drawCircle(xCenter, yCenter, 7, cVALUE);
  }

  updateScreen();                     // fill in values immediately, don't wait for the main loop to eventually get around to it

  #ifdef SHOW_SCREEN_CENTERLINE
    // show centerline at      x1,y1              x2,y2             color
    tft->drawLine( tft->width()/2,0,  tft->width()/2,tft->height(), cWARN); // debug
  #endif
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
          case eMILES:
              fMiles();
              break;
          case eKILOMETERS:
              fKilometers();
              break;
          default:
              Serial.print("Error, unknown function "); Serial.println(item.functionIndex);
              break;
        }
        updateScreen();               // update UI immediately, don't wait for laggy mainline loop
        this->saveConfig();           // after UI is updated, save setting to nvr
     }
  }
  return handled;                     // true=handled, false=controller uses default action
}
