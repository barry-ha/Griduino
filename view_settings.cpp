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
            | Breadcrumb trail                  |
            |     Storing 400 places    [Clear] |
            | GPS                               |
            |     [X] Receiver    [ ] Simulated |
            |                                   |
            |                                   |
            | Version 1.10                      |
            |     Compiled Jan 1, 2020  01:23   |
            +-----------------------------------+
*/

#include <Arduino.h>
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors
#include "model.cpp"                // "Model" portion of model-view-controller
#include "TextField.h"              // Optimize TFT display text for proportional fonts

// ========== extern ===========================================
extern Adafruit_ILI9341 tft;        // Griduino.ino
extern Model model;                 // "model" portion of model-view-controller

void setFontSize(int font);         // Griduino.ino

// ============== constants ====================================

// color scheme: see constants.h
#define col1 10                     // left-adjusted column of text
#define indent 40                   // indented column of text

enum txtSettings {
  SETTINGS=0, 
  TRAIL,   CLEARTRAIL,
  GPSTYPE, RECEIVER, SIMULATED,
  VERSION, COMPILED,
};
TextField txtSettings[] = {
  //        text                             x,y      color
  TextField("Settings",                   col1,   20, cHIGHLIGHT),// [SETTINGS]
  TextField("Breadcrumb trail, %d places",col1,   60, cLABEL),    // [TRAIL]
  TextField(   "[ Clear ]",               indent, 80, cVALUE),    // [CLEARTRAIL] todo: make this a button
  TextField("GPS",                        col1,  120, cLABEL),    // [GPSTYPE]
  TextField(   "[o] Receiver",            indent,140, cVALUE),    // [RECEIVER]   todo: make this a button
  TextField(   "[  ] Simulated",      indent+140,140, cVALUE),    // [SIMULATED]  todo: make this a button
  TextField("Version " PROGRAM_VERSION,   col1,  210, cLABEL),    // [VERSION]
  TextField("Compiled " PROGRAM_COMPILED, col1,  230,cLABEL),     // [COMPILED]
};
const int numSettingsFields = sizeof(txtSettings)/sizeof(txtSettings[0]);

// ========== helpers ==========================================

// ========== splash screen view ===============================
void updateSettingsScreen() {
  // called on every pass through main()
  // nothing to do in the main loop - this screen has no dynamic items
}
void startSettingsScreen() {
  // called once each time this view becomes active
  tft.fillScreen(cBACKGROUND);      // clear screen
  txtSettings[0].setBackground(cBACKGROUND);                // set background for all TextFields in this view
  TextField::setTextDirty( txtSettings, numSettingsFields );  // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALLEST);

  // fill in replacment strings
  char temp[100];
  snprintf(temp, sizeof(temp), "Breadcrumb trail, %d places", model.getHistoryCount() );
  txtSettings[TRAIL].print(temp);

  for (int ii=0; ii<numSettingsFields; ii++) {
      txtSettings[ii].print();
  }
  
  updateSettingsScreen();             // fill in values immediately, don't wait for the main loop to eventually get around to it
}
bool onTouchSettings(Point touch) {
  Serial.println("->->-> Touched splash screen.");
  return false;                     // true=handled, false=controller uses default action
}
