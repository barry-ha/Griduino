/*
  File: view_settings.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the 'control panel' for one-time Griduino setup.
            Since it's NOT intended for a driver in motion, we use a
            much smaller font and cram more stuff onto the screen.

            +-----------------------------------+
            | Settings                          |
            |                                   |
            | Breadcrumb trail          [Clear] |
            |     320 / 400 places              |
            | GPS                 [x Receiver ] |
            |                     [  Simulated] |
            |                                   |
            | Version 0.17                      |
            |     Compiled Jun 23 2020  20:59   |
            +-----------------------------------+
*/

#include <Arduino.h>
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors
#include "model.cpp"                // "Model" portion of model-view-controller
#include "TextField.h"              // Optimize TFT display text for proportional fonts

// ========== extern ===========================================
extern Adafruit_ILI9341 tft;        // Griduino.ino
extern Model* model;                // "model" portion of model-view-controller

void fSetReceiver();                // Griduino.ino
void fSetSimulated();               // Griduino.ino
int fGetDataSource();               // Griduino.ino
void setFontSize(int font);         // Griduino.ino
int getOffsetToCenterTextOnButton(String text, int leftEdge, int width ); // Griduino.ino
void drawAllIcons();                // draw gear (settings) and arrow (next screen) // Griduino.ino

// ========== forward reference ================================
void fClear();
void fReceiver();
void fSimulated();
void fFactoryReset();

// ============== constants ====================================

// color scheme: see constants.h
#define col1 10                     // left-adjusted column of text
#define xButton 160                 // indented column of buttons

enum txtSettings {
  SETTINGS=0, 
  TRAIL,   TRAILCOUNT,
  GPSTYPE, /*RECEIVER, SIMULATED,*/
  COMPILED,
};
TextField txtSettings[] = {
  //        text                   x,y   color
  TextField("Settings",        col1, 20, cHIGHLIGHT, ALIGNCENTER),// [SETTINGS]
  TextField("Breadcrumb trail",col1, 50, cVALUE),                 // [TRAIL]
  TextField(   "%d crumbs",    col1, 70, cLABEL),                 // [TRAILCOUNT]
  TextField("Route",           col1,120, cVALUE),                 // [GPSTYPE]
  //TextField("All settings",  col1,200, cVALUE),                 // [GPSTYPE]
  TextField(PROGRAM_VERSION ", " PROGRAM_COMPILED, 
                               col1,234, cLABEL, ALIGNCENTER),    // [COMPILED]
};
const int numSettingsFields = sizeof(txtSettings)/sizeof(txtSettings[0]);

enum buttonID {
  eCLEAR,
  eRECEIVER,
  eSIMULATOR,
  eFACTORYRESET,
};
TimeButton settingsButtons[] = {
  // label             origin     size      touch-target     
  // text                x,y       w,h      x,y      w,h  radius  color   function
  {"Clear",        xButton, 30,  130,30, {140, 20, 180,50},  4,  cVALUE,  fClear    },   // [eCLEAR] Clear track history
  {"GPS Receiver", xButton,100,  130,30, {138, 84, 180,46},  4,  cVALUE,  fReceiver },   // [eRECEIVER] Satellite receiver
  {"Simulator",    xButton,130,  130,30, {140,130, 180,47},  4,  cVALUE,  fSimulated},   // [eSIMULATOR] Simulated track
  //{"Factory Reset",xButton,180,  130,30, {138,174, 180,50},  4,  cVALUE,  fFactoryReset},// [eFACTORYRESET] Factory Reset
};
const int nSettingsButtons = sizeof(settingsButtons)/sizeof(settingsButtons[0]);

