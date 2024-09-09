#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     cfg_nmea.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is one of the 'control panel' screens for Griduino setup.
            Since it's not intended for a driver in motion, we can use
            a smaller font and cram more stuff onto the screen.

            +-------------------------------------------+
            |  *          NMEA Broadcasting          >  |
            |                                           |
            | Send GPS position   ( )[ Send NMEA ]      |... yRow1
            | position to                               |... yRow2
            | USB port:           (o)[ None      ]      |... yRow3
            |                                           |
            | NMEA sentences can be used by             |... yRow4
            | a USB-attached computer.                  |... yRow5
            |                                           |
            | v1.14, Feb 12 2024                        |... yRow9
            +-------------------------------------------+
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
extern Logger logger;   // Griduino.ino
extern Model *model;    // "model" portion of model-view-controller

extern void start_nmea();   // commands.cpp
extern void stop_nmea();    // commands.cpp

// ========== class ViewCfgNMEA ===================================
class ViewCfgNMEA : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewCfgNMEA(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background     = cBACKGROUND;   // every view can have its own background color
    selectedOption = SEND_NMEA;     // default: assume a program like NMEATime2 is listening
  }
  void updateScreen();
  void startScreen();
  void endScreen();
  bool onTouch(Point touch);
  void loadConfig();
  void saveConfig();

  enum functionID {
    SEND_NMEA = 0,
    SILENT,
  };
  functionID selectedOption;   // <-- this is the whole reason for this module's existence

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  // vertical placement of text rows
  const int space = 30;
  const int yRow1 = 80;
  const int yRow2 = yRow1 + 24;
  const int yRow3 = yRow2 + 24;
  const int yRow4 = yRow3 + 50;
  const int yRow5 = yRow4 + 24;
  const int yRow9 = gScreenHeight - 10;   // "v1.14, Feb 15 2024"

  const int yButton1 = 62;
  const int yButton2 = yButton1 + 54;

#define col1    10    // left-adjusted column of text
#define xButton 160   // indented column of buttons
#define bWidth  140   //
#define bHeight 40    //

  // clang-format off
#define nTextNMEA 6
  TextField txtStatic[nTextNMEA] = {
      //  text                x, y      color
      {"GPS NMEA Sentences", -1, 20,    cHIGHLIGHT, ALIGNCENTER},
      {"Report GPS ",      col1, yRow1, cVALUE},
      {"positions",        col1, yRow2, cVALUE},
      {"NMEA sentences are available", -1, yRow4, cFAINT, ALIGNCENTER},
      {"to a USB-attached computer",   -1, yRow5, cFAINT, ALIGNCENTER},
      {PROGRAM_VERDATE,   -1, yRow9, cLABEL, ALIGNCENTER},
  };
  // clang-format on

  enum buttonID {
    eSTART_NMEA = 0,
    eSTOP_NMEA,
  };
  // clang-format off
#define nButtonsNMEA 2
  FunctionButton myButtons[nButtonsNMEA] = {
      // label                  origin           size      touch-target
      // text                     x,y             w,h       x,y            w,h  radius color  functionID
      {"Send NMEA",         xButton, yButton1,  150, 40, {130, yButton1, 184, 50}, 4, cVALUE, eSTART_NMEA},
      {"No report",         xButton, yButton2,  150, 40, {130, yButton2, 184, 60}, 4, cVALUE, eSTOP_NMEA},
  };
  // clang-format on

  // ---------- local functions for this derived class ----------
  void fStartNMEA() {
    selectedOption = SEND_NMEA;
    start_nmea();   // start sending nmea sentences
    this->updateScreen();
  }

  void fStopNMEA() {
    selectedOption = SILENT;
    stop_nmea();   // stop nmea
    this->updateScreen();
  }

};   // end class ViewCfgNMEA

// ============== implement public interface ================
void ViewCfgNMEA::updateScreen() {
  // called on every pass through main()

  // ----- show selected radio buttons by filling in the circle
  for (int ii = eSTART_NMEA; ii <= eSTOP_NMEA; ii++) {
    FunctionButton item = myButtons[ii];
    int xCenter         = item.x - 16;
    int yCenter         = item.y + (item.h / 2);
    int buttonFillColor = cBACKGROUND;

    if (ii == eSTART_NMEA && logger.print_nmea) {
      buttonFillColor = cLABEL;
    }
    if (ii == eSTOP_NMEA && !logger.print_nmea) {
      buttonFillColor = cLABEL;
    }
    tft->fillCircle(xCenter, yCenter, 4, buttonFillColor);
  }
}   // end updateScreen

