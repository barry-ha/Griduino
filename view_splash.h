#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     view_splash.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  When you power up the Griduino, you are greeted with this view.
            We announce ourselves and the version number.

            +-----------------------------------+
            |                                   |
            |      Griduino                     |
            |      version 1.0                  |
            |                                   |
            |      Barry K7BWH                  |
            |      John KM7O                    |
            |                                   |
            +-----------------------------------+
*/

#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
extern void showDefaultTouchTargets();   // Griduino.ino

// ========== class ViewSplash =================================
class ViewSplash : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewSplash(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {}
  void updateScreen();
  void startScreen();
  bool onTouch(Point touch) override;

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

};   // end class ViewSplash

// ============== implement public interface ================
void ViewSplash::updateScreen() {
  // called on every pass through main()
  // nothing to do in the main loop - this screen has no dynamic items
}

void ViewSplash::startScreen() {
  // called once each time this view becomes active
  logger.log(SCREEN, DEBUG, "ViewSplash::startScreen()");

  TextField txtSplash[] = {
      //        text     x,y    color       alignment    font size
      {PROGRAM_TITLE, -1, 84, cHIGHLIGHT, ALIGNCENTER, eFONTBIG},    // giant program title, centered
      {PROGRAM_VERSION, -1, 110, cVALUE, ALIGNCENTER, eFONTSMALL},   // normal size text, centered
      {PROGRAM_LINE1, -1, 164, cVALUE, ALIGNCENTER, eFONTSMALL},
      {PROGRAM_LINE2, -1, 196, cVALUE, ALIGNCENTER, eFONTSMALL},
  };
  const int numSplashFields = sizeof(txtSplash) / sizeof(txtSplash[0]);

  this->clearScreen(this->background);                   // clear screen
  txtSplash[0].setBackground(cBACKGROUND);               // set background for all TextFields in this view
  TextField::setTextDirty(txtSplash, numSplashFields);   // make sure all fields get re-printed on screen change

  showDefaultTouchTargets();   // optionally draw box around default button-touch areas
  showMyTouchTargets(0, 0);    // no real buttons on this view
  showScreenBorder();          // optionally outline visible area
  showScreenCenterline();      // optionally draw visual alignment bar

  // ----- draw text fields
  for (int ii = 0; ii < numSplashFields; ii++) {
    txtSplash[ii].print();
  }

  updateScreen();   // update UI immediately, don't wait for the main loop to eventually get around to it

#ifdef USE_MORSE_CODE
  // ----- announce in Morse code, so vehicle's driver doesn't have to look at the screen
  spkrMorse.setMessage(String(" de k7bwh  "));   // lowercase is required
  // spkrMorse.startSending();       // non-blocking (TODO: does not send evenly)
  spkrMorse.sendBlocking();   // blocking
#endif
}

bool ViewSplash::onTouch(Point touch) {
  // do nothing - this screen does not respond to buttons
  logger.log(CONFIG, INFO, "->->-> Touched splash screen.");
  return false;   // true=handled, false=controller uses default action

}   // end onTouch()
