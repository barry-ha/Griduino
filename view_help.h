#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     view_help.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is a brief screen displayed at startup to
            remind the user about the controls on the main
            screen. They are not outlined or shown and you
            will need to simply remember where they are.

            +-Hint:-------+-----------------------------+
            |             |                             |
            |  Settings   |     Tap for next view       |
            |             |                             |
            |-------------+-----------------------------|
            |                                           |
            |      Tap to change brightness             |
            |                                           |
            +-------------------------------------------+
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>   // TFT color display library
#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
extern Logger logger;                    // Griduino.ino
extern void showDefaultTouchTargets();   // Griduino.ino

// ========== class ViewHelp ===================================
class ViewHelp : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewHelp(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {}
  void updateScreen();
  void startScreen();
  bool onTouch(Point touch);

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  const int margin = 10;   // slight margin between button border and edge of screen
  const int radius = 10;   // rounded corners on buttons

  enum txtIndex {
    SETTINGS = 0,
    NEXTVIEW,
    BRIGHTNESS,
    VIEWNAME,
  };

// const int cl = gScreenWidth/2;
#define nHelpButtons 3
  Button helpButtons[nHelpButtons] = {
      //       text                x,y             w,h      r      color
      {"Settings", margin, margin, 98, 105, radius, cBUTTONLABEL},                   //[SETTINGS]
      {"Tap for next view", margin + 108, margin, 192, 105, radius, cBUTTONLABEL},   //[NEXTVIEW]
      {"Tap for brightness", margin, 126, 300, 105, radius, cBUTTONLABEL},           //[BRIGHTNESS]
                                                                                     //{"Hint",             cl-38,4,            76,26, radius/2, cHIGHLIGHT }, //[VIEWNAME]
  };

};   // end class ViewHelp

// ============== implement public interface ================
void ViewHelp::updateScreen() {
  // called on every pass through main()
  // nothing to do in the main loop - this screen has no dynamic items
}

void ViewHelp::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                     // clear screen

  showDefaultTouchTargets();                               // optionally draw box around default button-touch areas
  // showMyTouchTargets(Buttons, nButtons);   // no real buttons on this view
  showScreenBorder();                                      // optionally outline visible area
  showScreenCenterline();                                  // optionally draw visual alignment bar
  // ----- draw buttons
  setFontSize(eFONTSMALL);
  for (int ii = 0; ii < nHelpButtons; ii++) {
    Button item = helpButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y + item.h / 2 + 7);
    tft->setTextColor(item.color);
    tft->print(item.text);
  }

  updateScreen();   // update UI immediately, don't wait for the main loop to eventually get around to it

  // ----- label this view in upper left corner
  showNameOfView("Hint: ", cWARN, cBACKGROUND);
}   // end startScreen()

bool ViewHelp::onTouch(Point touch) {
  // do nothing - this screen does not respond to buttons
  logger.info("->->-> Touched help screen.");
  return false;   // true=handled, false=controller uses default action

}   // end onTouch()