void ViewCfgNMEA::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);             // clear screen
  txtStatic[0].setBackground(this->background);    // set background for all TextFields in this view
  TextField::setTextDirty(txtStatic, nTextNMEA);   // make sure all fields get re-printed on screen change

  drawAllIcons();                                // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();                     // optionally draw box around default button-touch areas
  showMyTouchTargets(myButtons, nButtonsNMEA);   // optionally show this view's touch targets
  showScreenBorder();                            // optionally outline visible area
  showScreenCenterline();                        // optionally draw visual alignment bar

  // ----- draw text fields
  for (int ii = 0; ii < nTextNMEA; ii++) {
    txtStatic[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii = 0; ii < nButtonsNMEA; ii++) {
    FunctionButton item = myButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y + item.h / 2 + 5);   // place text centered inside button
    tft->setTextColor(item.color);
    tft->print(item.text);
  }

  // ----- draw outlines of radio buttons
  for (int ii = eSTART_NMEA; ii <= eSTOP_NMEA; ii++) {
    FunctionButton item = myButtons[ii];
    int xCenter         = item.x - 16;
    int yCenter         = item.y + (item.h / 2);

    // outline the radio button
    // the active button will be indicated in updateScreen()
    tft->drawCircle(xCenter, yCenter, 7, cVALUE);
  }

  showProgressBar(5, 8);   // draw marker for advancing through settings
  updateScreen();          // update UI immediately, don't wait for laggy mainline loop
}   // end startScreen()

void ViewCfgNMEA::endScreen() {
  // Called once each time this view becomes INactive
  // This is a 'goodbye kiss' to do cleanup work
  // For the current configuration screen; save our settings here instead of on each
  // button press because writing to NVR is slow (0.5 sec) and would delay the user
  // while trying to press a button many times in a row.
  saveConfig();
}

bool ViewCfgNMEA::onTouch(Point touch) {
  bool handled = false;   // assume a touch target was not hit
  for (int ii = 0; ii < nButtonsNMEA; ii++) {
    FunctionButton item = myButtons[ii];
    if (item.hitTarget.contains(touch)) {
      handled = true;               // hit!
      switch (item.functionIndex)   // do the thing
      {
      case eSTART_NMEA:
        fStartNMEA();
        break;
      case eSTOP_NMEA:
        fStopNMEA();
        break;
      default:
        logger.log(CONFIG, ERROR, "unknown function %d", item.functionIndex);
        break;
      }
      updateScreen();   // update UI immediately, don't wait for laggy mainline loop
    }
  }
  return handled;   // true=handled, false=controller uses default action
}   // end onTouch()

// ========== load/save config setting =========================
#define NMEA_CONFIG_FILE    CONFIG_FOLDER "/nmea.cfg"
#define NMEA_CONFIG_VERSION "NMEA Broadcast v01"

// ----- load from SDRAM -----
void ViewCfgNMEA::loadConfig() {
  // Load NMEA on/off setting from NVR, and do minimal amount of work
  // Since this is called first thing during setup, we can't use
  // resource-heavy functions like updateScreen()

  SaveRestore config(NMEA_CONFIG_FILE, NMEA_CONFIG_VERSION);
  int tempOption = 0;
  int result     = config.readConfig((byte *)&tempOption, sizeof(tempOption));
  if (result) {
    switch (tempOption) {
    case SEND_NMEA:
      this->selectedOption = SEND_NMEA;
      start_nmea();   // commands.cpp
      break;
    case SILENT:
      this->selectedOption = SILENT;
      stop_nmea();   // commands.cpp
      break;
    default:
      // should not happen unless config file is corrupt
      char msg[256];
      snprintf(msg, sizeof(msg), "%s has unexpected setting: %d",
               NMEA_CONFIG_FILE, tempOption);
      logger.log(CONFIG, ERROR, msg);
      this->selectedOption = SILENT;
      stop_nmea();
      break;
    }
    logger.log(NMEA, INFO, "Loaded NMEA value: %d", this->selectedOption);
  } else {
    logger.log(NMEA, ERROR, "Failed to load config, re-initializing file");
    this->selectedOption = SILENT;
    saveConfig();
  }
}
// ----- save to SDRAM -----
void ViewCfgNMEA::saveConfig() {
  SaveRestore config(NMEA_CONFIG_FILE, NMEA_CONFIG_VERSION);
  logger.log(NMEA, INFO, "Saving value: %d", selectedOption);
  int rc = config.writeConfig((byte *)&selectedOption, sizeof(selectedOption));
  logger.log(NMEA, DEBUG, "Finished ViewCfgRotation::saveConfig(%d) with rc = %d", selectedOption, rc);   // debug
}
