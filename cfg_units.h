#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     cfg_units.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the 'control panel' for one-time Griduino setup.
            Since it's not intended for a driver in motion, we can use
            a smaller font and cram more stuff onto the screen.

            +-------------------------------------------+
            |  *                Units                >  |
            |                                           |
            | Distance         (o)[ Miles, inHg     ]   |. . .yRow1
            |                  ( )[ Kilometers, hPa ]   |. . .yRow2
            |                                           |
            |                                           |
            |                                           |
            |                                           |
            |       v1.15, compiled Jun 23 2025         |... yRow9
            +-------------------------------------------+
*/

#include "logger.h"      // conditional printing to Serial port
#include "TextField.h"   // Optimize TFT display text for proportional fonts
#include "view.h"        // Base class for all views

// ========== extern ===========================================
extern void showDefaultTouchTargets();   // Griduino.ino

#define nUnitsButtons 2
#define nTextUnits    4

// ========== class ViewCfgUnits ==============================
class ViewCfgUnits : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewCfgUnits(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background = cBACKGROUND;   // every view can have its own background color
    isMetric   = false;         // default to English units (until we read setting from Flash)
  }
  // speed & distance reported in miles(false), kilometers(true)
  bool isMetric = false;   // <-- this is the whole reason for this module's existence

  void begin() {
    this->loadConfig();
  }   // end begin
  void updateScreen() {
    // called on every pass through main()
    // ----- show selected radio buttons by filling in the circle
    for (int ii = eENGLISH; ii <= eMETRIC; ii++) {
      FunctionButton item = myButtons[ii];
      int xCenter         = item.x - 16;
      int yCenter         = item.y + (item.h / 2);
      int buttonFillColor = cBACKGROUND;

      if (ii == eENGLISH && !this->isMetric) {
        buttonFillColor = cLABEL;
      }
      if (ii == eMETRIC && this->isMetric) {
        buttonFillColor = cLABEL;
      }
      tft->fillCircle(xCenter, yCenter, 4, buttonFillColor);
    }
  }   // end updateScreen
  void startScreen() {
    // called once each time this view becomes active
    logger.log(SCREEN, DEBUG, "ViewCfgUnits::startScreen()");
    this->clearScreen(this->background);              // clear screen
    txtStatic[0].setBackground(this->background);     // set background for all TextFields in this view
    TextField::setTextDirty(txtStatic, nTextUnits);   // make sure all fields get re-printed on screen change

    drawAllIcons();                                 // draw gear (settings) and arrow (next screen)
    showDefaultTouchTargets();                      // optionally draw box around default button-touch areas
    showMyTouchTargets(myButtons, nUnitsButtons);   // optionally show this view's touch targets
    showScreenBorder();                             // optionally outline visible area
    showScreenCenterline();                         // optionally draw visual alignment bar

    // ----- draw text fields
    for (int ii = 0; ii < nTextUnits; ii++) {
      txtStatic[ii].print();
    }

    // ----- draw buttons
    setFontSize(eFONTSMALLEST);
    for (int ii = 0; ii < nUnitsButtons; ii++) {
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
    for (int ii = eENGLISH; ii <= eMETRIC; ii++) {
      FunctionButton item = myButtons[ii];
      int xCenter         = item.x - 16;
      int yCenter         = item.y + (item.h / 2);

      // outline the radio button
      // the active button will be indicated in updateScreen()
      tft->drawCircle(xCenter, yCenter, 7, cVALUE);
    }

    showProgressBar(6, 9);   // draw marker for advancing through settings
    updateScreen();          // update UI immediately, don't wait for the main loop to eventually get around to it
  }   // end startScreen()
  void endScreen() {
    // Called once each time this view becomes INactive
    // This is a 'goodbye kiss' to do cleanup work
    // For english/metric units selection, there is nothing to do
    // because configuration was saved when the button was pressed
  }
  bool onTouch(Point touch) override {
    logger.fencepost("cfg_units.h", __LINE__);   // debug
    logger.log(SCREEN, INFO, "->->-> Touched units screen.");
    bool handled = false;   // assume a touch target was not hit
    for (int ii = 0; ii < nUnitsButtons; ii++) {
      FunctionButton item = myButtons[ii];
      if (item.hitTarget.contains(touch)) {
        handled = true;               // hit!
        switch (item.functionIndex)   // do the thing
        {
        case eENGLISH:
          logger.log(CONFIG, DEBUG, "eENGLISH");
          isMetric = false;
          saveConfig();   // config file is so small and quick we can save it on button press
          break;
        case eMETRIC:
          logger.log(CONFIG, DEBUG, "eMETRIC");
          isMetric = true;
          saveConfig();   // config file is so small and quick we can save it on button press
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

protected:
  // ----- load/save from SDRAM -----
#define UNITS_CONFIG_FILE    CONFIG_FOLDER "/units.cfg"
#define CONFIG_UNITS_VERSION "English/Metric v01"
  void loadConfig() {
    logger.fencepost("ViewCfgUnits::loadConfig", __LINE__);
    SaveRestore config(UNITS_CONFIG_FILE, CONFIG_UNITS_VERSION);
    bool tempUnits = false;
    int result     = config.readConfig((byte *)&tempUnits, sizeof(tempUnits));
    if (result) {
      isMetric = tempUnits;   // global english/metric setting
      logger.log(CONFIG, INFO, "Loaded english/metric setting from NVR: ", isMetric);
    } else {
      logger.log(CONFIG, ERROR, "Failed to load english/metric setting, re-initializing file");
      saveConfig();
    }
  }
  void saveConfig() {
    SaveRestore config(UNITS_CONFIG_FILE, CONFIG_UNITS_VERSION);
    config.writeConfig((byte *)&isMetric, sizeof(isMetric));
  }
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  // vertical placement of text rows   ---label---         ---button---
  const int yRow1 = 92;                   // "English",          "Miles, inHg"
  const int yRow2 = yRow1 + 50;           // "Metric",           "Kilometers, hPa"
  const int yRow9 = gScreenHeight - 10;   // "v1.14, Jan 22 2024"

  const int col1 = 10;   // left-adjusted column of text
#define xButton 160      // indented column of buttons

  // clang-format off
  TextField txtStatic[nTextUnits] = {
      //  text             x, y      color
      {"Units",           -1, 20,    cHIGHLIGHT, ALIGNCENTER},
      {"English",       col1, yRow1, cVALUE},
      {"Metric",        col1, yRow2, cVALUE},
      {PROGRAM_VERDATE,   -1, yRow9, cLABEL, ALIGNCENTER},
  };
  // clang-format on

  enum buttonID {
    eENGLISH = 0,
    eMETRIC,
  };
  // clang-format off
  FunctionButton myButtons[nUnitsButtons] = {
      // label                  origin           size      touch-target
      // text                     x,y             w,h       x,y            w,h  radius color  functionID
      {"Miles, Feet, inHg", xButton, yRow1-26,  150, 40, {112, yRow1-36, 204, 56}, 4, cVALUE, eENGLISH},   // [eENGLISH] set units Miles/inHg
      {"Kilometers, hPa",   xButton, yRow2-26,  150, 40, {112, yRow2-31, 204, 64}, 4, cVALUE, eMETRIC},    // [eMETRIC] set units Metric
  };
  // clang-format on

};   // end class ViewCfgUnits
