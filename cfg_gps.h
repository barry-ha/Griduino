#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     cfg_gps.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the 'control panel' for one-time Griduino setup.
            Since it's not intended for a driver in motion, we can use
            a smaller font and cram more stuff onto the screen.

            +-------------------------------------------+
            |                4. GPS                     |
            |                                           |
            | Breadcrumb trail     [ Clear ]            |
            | 123 of 6000                               |
            |                                           |
            | Route             (o)[ GPS Receiver ]     |
            |                   ( )[  Simulator   ]     |
            |                                           |
            |                                           |
            | v1.12, Feb 10 2021                        |... yRow9
            +-------------------------------------------+
*/

#include "constants.h"           // Griduino constants and colors
#include "logger.h"              // conditional printing to Serial port
#include "model_breadcrumbs.h"   // breadcrumb trail
#include "model_gps.h"           // Model of a GPS for model-view-controller
#include "TextField.h"           // Optimize TFT display text for proportional fonts
#include "view.h"                // Base class for all views

// ========== extern ===========================================
extern void showDefaultTouchTargets();   // Griduino.ino
extern void fSetReceiver();              // Griduino.ino
extern void fSetSimulated();             // Griduino.ino
extern int fGetDataSource();             // Griduino.ino

// ========== class ViewCfgGPS ==============================
class ViewCfgGPS : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewCfgGPS(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background = cBACKGROUND;   // every view can have its own background color
  }
  void updateScreen();
  void startScreen();
  bool onTouch(Point touch) override;

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  // vertical placement of text rows   ---label---         ---button---
  const int yRow1 = 74;                   // "Breadcrumb trail", "Clear"
  const int yRow2 = yRow1 + 20;           // "%d of %d"
  const int yRow3 = yRow2 + 52;           // "Route",            "GPS Receiver"
  const int yRow4 = yRow3 + 48;           //                     "Simulator"
  const int yRow9 = gScreenHeight - 10;   // "v1.14, Jan 22 2024"

#define col1    10    // left-adjusted column of text
#define xButton 160   // indented column of buttons

  // names for the array indexes, must be named in same order as array below
  enum txtSettings2 {
    SETTINGS = 0,
    TRAIL,
    TRAILCOUNT,
    GPSTYPE,
    COMPILED,
    PANEL,
  };

  // clang-format off
  #define nTextGPS 5
  TextField txtSettings2[nTextGPS] = {
      //  text                x, y      color
      {"GPS",                -1, 20,    cHIGHLIGHT, ALIGNCENTER},   // [SETTINGS]
      {"Breadcrumb trail", col1, yRow1, cVALUE},                    // [TRAIL]
      {"%d crumbs",        col1, yRow2, cLABEL},                    // [TRAILCOUNT]
      {"Route",            col1, yRow3, cVALUE},                    // [GPSTYPE]
      {PROGRAM_VERDATE,      -1, yRow9, cLABEL, ALIGNCENTER},       // [COMPILED]
  };
  // clang-format on

  enum buttonID {
    eCLEAR,
    eRECEIVER,
    eSIMULATOR,
    eFACTORYRESET,
  };
  // clang-format off
  #define nButtonsGPS 3
  FunctionButton settings2Buttons[nButtonsGPS] = {
      // label              origin             size      touch-target
      // text                 x,y               w,h        x,y       w,h  radius  color   functionID
      {"Clear",        xButton, yRow1 - 14,   130, 40,  {154, 58,  156,54}, 4, cVALUE, eCLEAR},      // [eCLEAR] Clear track history
      {"GPS Receiver", xButton, yRow3 - 26,   130, 40,  {130,112,  180,50}, 4, cVALUE, eRECEIVER},   // [eRECEIVER] Satellite receiver
      {"Simulator",    xButton, yRow4 - 26,   130, 40,  {130,164,  180,50}, 4, cVALUE, eSIMULATOR},  // [eSIMULATOR] Simulated track
    //{"Factory Reset",xButton, 180,          130,30,   {138,174,  180,50}, 4,  cVALUE,  fFactoryReset},// [eFACTORYRESET] Factory Reset
  };
  // clang-format on

  // ---------- local functions for this derived class ----------
  void fClear() {
    logger.log(CONFIG, INFO, "->->-> Clicked CLEAR button.");
    trail.clearHistory();
    trail.rememberPUP();
    trail.deleteFile();               // out with the old history file
    trail.saveGPSBreadcrumbTrail();   // start over with new history file
    logger.log(FILES, INFO, "Breadcrumb trail erased and file deleted");
  }
  void fReceiver() {
    // select GPS receiver data
    logger.log(CONFIG, INFO, "->->-> Clicked GPS RECEIVER button");
    fSetReceiver();   // use "class Model" for GPS receiver hardware
  }
  void fSimulated() {
    // simulate satellite track
    logger.log(CONFIG, INFO, "->->-> Clicked GPS SIMULATOR button");
    fSetSimulated();   // use "class MockModel" for simulated track
  }
  void fFactoryReset() {
    // todo: clear all settings, erase all saved files
    logger.log(CONFIG, INFO, "->->-> Clicked FACTORY RESET button.");
  }

};   // end class ViewCfgGPS

