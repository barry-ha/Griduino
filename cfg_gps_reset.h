#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     cfg_gps_reset.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Optionally reset GPS to factory state.
            User interface calls it "cold restart" but really
            the GPS is completely reset to as-shipped condition.

            +-------------------------------------------+
            |  *          GPS Cold Restart           >  |... yRow1
            |                                           |
            |    If your GPS receives no satellites     |... yRow2
            |    for hours, this action might help.     |... yRow3
            |             +---------------+             |... yBtn
            |             |               |             |
            |             |  Restart GPS  |             |... yBtn
            |             |               |             |
            |             +---------------+             |
            |             :                             |
            |  Done. Please power cycle Griduino.       |... yRow8 Confirmation
            | v1.14, Feb 15 2024                        |... yRow9
            +-------------:-----------------------------+
                          :
                          xBtn
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
extern Logger logger;   // Griduino.ino

#include "model_gps.h"   // Model of a GPS for model-view-controller
extern Model *model;

// ========== class ViewCfgGpsReset ================================
class ViewCfgGpsReset : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewCfgGpsReset(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background = cBACKGROUND;   // every view can have its own background color
  }
  void updateScreen();
  void startScreen();
  bool onTouch(Point touch);

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  const int radius = 10;   // rounded corners on buttons

  // vertical placement of text rows
  const int space = 24;
  const int half  = space / 2;

  const int yRow1 = 36;
  const int yRow2 = yRow1 + space + half;
  const int yRow3 = yRow2 + space;
  const int yBtn  = yRow3 + space;                // ul corner of big button
  const int yRow8 = gScreenHeight - 10 - space;   // Confirmation message
  const int yRow9 = gScreenHeight - 10;           // "v1.14, Feb 15 2024"

  // ----- screen text
  // names for the array indexes, must be named in same order as array below
  enum txtIndex {
    TITLE = 0,
    LINE1,
    LINE2,
    NUMSATS,
    CONFIRMATION,
    COMPILED,
  };

  const int bWidth  = 320 / 2;
  const int bHeight = 64;

  const int xBtn     = 320 / 4;
  const int yBtnText = yBtn + (bHeight / 2) + 6;   // vertically center text inside big button

  // ----- static screen text
  // clang-format off
#define nRestartValues 6
  TextField txtStatic[nRestartValues] = {
      //  text                                      x, y      color
      {"GPS Cold Start",                           -1, 20,    cHIGHLIGHT, ALIGNCENTER},                  // [TITLE]
      {"If your GPS receives no satellites",       -1, yRow2, cFAINT,     ALIGNCENTER, eFONTSMALLEST},   // [LINE1]
      {"for hours, this action might help.",       -1, yRow3, cFAINT,     ALIGNCENTER, eFONTSMALLEST},   // [LINE2]
      {"n#",                                 (320-32), yBtnText, cLABEL,  ALIGNRIGHT},                   // [NUMSATS]
      {"Restarting! Please power cycle Griduino.", -1, yRow8, cBACKGROUND, ALIGNCENTER},                 // [CONFIRMATION]
      {PROGRAM_VERDATE,                            -1, yRow9, cFAINT,     ALIGNCENTER, eFONTSMALLEST},   // [COMPILED]
  };

enum buttonID {
  eRESTART,
};
#define nRestartButtons 1
  FunctionButton myButtons[nRestartButtons] = {
    // label              origin         size        {   touch-target     }
    // text                 x,y           w,h        {   x,y        w,h   }, radius, color,      functionID
      {"Cold Start GPS", xBtn,yBtn, bWidth,bHeight, {180,yBtn,  140,110 }, radius, cBUTTONLABEL, eRESTART},
  };
  // clang-format on

  // ---------- local functions for this derived class ----------
  void fRestart() {
    logger.log(CONFIG, INFO, "->->-> Clicked RESTART button.");
    txtStatic[CONFIRMATION].color = cVALUEFAINT;
    txtStatic[CONFIRMATION].dirty = true;
    txtStatic[CONFIRMATION].print();

    model->factoryReset();   // do the thing
  }
  void drawNumSatellites() {
    char sTemp[4];   // strlen("12#") = 3
    snprintf(sTemp, sizeof(sTemp), "%d#", model->gSatellites);
    txtStatic[NUMSATS].print(sTemp);   // number of satellites
  }

};   // end class ViewCfgGpsReset

// ============== implement public interface ================
void ViewCfgGpsReset::updateScreen() {
  // called on every pass through main()
  // nothing to do in the main loop - this screen has no dynamic items
  drawNumSatellites();
}   // end updateScreen

void ViewCfgGpsReset::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                  // clear screen
  txtStatic[0].setBackground(this->background);         // set background for all TextFields in this view
  TextField::setTextDirty(txtStatic, nRestartValues);   // make sure all fields get re-printed on screen change

  drawAllIcons();                                   // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();                        // optionally draw box around default button-touch areas
  showMyTouchTargets(myButtons, nRestartButtons);   // optionally show this view's touch targets
  showScreenBorder();                               // optionally outline visible area
  showScreenCenterline();                           // optionally draw visual alignment bar

  // ----- draw text fields
  for (int ii = 0; ii < nRestartValues; ii++) {
    txtStatic[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii = 0; ii < nRestartButtons; ii++) {
    FunctionButton item = myButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y + item.h / 2 + 5);   // place text centered inside button
    tft->setTextColor(item.color);
    tft->print(item.text);
  }

  showProgressBar(6, 9);   // draw marker for advancing through settings
  updateScreen();          // update UI immediately, don't wait for the main loop to eventually get around to it
}   // end startScreen()

bool ViewCfgGpsReset::onTouch(Point touch) {
  logger.log(CONFIG, INFO, "->->-> Touched GPS restart screen.");
  bool handled = false;   // assume a touch target was not hit
  for (int ii = 0; ii < nRestartButtons; ii++) {
    FunctionButton item = myButtons[ii];
    if (item.hitTarget.contains(touch)) {
      handled = true;               // hit!
      switch (item.functionIndex)   // do the thing
      {
      case eRESTART:
        fRestart();
        break;
      default:
        logger.log(CONFIG, ERROR, "unknown function %d", item.functionIndex);
        break;
      }
    }
  }
  return handled;   // true=handled, false=controller uses default action
}   // end onTouch()
