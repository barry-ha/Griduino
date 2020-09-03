/*
  File: view_settings2.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the 'control panel' for one-time Griduino setup.
            Since it's NOT intended for a driver in motion, we use a
            much smaller font and cram more stuff onto the screen.

            +-----------------------------------+
            |              Settings 2           |
            |                                   |
            | Breadcrumb trail    [ Clear ]     |
            | 123 of 6000                       |
            |                                   |
            | Route         (o)[ GPS Receiver ] |
            |                  [  Simulator   ] |
            |                                   |
            | Version 0.23                      |
            |     Compiled Sep 02 2020  09:16   |
            +-----------------------------------+
*/

#include <Arduino.h>
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors
#include "model.cpp"                // "Model" portion of model-view-controller
#include "TextField.h"              // Optimize TFT display text for proportional fonts
#include "view.h"                   // Base class for all views

// ========== extern ===========================================
void showNameOfView(String sName, uint16_t fgd, uint16_t bkg);  // Griduino.ino
extern Model* model;                // "model" portion of model-view-controller

void fSetReceiver();                // Griduino.ino
void fSetSimulated();               // Griduino.ino
int fGetDataSource();               // Griduino.ino
void setFontSize(int font);         // Griduino.ino
int getOffsetToCenterTextOnButton(String text, int leftEdge, int width ); // Griduino.ino
void drawAllIcons();                // draw gear (settings) and arrow (next screen) // Griduino.ino
void showScreenBorder();            // optionally outline visible area

// ========== forward reference ================================
void fClear();
void fReceiver();
void fSimulated();
void fFactoryReset();

// ============== constants ====================================
// vertical placement of text rows   ---label---         ---button---
const int yRow1 = 70;             // "Breadcrumb trail", "Clear"
const int yRow2 = yRow1 + 20;     // "%d of %d"
const int yRow3 = yRow2 + 56;     // "Route",            "GPS Receiver"
const int yRow4 = yRow3 + 48;     //                     "Simulator"
const int yRow9 = 234;            // "v0.22, Aug 21 2020 45:67:89"

// color scheme: see constants.h
#define col1 10                   // left-adjusted column of text
#define xButton 160               // indented column of buttons

// ========== globals ==========================================
enum txtSettings2 {
  SETTINGS=0, 
  TRAIL,   TRAILCOUNT,
  GPSTYPE, /*RECEIVER, SIMULATED,*/
  COMPILED,
};
TextField txtSettings2[] = {
  //        text                  x, y     color
  TextField("Settings 2",      col1, 20,   cHIGHLIGHT, ALIGNCENTER),// [SETTINGS]
  TextField("Breadcrumb trail",col1,yRow1, cVALUE),                 // [TRAIL]
  TextField(   "%d crumbs",    col1,yRow2, cLABEL),                 // [TRAILCOUNT]
  TextField("Route",           col1,yRow3, cVALUE),                 // [GPSTYPE]
  TextField(PROGRAM_VERSION ", " PROGRAM_COMPILED, 
                               col1,yRow9, cLABEL, ALIGNCENTER),    // [COMPILED]
};
const int numSettingsFields = sizeof(txtSettings2)/sizeof(txtSettings2[0]);

enum buttonID {
  eCLEAR,
  eRECEIVER,
  eSIMULATOR,
  eFACTORYRESET,
};
TimeButton settings2Buttons[] = {
  // label             origin         size      touch-target     
  // text                x,y           w,h      x,y      w,h  radius  color   function
  {"Clear",        xButton,yRow1-20, 130,40, {130, 40, 180,60},  4,  cVALUE,  fClear    },   // [eCLEAR] Clear track history
  {"GPS Receiver", xButton,yRow3-26, 130,40, {128,112, 180,50},  4,  cVALUE,  fReceiver },   // [eRECEIVER] Satellite receiver
  {"Simulator",    xButton,yRow4-26, 130,40, {130,164, 180,60},  4,  cVALUE,  fSimulated},   // [eSIMULATOR] Simulated track
  //{"Factory Reset",xButton,180, 130,30, {138,174, 180,50},  4,  cVALUE,  fFactoryReset},// [eFACTORYRESET] Factory Reset
};
const int nSettingsButtons = sizeof(settings2Buttons)/sizeof(settings2Buttons[0]);

// ========== helpers ==========================================
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

// ========== class ViewSettings2 ==============================
void ViewSettings2::updateScreen() {
  // called on every pass through main()

  // ----- fill in replacment string text
  char temp[100];
  snprintf(temp, sizeof(temp), "%d of %d", model->getHistoryCount(), model->numHistory );
  txtSettings2[TRAILCOUNT].print(temp);

  // ----- show selected radio buttons by filling in the circle
  for (int ii=eRECEIVER; ii<=eSIMULATOR; ii++) {
    TimeButton item = settings2Buttons[ii];
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
  tft->fillScreen(cBACKGROUND);      // clear screen
  txtSettings2[0].setBackground(cBACKGROUND);                  // set background for all TextFields in this view
  TextField::setTextDirty( txtSettings2, numSettingsFields );  // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALLEST);

  drawAllIcons();                   // draw gear (settings) and arrow (next screen)
  showScreenBorder();               // optionally outline visible area

  // ----- draw text fields
  for (int ii=0; ii<numSettingsFields; ii++) {
      txtSettings2[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii=0; ii<nSettingsButtons; ii++) {
    TimeButton item = settings2Buttons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y+item.h/2+5);
    tft->setTextColor(item.color);
    tft->print(item.text);

    #ifdef SHOW_TOUCH_TARGETS
    tft->drawRect(item.hitTarget.ul.x, item.hitTarget.ul.y,  // debug: draw outline around hit target
                 item.hitTarget.size.x, item.hitTarget.size.y, 
                 cWARN);
    #endif
  }

  // ----- show outlines of radio buttons
  for (int ii=eRECEIVER; ii<=eSIMULATOR; ii++) {
    TimeButton item = settings2Buttons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);

    // outline the radio button
    // the active button will be indicated in updateScreen()
    tft->drawCircle(xCenter, yCenter, 7, cVALUE);
  }

  updateScreen();                     // fill in values immediately, don't wait for the main loop to eventually get around to it

  // ----- label this view in upper left corner
  showNameOfView("Settings 2", cWARN, cBACKGROUND);

  // debug: show centerline on display
  //                        x1,y1            x2,y2            color
  //tft->drawLine(tft->width()/2,0, tft->width()/2,tft->height(), cWARN); // debug
}

bool ViewSettings2::onTouch(Point touch) {
  Serial.println("->->-> Touched settings screen.");
  bool handled = false;             // assume a touch target was not hit
  for (int ii=0; ii<nSettingsButtons; ii++) {
    TimeButton item = settings2Buttons[ii];
    if (touch.x >= item.x && touch.x <= item.x+item.w
     && touch.y >= item.y && touch.y <= item.y+item.h) {
        handled = true;             // hit!
        item.function();            // do the thing

        #ifdef SHOW_TOUCH_TARGETS
          const int radius = 3;     // debug: show where touched
          tft->fillCircle(touch.x, touch.y, radius, cWARN);  // debug - show dot
        #endif
     }
  }
  return handled;                   // true=handled, false=controller uses default action
}
