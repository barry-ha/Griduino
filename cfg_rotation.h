#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     cfg_rotation.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the 'control panel' for screen rotation.
            Since it's not intended for a driver in motion, we can use
            a smaller font and cram more stuff onto the screen.

            +-------------------------------------------+
            |  *              Rotation        /|\     > |
            | Screen                           |        |
            | Orientation         (o)[ This edge up ]   |
            |                                           |
            |                     ( )[ That edge up ]   |
            |                                  |        |
            |                                  |        |
            |                                  |        |
            | v1.12, Feb 10 2021              \|/       |
            +-------------------------------------------+
*/

#include "constants.h"   // Griduino constants and colors
#include "logger.h"      // conditional printing to Serial port
#include "TextField.h"   // Optimize TFT display text for proportional fonts
#include "view.h"        // Base class for all views

// ========== extern ===========================================
extern void showDefaultTouchTargets();   // Griduino.ino

// ========== class ViewCfgRotation ==============================
class ViewCfgRotation : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewCfgRotation(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background = cBACKGROUND;   // every view can have its own background color
  }
  void updateScreen();
  void startScreen();
  bool onTouch(Point touch) override;
  void loadConfig();
  void saveConfig();

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  // vertical placement of text rows   ---label---           ---button---
  const int yRow1 = 86;                   // "Screen Orientation", "This edge up"
  const int yRow2 = yRow1 + 70;           //                       "That edge up"
  const int yRow9 = gScreenHeight - 10;   // "v1.14, Feb 12 2024"

#define col1    10    // left-adjusted column of text
#define xButton 160   // indented column of buttons
#define bWidth  140   //
#define bHeight 40    //

  // clang-format off
#define nFields 4
  TextField txtSettings6[nFields] = {
      //  text             x, y      color
      {"Rotation",        -1, 20,    cHIGHLIGHT, ALIGNCENTER},
      {"Screen",        col1, yRow1, cVALUE},
      {"Orientation",   col1, yRow1 + 20, cVALUE},
      {PROGRAM_VERSION,   -1, yRow9, cLABEL, ALIGNCENTER},
  };
  // clang-format on

  enum functionID {
    THIS_BUTTON = 0,
    THAT_BUTTON,
  };
  // clang-format off
#define nButtons 2
  FunctionButton myButtons[nButtons] = {
      // label             origin           size                  touch-target
      // text                x,y             w,h                     x,y            w,h  radius  color   functionID
      {"This edge up", xButton, yRow1-26,  bWidth, bHeight, {xButton-2, yRow1-28,  bWidth+4, bHeight+4}, 4, cVALUE, THIS_BUTTON},
      {"That edge up", xButton, yRow2-26,  bWidth, bHeight, {xButton-2, yRow2-28,  bWidth+4, bHeight+4}, 4, cVALUE, THAT_BUTTON},
  };
  // clang-format on

#define xLine (xButton + 100)
#define topY  8
#define botY  (gScreenHeight - 6)

#define nLines 6
  TwoPoints myLines[nLines] = {
      //   x1,y1           x2,y2
      {xLine, yRow1 - 26, xLine, topY, cBUTTONOUTLINE},   // from top outline of top button, to top edge of screen
      {xLine, 5, xLine - 4, topY + 15, cBUTTONOUTLINE},   // left half of arrowhead
      {xLine, 5, xLine + 4, topY + 15, cBUTTONOUTLINE},   // right half of arrowhead

      {xLine, yRow2 + 14, xLine, botY, cBUTTONOUTLINE},      // from bottom outline of bottom button, to bottom edge of screen
      {xLine, botY, xLine - 4, botY - 16, cBUTTONOUTLINE},   // left half of arrowhead
      {xLine, botY, xLine + 4, botY - 16, cBUTTONOUTLINE},   // right half of arrowhead
  };

  // ---------- local functions for this derived class ----------
  void fRotateScreen() {
    logger.log(CONFIG, INFO, "->->-> Clicked OTHER EDGE UP button.");
    if (this->screenRotation == LANDSCAPE) {
      this->setScreenRotation(FLIPPED_LANDSCAPE);
    } else {
      this->setScreenRotation(LANDSCAPE);
    }
  }

};   // end class ViewCfgRotation

// ============== implement public interface ================
void ViewCfgRotation::updateScreen() {
  // called on every pass through main()

  // ----- show selected radio buttons by filling in the circle
  for (int ii = 0; ii < nButtons; ii++) {
    FunctionButton item = myButtons[ii];
    int xCenter         = item.x - 16;
    int yCenter         = item.y + (item.h / 2);
    int buttonFillColor = cBACKGROUND;

    if (item.functionIndex == THIS_BUTTON) {
      // it may seem strange, but we ALWAYS highlight the first button as active
      // because the first button is always the "up" edge
      buttonFillColor = cLABEL;
    }
    tft->fillCircle(xCenter, yCenter, 4, buttonFillColor);
  }
}   // end updateScreen

