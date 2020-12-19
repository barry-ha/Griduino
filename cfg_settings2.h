/*
  File:     cfg_settings2.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the 'control panel' for one-time Griduino setup.
            Since it's not intended for a driver in motion, we can use 
            a smaller font and cram more stuff onto the screen.

            +-----------------------------------+
            |            Settings 2             |
            |                                   |
            | Breadcrumb trail    [ Clear ]     |
            | 123 of 6000                       |
            |                                   |
            | Route         (o)[ GPS Receiver ] |
            |                  [  Simulator   ] |
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
extern void fSetReceiver();           // Griduino.ino
extern void fSetSimulated();          // Griduino.ino
extern int fGetDataSource();          // Griduino.ino

// ========== class ViewSettings2 ==============================
class ViewSettings2 : public View {
  public:
    // ---------- public interface ----------
    // This derived class must implement the public interface:
    ViewSettings2(Adafruit_ILI9341* vtft, int vid)  // ctor 
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
    const int yRow1 = 70;             // "Breadcrumb trail", "Clear"
    const int yRow2 = yRow1 + 20;     // "%d of %d"
    const int yRow3 = yRow2 + 56;     // "Route",            "GPS Receiver"
    const int yRow4 = yRow3 + 48;     //                     "Simulator"
    const int yRow9 = gScreenHeight - 12; // "v0.27, Nov 03 2020"

    #define col1 10                   // left-adjusted column of text
    #define xButton 160               // indented column of buttons

    enum txtSettings2 {
      SETTINGS=0, 
      TRAIL,   TRAILCOUNT,
      GPSTYPE,
      COMPILED,
    };
    #define nTextGPS 5
    TextField txtSettings2[nTextGPS] = {
      //        text                  x, y     color
      TextField("Settings 2",      col1, 20,   cHIGHLIGHT, ALIGNCENTER),// [SETTINGS]
      TextField("Breadcrumb trail",col1,yRow1, cVALUE),                 // [TRAIL]
      TextField(   "%d crumbs",    col1,yRow2, cLABEL),                 // [TRAILCOUNT]
      TextField("Route",           col1,yRow3, cVALUE),                 // [GPSTYPE]
      TextField(PROGRAM_VERSION ", " PROGRAM_COMPILED, 
                                   col1,yRow9, cLABEL, ALIGNCENTER),    // [COMPILED]
    };

    enum buttonID {
      eCLEAR,
      eRECEIVER,
      eSIMULATOR,
      eFACTORYRESET,
    };
    #define nButtonsGPS 3
    FunctionButton settings2Buttons[nButtonsGPS] = {
      // label             origin         size      touch-target     
      // text                x,y           w,h      x,y      w,h  radius  color   functionID
      {"Clear",        xButton,yRow1-20, 130,40, {130, 40, 180,60},  4,  cVALUE,  eCLEAR    },   // [eCLEAR] Clear track history
      {"GPS Receiver", xButton,yRow3-26, 130,40, {128,112, 180,50},  4,  cVALUE,  eRECEIVER },   // [eRECEIVER] Satellite receiver
      {"Simulator",    xButton,yRow4-26, 130,40, {130,164, 180,60},  4,  cVALUE,  eSIMULATOR},   // [eSIMULATOR] Simulated track
      //{"Factory Reset",xButton,180,    130,30, {138,174, 180,50},  4,  cVALUE,  fFactoryReset},// [eFACTORYRESET] Factory Reset
    };

    // ----- helpers -----
    void fClear() {
      Serial.println("->->-> Clicked CLEAR button.");
      model->clearHistory();
      model->save();
    }
    void fReceiver() {
      // select GPS receiver data
      Serial.println("->->-> Clicked GPS RECEIVER button.");
      fSetReceiver();       // use "class Model" for GPS receiver hardware
    }
    void fSimulated() {
      // simulate satellite track
      Serial.println("->->-> Clicked GPS SIMULATOR button.");
      fSetSimulated();      // use "class MockModel" for simulated track
    }
    void fFactoryReset() {
      // todo: clear all settings, erase all saved files
      Serial.println("->->-> Clicked FACTORY RESET button.");
    }

};  // end class ViewSettings2

// ============== implement public interface ================
void ViewSettings2::updateScreen() {
  // called on every pass through main()

  // ----- fill in replacment string text
  char temp[100];
  snprintf(temp, sizeof(temp), "%d of %d", model->getHistoryCount(), model->numHistory );
  txtSettings2[TRAILCOUNT].print(temp);

  // ----- show selected radio buttons by filling in the circle
  for (int ii=eRECEIVER; ii<=eSIMULATOR; ii++) {
    FunctionButton item = settings2Buttons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);
    int buttonFillColor = cBACKGROUND;

    if (ii==eRECEIVER && fGetDataSource()==eGPSRECEIVER) {
      buttonFillColor = cLABEL;
    } 
    if (ii==eSIMULATOR && fGetDataSource()==eGPSSIMULATOR) {
      buttonFillColor = cLABEL;
    }
    tft->fillCircle(xCenter, yCenter, 4, buttonFillColor);
  }
}


void ViewSettings2::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                  // clear screen
  txtSettings2[0].setBackground(this->background);      // set background for all TextFields in this view
  TextField::setTextDirty( txtSettings2, nTextGPS );    // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALLEST);

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();          // optionally draw boxes around button-touch area
  showScreenBorder();                 // optionally outline visible area

  // ----- draw text fields
  for (int ii=0; ii<nTextGPS; ii++) {
      txtSettings2[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii=0; ii<nButtonsGPS; ii++) {
    FunctionButton item = settings2Buttons[ii];
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
  for (int ii=eRECEIVER; ii<=eSIMULATOR; ii++) {
    FunctionButton item = settings2Buttons[ii];
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


bool ViewSettings2::onTouch(Point touch) {
  Serial.println("->->-> Touched settings screen.");
  bool handled = false;               // assume a touch target was not hit
  for (int ii=0; ii<nButtonsGPS; ii++) {
    FunctionButton item = settings2Buttons[ii];
    if (item.hitTarget.contains(touch)) {
        handled = true;               // hit!
        switch (item.functionIndex)   // do the thing
        {
          case eCLEAR:
              fClear();
              break;
          case eRECEIVER:
              fReceiver();
              break;
          case eSIMULATOR:
              fSimulated();
              break;
          default:
              Serial.print("Error, unknown function "); Serial.println(item.functionIndex);
              break;
        }
     }
  }
  return handled;                     // true=handled, false=controller uses default action
}
