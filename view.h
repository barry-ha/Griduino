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
    Adafruit_ILI9341* tft;           // an instance of the TFT Display
    const int screenID;              // unique identifier of which screen this is

  public:
    /**
     * Constructor
     */
    //View() { }                      // no default ctor (must be told 'tft')
    View(Adafruit_ILI9341* vtft, int vid) 
      : tft( vtft ), screenID( vid )
    {
    }

    /**
     * Called on every pass through main()
     */
    virtual void updateScreen() {
    }

    /**
     * Called once each time this view becomes active
     */
    virtual void startScreen() {
      // todo - make this pure virtual
    }

    /**
     * Called whenever the touchscreen has an event for this view
     */
    virtual bool onTouch(Point touch) {
      Serial.println("->->-> Touched screen.");
      return false;                     // true=handled, false=controller uses default action
    }

    /**
     * Call to load/save configuration from non-volatile RAM
     */
    virtual void loadConfig() {
      // default: loads nothing
    }
    virtual void saveConfig() {
      // default: saves nothing
    }

  protected:
    void clearScreen(uint16_t color=cBACKGROUND) { // clear the screen
      tft->fillScreen(color);
    }

    void showScreenBorder() {           // optionally outline visible area
      #ifdef SHOW_SCREEN_BORDER
        tft->drawRect(0, 0, gScreenWidth, gScreenHeight, ILI9341_BLUE);  // debug: border around screen
      #endif
    };
};  // end class View

// ----------------------------------------------------------
// In alphabetical order:
//      Grid, Help, Settings2, Settings3, Splash, Status, Time, Volume
class ViewGrid : public View {
  public:
    ViewGrid(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};

class ViewHelp : public View {
  public:
    ViewHelp(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};

class ViewSettings2 : public View {
  public:
    ViewSettings2(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};

class ViewSettings3 : public View {
  public:
    ViewSettings3(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};

class ViewSplash : public View {
  public:
    ViewSplash(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};

class ViewStatus : public View {
  public:
    ViewStatus(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};

class ViewTime : public View {
  public:
    ViewTime(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
};

class ViewVolume : public View {
  public:
    ViewVolume(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
    void loadConfig();
    void saveConfig();

};
