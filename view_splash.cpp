 /*
  File: view_splash.cpp

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

#include <Arduino.h>
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors
#include "TextField.h"              // Optimize TFT display text for proportional fonts
#include "view.h"                   // Base class for all views

// ========== extern ===========================================
void setFontSize(int font);             // Griduino.ino
void showScreenBorder();                // optionally outline visible area

// ============== constants ====================================
// color scheme: see constants.h

// ========== splash screen view ===============================
void ViewSplash::updateScreen() {
  // called on every pass through main()
  // nothing to do in the main loop - this screen has no dynamic items
}

void ViewSplash::startScreen() {
  // called once each time this view becomes active

  TextField txtSplash[] = {
    //        text               x,y    color  
    TextField(PROGRAM_TITLE,    64, 84, cHIGHLIGHT),  // giant program title, centered
    TextField(PROGRAM_VERSION, 132,110, cVALUE),      // normal size text, centered
    TextField(PROGRAM_LINE1,    89,164, cVALUE),
    TextField(PROGRAM_LINE2,    98,196, cVALUE),
  };
  const int numSplashFields = sizeof(txtSplash)/sizeof(txtSplash[0]);

  this->clearScreen(cBACKGROUND);        // clear screen
  txtSplash[0].setBackground(cBACKGROUND);                // set background for all TextFields in this view
  TextField::setTextDirty( txtSplash, numSplashFields );  // make sure all fields get re-printed on screen change

  setFontSize(24);
  txtSplash[0].print();               // large program title

  setFontSize(12);
  txtSplash[1].print();               // program version
  txtSplash[2].print();               // credits 1
  txtSplash[3].print();               // credits 2

  showScreenBorder();                 // optionally outline visible area

  updateScreen();                     // fill in values immediately, don't wait for the main loop to eventually get around to it

#ifdef USE_MORSE_CODE
  // ----- announce in Morse code, so vehicle's driver doesn't have to look at the screen
  spkrMorse.setMessage(String(" de k7bwh  ")); // lowercase is required
  //spkrMorse.startSending();         // non-blocking (TODO: does not send evenly)
  spkrMorse.sendBlocking();           // blocking
#endif
}

bool ViewSplash::onTouch(Point touch) {
  Serial.println("->->-> Touched splash screen.");
  return false;                       // true=handled, false=controller uses default action
}