// ============== implement public interface ================
void ViewCfgGPS::updateScreen() {
  // called on every pass through main()

  // ----- fill in replacement string text
  char temp[100];
  snprintf(temp, sizeof(temp), "%d of %d", trail.getHistoryCount(), trail.capacity);
  txtSettings2[TRAILCOUNT].print(temp);

  // ----- show selected radio buttons by filling in the circle
  for (int ii = eRECEIVER; ii <= eSIMULATOR; ii++) {
    FunctionButton item = settings2Buttons[ii];
    int xCenter         = item.x - 16;
    int yCenter         = item.y + (item.h / 2);
    int buttonFillColor = cBACKGROUND;

    if (ii == eRECEIVER && fGetDataSource() == eGPSRECEIVER) {
      buttonFillColor = cLABEL;
    }
    if (ii == eSIMULATOR && fGetDataSource() == eGPSSIMULATOR) {
      buttonFillColor = cLABEL;
    }
    tft->fillCircle(xCenter, yCenter, 4, buttonFillColor);
  }
}   // end updateScreen

void ViewCfgGPS::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);               // clear screen
  txtSettings2[0].setBackground(this->background);   // set background for all TextFields in this view
  TextField::setTextDirty(txtSettings2, nTextGPS);   // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALLEST);

  drawAllIcons();                                      // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();                           // optionally draw box around default button-touch areas
  showMyTouchTargets(settings2Buttons, nButtonsGPS);   // optionally show this view's touch targets
  showScreenBorder();                                  // optionally outline visible area
  showScreenCenterline();                              // optionally draw visual alignment bar

  // ----- draw text fields
  for (int ii = 0; ii < nTextGPS; ii++) {
    txtSettings2[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii = 0; ii < nButtonsGPS; ii++) {
    FunctionButton item = settings2Buttons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y + item.h / 2 + 5);   // place text centered inside button
    tft->setTextColor(item.color);
    tft->print(item.text);
  }

  // ----- draw outlines of radio buttons
  for (int ii = eRECEIVER; ii <= eSIMULATOR; ii++) {
    FunctionButton item = settings2Buttons[ii];
    int xCenter         = item.x - 16;
    int yCenter         = item.y + (item.h / 2);

    // outline the radio button
    // the active button will be indicated in updateScreen()
    tft->drawCircle(xCenter, yCenter, 7, cVALUE);
  }

  showProgressBar(4, 9);   // draw marker for advancing through settings
  updateScreen();          // fill in values immediately, don't wait for the main loop to eventually get around to it

  showScreenCenterline();   // optionally draw alignment bar
}

bool ViewCfgGPS::onTouch(Point touch) {
  logger.log(CONFIG, INFO, "->->-> Touched settings screen.");
  bool handled = false;   // assume a touch target was not hit
  for (int ii = 0; ii < nButtonsGPS; ii++) {
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
        logger.log(CONFIG, ERROR, "unknown function %d", item.functionIndex);
        break;
      }
    }
  }
  return handled;   // true=handled, false=controller uses default action
}   // end onTouch()
