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

#include <Arduino.h>
#include <Adafruit_ILI9341.h>         // TFT color display library
#include "constants.h"                // Griduino constants and colors
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "view.h"                     // Base class for all views

// ========== class ViewSplash =================================
class ViewSplash : public View {
  public:
    // ---------- public interface ----------
    // This derived class must implement the public interface:
    ViewSplash(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);

  protected:
    // ---------- local data for this derived class ----------
    // color scheme: see constants.h

};  // end class ViewSplash

// ============== implement public interface ================
void ViewSplash::updateScreen() {
  // called on every pass through main()
  // nothing to do in the main loop - this screen has no dynamic items
}


void ViewSplash::startScreen() {
  // called once each time this view becomes active

  TextField txtSplash[] = {
    //        text     x,y    color       alignment    font size
    {PROGRAM_TITLE,   -1, 84, cHIGHLIGHT, ALIGNCENTER, eFONTBIG  },   // giant program title, centered
    {PROGRAM_VERSION, -1,110, cVALUE,     ALIGNCENTER, eFONTSMALL},   // normal size text, centered
    {PROGRAM_LINE1,   -1,164, cVALUE,     ALIGNCENTER, eFONTSMALL},
    {PROGRAM_LINE2,   -1,196, cVALUE,     ALIGNCENTER, eFONTSMALL},
  };
  const int numSplashFields = sizeof(txtSplash)/sizeof(txtSplash[0]);

  this->clearScreen(cBACKGROUND);     // clear screen
  txtSplash[0].setBackground(cBACKGROUND);                // set background for all TextFields in this view
  TextField::setTextDirty( txtSplash, numSplashFields );  // make sure all fields get re-printed on screen change

  // ----- draw text fields
  for (int ii=0; ii<numSplashFields; ii++) {
      txtSplash[ii].print();
  }

  showScreenBorder();                 // optionally outline visible area

  updateScreen();                     // fill in values immediately, don't wait for the main loop to eventually get around to it

  #ifdef SHOW_SCREEN_CENTERLINE
    // show centerline at      x1,y1              x2,y2             color
    tft->drawLine( tft->width()/2,0,  tft->width()/2,tft->height(), cWARN); // debug
  #endif

  #ifdef USE_MORSE_CODE
    // ----- announce in Morse code, so vehicle's driver doesn't have to look at the screen
    spkrMorse.setMessage(String(" de k7bwh  ")); // lowercase is required
    //spkrMorse.startSending();       // non-blocking (TODO: does not send evenly)
    spkrMorse.sendBlocking();         // blocking
  #endif
}

bool ViewSplash::onTouch(Point touch) {
  Serial.println("->->-> Touched splash screen.");
  return false;                       // true=handled, false=controller uses default action
}
