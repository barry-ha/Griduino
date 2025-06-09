#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     view_time.h

  GMT_clock - bright colorful Greenwich Mean Time based on GPS

  Version history:
            2021-07-11 fixed saving local time zone offset
            2020-10-15 refactored from .cpp to .h
            2020-04-22 created
            2020-04-29 added touch adjustment of local time zone
            2020-05-01 added save/restore to nonvolatile RAM
            2020-05-12 updated TouchScreen code
            2020-05-13 proportional fonts
            2020-06-02 merged from standalone example into Griduino view

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is a bright colorful GMT clock for your shack
            or dashboard. After all, once a rover arrives at a
            destination they no longer need grid square navigation.

            +-----------------------------------------+
            |            Griduino GMT Clock           |
            |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
            |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
            |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
            |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
            |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
            |                                         |
            |         GMT Date: Dec 19, 2020          |
            |                                         |
            |         Internal Temp:  101.7 F         |
            |                                         |
            +-------+                         +-------+
            | +1 hr |    Local: HH:MM:SS      | -1 hr |
            +-------+-------------------------+-------+

*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "model_gps.h"          // Model of a GPS for model-view-controller
#include "save_restore.h"       // Save configuration in non-volatile RAM
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
extern Logger logger;   // Griduino.ino
extern Model *model;    // "model" portion of model-view-controller

extern void showDefaultTouchTargets();           // Griduino.ino
extern void getDate(char *result, int maxlen);   // model_gps.h

// ========== class ViewTime ===================================
class ViewTime : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewTime(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background = cBACKGROUND;   // every view can have its own background color
  }
  void updateScreen();
  void startScreen();
  void endScreen();
  bool onTouch(Point touch);
  // void loadConfig();
  // void saveConfig();

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  // ========== text screen layout ===================================

  // vertical placement of text rows
  const int yRow9 = 226;   // aligned vertically wqith + and - buttons

  // ----- screen text
  // names for the array indexes, must be named in same order as array below
  enum txtIndex {
    TITLE = 0,
    HOURS,
    COLON1,
    MINUTES,
    COLON2,
    SECONDS,
    GMTDATE,
    DEGREES,
    LOCALTIME,
    TIMEZONE,
    NUMSATS,
  };

  // ----- static + dynamic screen text
  // clang-format off
#define numClockFields 11
  TextField txtClock[numClockFields] = {
      // text            x,y      color       align       font
      {"Griduino GMT", -1,  18,   cTITLE,  ALIGNCENTER, eFONTSMALLEST},   // [TITLE]   program title, centered
      {"hh",           12,  94,   cVALUE,  ALIGNLEFT,   eFONTGIANT},      // [HOURS]     giant clock hours
      {":",            94,  94,   cVALUE,  ALIGNLEFT,   eFONTGIANT},      // [COLON1]    :
      {"mm",          120,  94,   cVALUE,  ALIGNLEFT,   eFONTGIANT},      // [MINUTES]   giant clock minutes
      {":",           204,  94,   cVALUE,  ALIGNLEFT,   eFONTGIANT},      // [COLON2]    :
      {"ss",          230,  94,   cVALUE,  ALIGNLEFT,   eFONTGIANT},      // [SECONDS]   giant clock seconds
      {"MMM dd, yyyy", -1, 140,   cVALUE,  ALIGNCENTER, eFONTSMALL},      // [GMTDATE]   GMT date
      {"",             -1, 174,   cVALUE,  ALIGNCENTER, eFONTSMALL},      // [DEGREES]   Temperature e.g. "12.3 F"
      {"hh:mm:ss",    118, yRow9, cTEXTCOLOR,ALIGNLEFT, eFONTSMALL},      // [LOCALTIME] Local time
      {"-7h",           8, yRow9, cFAINT,  ALIGNLEFT,   eFONTSMALLEST},   // [TIMEZONE]  addHours time zone
      {"6#",          308, yRow9, cFAINT,  ALIGNRIGHT,  eFONTSMALLEST},   // [NUMSATS]   numSats
  };
  // clang-format on

  enum buttonID {
    etimeZonePlus,
    etimeZoneMinus,
  };
#define nTimeButtons 2
  FunctionButton timeButtons[nTimeButtons] = {
      // For "GMT_clock" we have rather small modest +/- buttons, meant to visually
      // fade a little into the background. However, we want larger touch-targets to
      // make them easy to press.
      //
      // 3.2" display is 320 x 240 pixels, landscape, (y=239 reserved for activity bar)
      //
      // label  origin   size      touch-target
      // text    x,y      w,h      x,y      w,h   radius  color     functionID
      {"+", 66, 204, 36, 30, {30, 180, 110, 59}, 4, cTEXTCOLOR, etimeZonePlus},      // Up
      {"-", 226, 204, 36, 30, {190, 180, 110, 59}, 4, cTEXTCOLOR, etimeZoneMinus},   // Down
  };

};   // end class ViewTime

