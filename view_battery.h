#pragma once   // Please format this file with clang before check-in to GitHub
/*
   File:    view_battery.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Show the coin battery voltage.
            Warn the user when the battery is getting low.
            The filled-in bars are color-coded:
            green above 2.4 volts
            yellow below 2.4 volts
            red below 1.8 volts

            +-----------------------------------+
            | *     Coin Battery Voltage      > |...yRow1
            |   +---===---+ 3.5                 |...y35, yUR
            |   |         |                     |
            |   |         | 3.0                 |...y30
            |   | ####### |           2.951     |...yVolts
            |   | ####### | 2.5       volts     |...y25
            |   | ####### |                     |
            |   | ####### | 2.0                 |...y20
            |   | ####### |                     |
            |   +---------+ 1.5                 |...y15, yLR
            |                                   |
            |    Jan 01, 2020  01:01:01  GMT    |...yRow9
            +---:---------:-:---------:---------+
                xUL     xLR xV        xVolts
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>   // TFT color display library
#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "grid_helper.h"        // lat/long conversion routines
#include "model_gps.h"          // Model of a GPS for model-view-controller
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
extern Logger logger;                                                                // Griduino.ino
extern Grids grid;                                                                   // grid_helper.h
extern Model *model;                                                                 // "model" portion of model-view-controller
void floatToCharArray(char *result, int maxlen, double fValue, int decimalPlaces);   // Griduino.ino
extern void showDefaultTouchTargets();                                               // Griduino.ino

// ========== class ViewBattery =================================
class ViewBattery : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewBattery(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background = cBACKGROUND;   // every view can have its own background color
  }
  void updateScreen();
  void startScreen();
  bool onTouch(Point touch);

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  // ========== text screen layout ===================================

  // vertical placement of text rows
  const int space = 30;
  const int half  = space / 2;

  const int yRow1 = 18;
  const int y35   = yRow1 + space + half;
  const int y30   = y35 + space;
  const int y25   = y30 + space;
  const int y20   = y25 + space;
  const int y15   = y20 + space;
  const int yRow9 = 226;   // GMT date on bottom row, "226" will match other views

  const int xV = 150;   // left-align values

  const int xVolts = 218;                        // "2.951" voltage display
  const int yVolts = (gScreenHeight / 2) - 50;   //

  const int xUL = 70;   // rectangle of battery icon
  const int yUL = 50;
  const int xLR = 130;
  const int yLR = 184;

  // ----- screen text
  // names for the array indexes, must be named in same order as array below
  enum txtIndex {
    TITLE = 0,
    LABEL_35,
    LABEL_30,
    LABEL_25,
    LABEL_20,
    LABEL_15,
    MEASUREMENT,
    VOLTS,
    GMT_DATE,
    GMT_TIME,
    GMT
  };

  // ----- static + dynamic screen text
  // clang-format off
#define nBatteryValues 11
  TextField txtValues[nBatteryValues] = {
      {"Coin Battery Voltage",-1, yRow1, cTITLE,  ALIGNCENTER,  eFONTSMALLEST}, // [TITLE] view title, centered
      {"3.5",           xV, y35,    cLABEL,  ALIGNLEFT,   eFONTSMALLEST},      // [LABEL_35]
      {"3.0",           xV, y30,    cLABEL,  ALIGNLEFT,   eFONTSMALLEST},      // [LABEL_30]
      {"2.5",           xV, y25,    cLABEL,  ALIGNLEFT,   eFONTSMALLEST},      // [LABEL_25]
      {"2.0",           xV, y20,    cLABEL,  ALIGNLEFT,   eFONTSMALLEST},      // [LABEL_20]
      {"1.5",           xV, y15,    cLABEL,  ALIGNLEFT,   eFONTSMALLEST},      // [LABEL_15]
      {"2.591",     xVolts, yVolts, cVALUE,  ALIGNLEFT,   eFONTSMALL},      // [MEASUREMENT]
      {"volts",     xVolts, yVolts+space, cVALUE, ALIGNLEFT, eFONTSMALL},   // [VOLTS]
      {"Apr 26, 2021", 130, yRow9,  cFAINT,  ALIGNRIGHT,  eFONTSMALLEST},   // [GMT_DATE]
      {"02:34:56",     148, yRow9,  cFAINT,  ALIGNLEFT,   eFONTSMALLEST},   // [GMT_TIME]
      {"GMT",          232, yRow9,  cFAINT,  ALIGNLEFT,   eFONTSMALLEST},   // [GMT]
  };
  // clang-format on

};   // end class ViewBattery

// ============== implement public interface ================
void ViewBattery::updateScreen() {
  // called on every pass through main()

  // update measured voltage
  const float analogRef     = 3.3;    // analog reference voltage
  const uint16_t analogBits = 1024;   // ADC resolution is 10 bits

  int coin_adc       = analogRead(BATTERY_ADC);
  float coin_voltage = (float)coin_adc * analogRef / analogBits;
  char sVolts[12];
  floatToCharArray(sVolts, sizeof(sVolts), coin_voltage, 3);
  txtValues[MEASUREMENT].print(sVolts);

  // ----- GMT date & time
  char sDate[15];   // strlen("Jan 12, 2020") = 13
  char sTime[10];   // strlen("19:54:14") = 8
  model->getDate(sDate, sizeof(sDate));
  model->getTime(sTime);
  txtValues[GMT_DATE].print(sDate);
  txtValues[GMT_TIME].print(sTime);
  txtValues[GMT].print();
}   // end updateScreen

void ViewBattery::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                  // clear screen
  txtValues[0].setBackground(this->background);         // set background for all TextFields in this view
  TextField::setTextDirty(txtValues, nBatteryValues);   // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALL);

  drawAllIcons();              // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();   // optionally draw box around default button-touch areas
  showMyTouchTargets(0, 0);    // no real buttons on this view
  showScreenBorder();          // optionally outline visible area
  showScreenCenterline();      // optionally draw visual alignment bar

  // ----- debug
  char msg[128];
  snprintf(msg, sizeof(msg), "y35=%d, y30=%d, y25=%d, y20=%d, y15=%d",
           y35, y30, y25, y20, y15);
  Serial.println(msg);

  // ----- draw battery representation
  int radius = 6;
  int w      = xLR - xUL;
  int h      = yLR - yUL;
  tft->drawRoundRect(xUL, yUL, w, h, radius, cVALUE);                          // battery outline
  tft->drawLine(xUL + (w / 3), yUL - 1, xUL + (w * 2 / 3), yUL - 1, cVALUE);   // battery +ve terminal

  // ----- draw all fields
  for (int ii = 0; ii < nBatteryValues; ii++) {
    txtValues[ii].print();
  }
  updateScreen();   // update UI immediately, don't wait for the main loop to eventually get around to it
}

bool ViewBattery::onTouch(Point touch) {
  logger.info("->->-> Touched battery screen.");
  return false;   // true=handled, false=controller uses default action
}   // end onTouch()
