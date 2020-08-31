/*
  File: view.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Abstract base class for all Griduino "view" modules.
            Contains default implementation for common functions
            and for example programming.
*/
#pragma once

#include <Arduino.h>
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors

class View {
  public:
    // public member variables go here

  public:
    /**
     * Constructor
     */
    View() {
    }

    /**
     * Called on every pass through main()
     */
    void updateScreen() {
    }

    /**
     * Called once each time this view becomes active
     */
    void startScreen() {
      clearScreen(cBACKGROUND);         // clear screen

      // setFontSize(24);               // example
      showScreenBorder();               // optionally outline visible area
      updateScreen();                   // fill in screen immediately, don't wait for the main loop to eventually get around to it
    }

    /**
     * Called whenever the touchscreen has an event for this view
     */
    bool onTouch(Point touch) {
      Serial.println("->->-> Touched screen.");
      return false;                     // true=handled, false=controller uses default action
    }

  protected:
};  // end class View