// ============== implement public interface ================
void ViewTime::updateScreen() {
  // called on every pass through main()

  // GMT Time
  char sHour[8], sMinute[8], sSeconds[8];
  snprintf(sHour, sizeof(sHour), "%02d", GPS.hour);
  snprintf(sMinute, sizeof(sMinute), "%02d", GPS.minute);
  snprintf(sSeconds, sizeof(sSeconds), "%02d", GPS.seconds);

  txtClock[HOURS].print(sHour);
  txtClock[MINUTES].print(sMinute);
  txtClock[SECONDS].print(sSeconds);
  txtClock[COLON2].dirty = true;   // re-draw COLON2 because it was erased by SECONDS field
  txtClock[COLON2].print();

  // GMT Date
  char sDate[16];   // strlen("Jan 12, 2020 ") = 14
  model->getDate(sDate, sizeof(sDate));
  txtClock[GMTDATE].print(sDate);

  // Hours to add/subtract from GMT for local time
  char sign[2] = {0, 0};                              // prepend a plus-sign when >=0
  sign[0]      = (model->gTimeZone >= 0) ? '+' : 0;   // (don't need to add a minus-sign bc the print stmt does that for us)
  char sTimeZone[6];                                  // strlen("-10h") = 4
  snprintf(sTimeZone, sizeof(sTimeZone), "%s%dh", sign, model->gTimeZone);
  txtClock[TIMEZONE].print(sTimeZone);

  // Local Time
  char sTime[10];   // strlen("01:23:45") = 8
  model->getTimeLocal(sTime, sizeof(sTime));
  txtClock[LOCALTIME].print(sTime);

  // Satellite Count
  char sBirds[8];   // strlen("5#") = 2
  snprintf(sBirds, sizeof(sBirds), "%d#", model->gSatellites);
  // change colors by number of birds
  txtClock[NUMSATS].color = (model->gSatellites < 1) ? cWARN : cFAINT;
  txtClock[NUMSATS].print(sBirds);
  // txtClock[NUMSATS].dump();         // debug
}   // end updateScreen

void ViewTime::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                 // clear screen
  txtClock[0].setBackground(this->background);         // set background for all TextFields in this view
  TextField::setTextDirty(txtClock, numClockFields);   // make sure all fields get re-printed on screen change

  drawAllIcons();              // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();   // optionally draw box around default button-touch areas
  showMyTouchTargets(timeButtons, nTimeButtons);
  showScreenBorder();       // optionally outline visible area
  showScreenCenterline();   // optionally draw visual alignment bar

  // ----- draw page title
  txtClock[TITLE].print();

  // ----- draw giant fields
  setFontSize(36);
  for (int ii = HOURS; ii <= SECONDS; ii++) {
    txtClock[ii].print();
  }

  // ----- draw text fields
  setFontSize(eFONTSMALLEST);
  for (int ii = GMTDATE; ii < numClockFields; ii++) {
    txtClock[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALL);
  for (int ii = 0; ii < nTimeButtons; ii++) {
    FunctionButton item = timeButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y + item.h / 2 + 5);   // place text centered inside button
    tft->setTextColor(item.color);
    tft->print(item.text);
  }

  updateScreen();   // update UI immediately, don't wait for laggy mainline loop
}   // end startScreen()

void ViewTime::endScreen() {
  // Called once each time this view becomes INactive
  // This is a 'goodbye kiss' to do cleanup work
  // For the GMT time view, the local timezone is part of the model.
  // We save our settings here instead of on each button press
  // because writing to NVR is slow (0.5 sec) and would delay the user
  // while trying to press a button many times in a row.
  //model->save();    // BUT! this slows down showing the next view
  // TODO: schedule the "model->save" until some idle time with no 
  //       user input, say 10 seconds of no touches
}

bool ViewTime::onTouch(Point touch) {
  logger.log(CONFIG, INFO, "->->-> Touched time screen.");

  bool handled = false;   // assume a touch target was not hit
  for (int ii = 0; ii < nTimeButtons; ii++) {
    FunctionButton item = timeButtons[ii];
    if (item.hitTarget.contains(touch)) {
      handled = true;               // hit!
      switch (item.functionIndex)   // do the thing
      {
      case etimeZonePlus:
        model->timeZonePlus();
        break;
      case etimeZoneMinus:
        model->timeZoneMinus();
        break;
      default:
        logger.log(CONFIG, ERROR, "unknown function %d", item.functionIndex);
        break;
      }
      updateScreen();   // show the result
    }
  }
  return handled;   // true=handled, false=controller uses default action
}   // end onTouch()