void ViewCfgRotation::startScreen() {
  // called once each time this view becomes active
  logger.log(SCREEN, DEBUG, "ViewCfgRotation::startScreen()");
  this->clearScreen(this->background);               // clear screen
  txtSettings6[0].setBackground(this->background);   // set background for all TextFields in this view
  TextField::setTextDirty(txtSettings6, nFields);    // make sure all fields get re-printed on screen change

  drawAllIcons();                            // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();                 // optionally draw box around default button-touch areas
  showMyTouchTargets(myButtons, nButtons);   // optionally show this view's touch targets
  showScreenBorder();                        // optionally outline visible area
  showScreenCenterline();                    // optionally draw visual alignment bar

  // ----- draw text fields
  for (int ii = 0; ii < nFields; ii++) {
    txtSettings6[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii = 0; ii < nButtons; ii++) {
    FunctionButton item = myButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y + item.h / 2 + 5);   // place text centered inside button
    tft->setTextColor(item.color);
    tft->print(item.text);
  }

  // ----- draw arrows from buttons to top/bottom edges
  for (int ii = 0; ii < nLines; ii++) {
    TwoPoints m = myLines[ii];
    tft->drawLine(m.x1, m.y1, m.x2, m.y2, m.color);
  }

  // ----- draw outlines of radio buttons
  for (int ii = 0; ii < nButtons; ii++) {
    FunctionButton item = myButtons[ii];
    int xCenter         = item.x - 16;
    int yCenter         = item.y + (item.h / 2);

    // outline the radio button
    // the active button will be indicated in updateScreen()
    tft->drawCircle(xCenter, yCenter, 7, cVALUE);
  }

  showProgressBar(7, 9);   // draw marker for advancing through settings
  updateScreen();          // update UI immediately, don't wait for laggy mainline loop
}   // end startScreen()

bool ViewCfgRotation::onTouch(Point touch) {
  logger.log(CONFIG, INFO, "->->-> Touched settings screen.");
  bool handled = false;   // assume a touch target was not hit
  for (int ii = 0; ii < nButtons; ii++) {
    FunctionButton item = myButtons[ii];
    if (item.hitTarget.contains(touch)) {
      handled = true;               // hit!
      switch (item.functionIndex)   // do the thing
      {
      case THIS_BUTTON:
        // nothing - this button is already at top of screen
        break;
      case THAT_BUTTON:
        // end-user wants the OTHER edge of the screen (whatever it is) to be "up"
        fRotateScreen();
        break;
      default:
        logger.log(CONFIG, ERROR, "unknown function %d", item.functionIndex);
        break;
      }
      updateScreen();       // update UI immediately, don't wait for laggy mainline loop
      this->saveConfig();   // after UI is updated, save setting to nvr
    }
  }
  return handled;   // true=handled, false=controller uses default action
}   // end onTouch()

// ========== load/save config setting =========================
#define SCREEN_CONFIG_FILE    CONFIG_FOLDER "/screen.cfg"
#define CONFIG_SCREEN_VERSION "Screen Orientation v04"

// ----- load from SDRAM -----
void ViewCfgRotation::loadConfig() {
  // Load screen orientation from NVR, and do minimal amount of work
  // Since this is called first thing during setup, we can't use
  // resource-heavy functions like "this->setScreenRotation(int rot)"

  SaveRestore config(SCREEN_CONFIG_FILE, CONFIG_SCREEN_VERSION);
  int tempRotation;
  int result = config.readConfig((byte *)&tempRotation, sizeof(tempRotation));
  if (result) {
    this->screenRotation = tempRotation;
    switch (tempRotation) {
    case LANDSCAPE:
      // normal result
      this->screenRotation = tempRotation;
      break;
    case FLIPPED_LANDSCAPE:
      // normal result
      this->screenRotation = tempRotation;
      break;
    default:
      // should not happen unless config file is corrupt
      char msg[256];
      snprintf(msg, sizeof(msg), "%s has unexpected screen orientation: %d",
               SCREEN_CONFIG_FILE, tempRotation);
      logger.log(CONFIG, ERROR, msg);
      this->screenRotation = LANDSCAPE;
      break;
    }
    tft->setRotation(this->screenRotation);   // 0=portrait (default), 1=landscape, 3=180 degrees
    logger.log(CONFIG, INFO, "Loaded rotation value: ", this->screenRotation);
  } else {
    logger.log(CONFIG, ERROR, "Failed to load screen rotation, re-initializing file");
    this->screenRotation = LANDSCAPE;
    saveConfig();
  }
}
// ----- save to SDRAM -----
void ViewCfgRotation::saveConfig() {
  SaveRestore config(SCREEN_CONFIG_FILE, CONFIG_SCREEN_VERSION);
  int rc = config.writeConfig((byte *)&screenRotation, sizeof(screenRotation));
  logger.log(CONFIG, DEBUG, "Finished ViewCfgRotation::saveConfig() with rc = %d", rc);   // debug
}
