#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     view.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Abstract base class for all Griduino "view" modules.
            Contains default implementation for common functions
            and for templates to be used in derived classes.
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>   // TFT color display library
#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "icons.h"              // bitmaps for icons
#include "model_adc.h"          // Model of the analog-digital converter

extern bool showTouchTargets;       // Griduino.ino
extern bool showCenterline;         // Griduino.ino
extern BatteryVoltage gpsBattery;   // model_adc.h

// ========== abstract base class ViewCfgAudioType ================
class View {
public:
  // public member variables go here
  Adafruit_ILI9341 *tft;   // an instance of the TFT Display
  const int screenID;      // unique identifier of which screen this is
  int screenRotation;      // 1=landscape, 3=landscape 180-degrees
  uint16_t background;     // screen background color

public:
  /**
   * Constructor
   */
  // View() { }                      // no default ctor (must be told 'tft')
  View(Adafruit_ILI9341 *vtft, int vid)
      : tft(vtft), screenID(vid), background(0x000)   // default black background
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
   * Called once each time this view becomes INactive
   * This is a 'goodbye kiss' for a view handler to do cleanup work, such as saving its settings
   */
  virtual void endScreen() {
  }

  /**
   * Called whenever the touchscreen has an event for this view
   */
  virtual bool onTouch(Point touch) {
    logger.info("->->-> Touched screen.");
    return false;   // true=handled, false=controller uses default action
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
  const int xPanel = 34;   // screen placement of "1 of 6" config panel legend

  /**
   *  The One and Only True Clear Screen (TOOTCS) function
   */
  void clearScreen(uint16_t color = cBACKGROUND) {   // clear entire screen
    tft->fillScreen(color);
  }

  void clearScreen(int startRow, int numRows, uint16_t color = cBACKGROUND) {   // clear selected rows
    tft->fillRect(0, startRow, tft->width(), numRows, color);
  }

  void drawAllIcons() {
    // draw gear (settings) and arrow (next screen)
    //              ul x,y                w,h   color
    tft->drawBitmap(05, 05, iconGear20, 20, 20, cFAINT);         // "settings" upper left
    tft->drawBitmap(300, 5, iconRightArrow18, 14, 18, cFAINT);   // "next screen" upper right

    float volts = gpsBattery.readCoinBatteryVoltage();
    if (volts < GOOD_BATTERY_MINIMUM) {
      int iconColor = gpsBattery.getBatteryColor(volts);
      tft->drawBitmap(36, 06, iconLowBattery, 14, 20, iconColor);   // "low battery" warning
    }
  }

  void showScreenBorder() {   // optionally outline visible area
#ifdef SHOW_SCREEN_BORDER
    tft->drawRect(0, 0, gScreenWidth, gScreenHeight, ILI9341_BLUE);   // debug: border around screen
#endif
  }

  void showScreenCenterline() {
    if (showCenterline) {
      // show centerline at       x1,y1                x2,y2              color
      tft->drawLine(tft->width() / 2, 0, tft->width() / 2, tft->height(), cWARN);
    }
  }

  void showNameOfView(String sName, uint16_t fgd, uint16_t bkg) {
    // Some of the "view" modules want to label themselves in the upper left corner
    // Caution: this function changes font. The caller needs to change it back, if needed.
    tft->setFont();
    tft->setTextSize(2);   // setFontSize(0);
    tft->setTextColor(fgd, bkg);
    tft->setCursor(1, 1);
    tft->print(sName);
  }

  void showProgressBar(int screen, int numScreens) {
    // draw marker for advancing through settings, eg, 1 of 6, 2 of 6, etc.
    // we try to make it look like a filled rectangle
    int x0 = 74;              // lhs bounding box
    int y0 = 28;              // top bounding box
    int w  = 320 - x0 - x0;   // width of bounding box
    int h  = 10;              // height bounding box
    int c  = cBUTTONOUTLINE;

    int wi = w / numScreens;   // width of each item
    for (int ii = 0; ii < numScreens; ii++) {
      //               x,          y,  w, h, color
      int xOuter = x0 + ii * wi;
      tft->drawRect(xOuter, y0, wi, h, cFAINTER);

      if (ii == (screen - 1)) {
        tft->fillRect(xOuter + 1, y0, wi - 2, h, cLABEL);
      }
    }
  }

  /**
   * Rotate screen right-side-up / upside-down
   * 1=landscape, 3=landscape 180-degrees
   * This is a "protected" method in base class to ensure *only* the Settings page will set rotation.
   */
  void setScreenRotation(int rot) {
    this->screenRotation = rot;
    tft->setRotation(rot);   // 0=portrait (default), 1=landscape, 3=180 degrees
    clearScreen();           // todo - necessary?
    startScreen();           // todo - necessary?
    updateScreen();          // todo - necessary?
  }

  /**
   * Some views include function-specific buttons
   */
  void showMyTouchTargets(FunctionButton buttons[], int numButtons) {
    if (showTouchTargets) {   // if feature is turned off, return
      if (buttons) {          // if a view has no buttons, return
        for (int ii = 0; ii < numButtons; ii++) {
          FunctionButton item = buttons[ii];
          tft->drawRect(item.hitTarget.ul.x, item.hitTarget.ul.y,   // debug: draw outline around hit target
                        item.hitTarget.size.x, item.hitTarget.size.y,
                        cTOUCHTARGET);
        }
      }
    }
  }

};   // end class View

// ----------------------------------------------------------
// Derived classes
class ViewGrid : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewGrid(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background = 0;   // every view can have its own background color; this screen is black
  }
  void updateScreen();
  void startScreen();
  bool onTouch(Point touch);
};   // end class ViewGrid