// ========== settings helpers =================================
void fClear() {
  Serial.println("->->-> Clicked CLEAR button.");
  model->clearHistory();
  model->save();
}
void fReceiver() {
  // todo: select GPS receiver data
  Serial.println("->->-> Clicked GPS RECEIVER button.");
  fSetReceiver();       // use "class Model" for GPS receiver hardware
}
void fSimulated() {
  // todo: simulate satellite track
  Serial.println("->->-> Clicked GPS SIMULATOR button.");
  fSetSimulated();      // use "class MockModel" for simulated track
}
void fFactoryReset() {
  // todo: simulate satellite track
  Serial.println("->->-> Clicked FACTORY RESET button.");
}

// ========== settings screen view =============================
void updateSettingsScreen() {
  // called on every pass through main()

  // ----- fill in replacment string text
  char temp[100];
  snprintf(temp, sizeof(temp), "%d of %d", model->getHistoryCount(), model->numHistory );
  txtSettings[TRAILCOUNT].print(temp);

  // ----- show selected radio buttons by filling in the circle
  for (int ii=eRECEIVER; ii<=eSIMULATOR; ii++) {
    TimeButton item = settingsButtons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);
    int buttonFillColor = cBACKGROUND;

    if (ii==eRECEIVER && fGetDataSource()==eGPSRECEIVER) {
      buttonFillColor = cLABEL;
    } 
    if (ii==eSIMULATOR && fGetDataSource()==eGPSSIMULATOR) {
      buttonFillColor = cLABEL;
    }
    tft.fillCircle(xCenter, yCenter, 4, buttonFillColor);
  }

}
void startSettingsScreen() {
  // called once each time this view becomes active
  tft.fillScreen(cBACKGROUND);      // clear screen
  txtSettings[0].setBackground(cBACKGROUND);                  // set background for all TextFields in this view
  TextField::setTextDirty( txtSettings, numSettingsFields );  // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALLEST);

  #ifdef SHOW_SCREEN_BORDER
    tft.drawRect(0, 0, gScreenWidth, gScreenHeight, ILI9341_BLUE);  // debug: border around screen
  #endif

  drawAllIcons();                   // draw gear (settings) and arrow (next screen)

  // ----- draw text fields
  for (int ii=0; ii<numSettingsFields; ii++) {
      txtSettings[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii=0; ii<nSettingsButtons; ii++) {
    TimeButton item = settingsButtons[ii];
    tft.fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft.drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);
    tft.setCursor(xx, item.y+item.h/2+5);
    tft.setTextColor(item.color);
    tft.print(item.text);

    #ifdef SHOW_TOUCH_TARGETS
    tft.drawRect(item.hitTarget.ul.x, item.hitTarget.ul.y,  // debug: draw outline around hit target
                 item.hitTarget.size.x, item.hitTarget.size.y, 
                 cWARN);
    #endif
  }

  // ----- show outlines of radio buttons
  for (int ii=eRECEIVER; ii<=eSIMULATOR; ii++) {
    TimeButton item = settingsButtons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);

    // outline the radio button
    // the active button will be indicated in updateSettingsScreen()
    tft.drawCircle(xCenter, yCenter, 7, cVALUE);
  }

  updateSettingsScreen();             // fill in values immediately, don't wait for the main loop to eventually get around to it

  // debug: show centerline on display
  //tft.drawLine(tft.width()/2,0, tft.width()/2,tft.height(), cWARN); // debug
}
bool onTouchSettings(Point touch) {
  Serial.println("->->-> Touched settings screen.");
  bool handled = false;             // assume a touch target was not hit
  for (int ii=0; ii<nSettingsButtons; ii++) {
    TimeButton item = settingsButtons[ii];
    if (touch.x >= item.x && touch.x <= item.x+item.w
     && touch.y >= item.y && touch.y <= item.y+item.h) {
        handled = true;             // hit!
        item.function();            // do the thing

        #ifdef SHOW_TOUCH_TARGETS
          const int radius = 3;     // debug: show where touched
          tft.fillCircle(touch.x, touch.y, radius, cWARN);  // debug - show dot
        #endif
     }
  }
  return handled;                   // true=handled, false=controller uses default action
}
