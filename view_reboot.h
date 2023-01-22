#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     view_reboot.h   (based on view_status.h)

  Version history:
            2022-12-12 written for rp2040

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Ask operator to confirm reboot.

            +-----------------------------------------+
            |  *           Griduino Reboot         >  |... yRow1
            |                                         |
            |  Use this option to update firmware.    |... yRow2
            |  Do you have a UF2 file?                |... yRow3
            |  Do you want to reboot?                 |... yRow4
            |  +=============+       +-------------+  |
            |  |             |       |             |  |
            |  |   Cancel    |       | Install UF2 |  |
            |  |             |       |             |  |
            |  +=============+       +-------------+  |
            |                                         |
            |           v1.11, Dec 13 2022            |... yRow9
            +-----------------------------------------+
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  extern "C" {
    #include <hardware/watchdog.h>
    #include <hardware/resets.h>
    #include <pico/bootrom.h>
  }
#endif
extern Logger logger;                    // Griduino.ino
extern void showDefaultTouchTargets();   // Griduino.ino
void selectNewView(int cmd);             // extern declaration

// ========== class ViewReboot ===================================
class ViewReboot : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewReboot(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background = cBACKGROUND;   // every view can have its own background color
  }
  void updateScreen();
  void startScreen();
  bool onTouch(Point touch);

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  const int radius = 10;   // rounded corners on buttons

  // vertical placement of text rows
  const int space = 30;
  const int half  = space / 2;

  const int yRow1 = 20;                  // title "Confirm reboot"
  const int yRow2 = yRow1 + space;       // "Use this option to update firmware."
  const int yRow3 = yRow2 + space;       // "Do you have a UF2 file?"
  const int yRow4 = yRow3 + space;       // "Do you want to install it?""
  const int yRow5 = yRow4 + half;        // "Cancel", "Install UF2" buttons
  const int yRow9 = gScreenHeight - 4;   // "v1.11, Dec 13 2022"

  // ----- screen text
  // names for the array indexes, must be named in same order as array below
  enum txtIndex {
    TITLE = 0,
    LINE1,
    LINE2,
    LINE3,
    COMPILED,
  };

  const int left = 30;   // x: left text edge
  const int top  = 40;   // y: top text row

// ----- static screen text
// clang-format off
#define nRebootValues 5
  TextField txtValues[nRebootValues] = {
    //       text                     x,y       color,  alignment,   font size
    {"Confirm Reboot",               -1, yRow1, cHIGHLIGHT, ALIGNCENTER, eFONTSMALLEST},    // [TITLE] centered
    {"Use this option to update firmware.", -1, yRow2, cLABEL, ALIGNCENTER, eFONTSMALLEST}, // [LINE1] centered
    {"Do you have a UF2 file?",    left, yRow3, cVALUE,     ALIGNLEFT,   eFONTSMALL},       // [LINE2]
    {"Do you want to install it?", left, yRow4, cVALUE,     ALIGNLEFT,   eFONTSMALL},       // [LINE3]
    {PROGRAM_VERSION ", " __DATE__,  -1, yRow9, cLABEL,     ALIGNCENTER, eFONTSMALLEST},    // [COMPILED]
  };

enum buttonID {
  eCANCEL,
  eREBOOT,
};
#define nRebootButtons 2
  FunctionButton myButtons[nRebootButtons] = {
    // label           origin      size   {   touch-target     }
    // text             x,y         w,h   {   x,y        w,h   }, radius, color,      functionID
      {"Cancel",       32,yRow5,  110,90, {  0,yRow5,   140,110 }, radius, cBUTTONLABEL, eCANCEL},
      {"Install UF2", 170,yRow5,  110,90, {180,yRow5,   140,110 }, radius, cBUTTONLABEL, eREBOOT},
  };
  // clang-format on

  // ---------- local functions for this derived class ----------
  void rebootGriduino() {   // operator confirmed: reboot to USB for software update
    // Do The Thing!
    Serial.println("---> REBOOTING");   // the end of our world is nigh
    delay(100);                         // allow time for Serial to send message

    this->clearScreen(this->background);   // clear screen
    // post message
    const int left = 28;   // x: left text edge
    const int top  = 40;   // y: top text row
    setFontSize(eFONTSMALLEST);
    tft->setTextColor(cHIGHLIGHT);
    tft->setCursor(100, yRow1);
    tft->println("Install Firmware");

    setFontSize(eFONTSMALL);
    tft->setTextColor(cVALUE);
    tft->setCursor(left, yRow1 + space * 2);
    tft->println("Check your external drives");
    tft->setCursor(left, yRow1 + space * 3);
    tft->println("and look for RPI-RP2.");
    tft->setCursor(left, yRow1 + space * 4);
    tft->println("Copy UF2 file to that drive.");

    setFontSize(eFONTSMALLEST);
    tft->setTextColor(cLABEL);
    tft->setCursor(left, yRow1 + space * 6);
    tft->println("To decline, press Reset button");
    tft->setCursor(left, yRow1 + space * 6 + half);
    tft->println("or power cycle Griduino");
    delay(1000);

    // reboot
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
    multicore_reset_core1();
    multicore_launch_core1(0);
    reset_usb_boot(0, 0);
#else
    // todo: try to discover a way to put Feather M4 into bootloader mode
    // todo: for now
#endif
  }
  void fCancel() {
    logger.info("->->-> Clicked CANCEL button.");
    extern /*const*/ int grid_view;   // see "Griduino.ino"
    selectNewView(grid_view);
  }
  void fReboot() {
    logger.info("->->-> Clicked REBOOT button.");
    rebootGriduino();
  }

};   // end class ViewReboot

// ============== implement public interface ================
void ViewReboot::updateScreen() {
  // called on every pass through main()
  // nothing to do in the main loop - this screen has no dynamic items
}   // end updateScreen

void ViewReboot::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                 // clear screen
  txtValues[0].setBackground(this->background);        // set background for all TextFields in this view
  TextField::setTextDirty(txtValues, nRebootValues);   // make sure all fields get re-printed on screen change

  drawAllIcons();                                  // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();                       // optionally draw box around default button-touch areas
  showMyTouchTargets(myButtons, nRebootButtons);   // optionally show this view's touch targets
  showScreenBorder();                              // optionally outline visible area
  showScreenCenterline();                          // optionally draw visual alignment bar

  // ----- draw text fields
  for (int ii = 0; ii < nRebootValues; ii++) {
    txtValues[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii = 0; ii < nRebootButtons; ii++) {
    FunctionButton item = myButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y + item.h / 2 + 5);   // place text centered inside button
    tft->setTextColor(item.color);
    tft->print(item.text);
  }

  updateScreen();   // update UI immediately, don't wait for the main loop to eventually get around to it
}   // end startScreen()

bool ViewReboot::onTouch(Point touch) {
  logger.info("->->-> Touched reboot screen.");
  bool handled = false;   // assume a touch target was not hit
  for (int ii = 0; ii < nRebootButtons; ii++) {
    FunctionButton item = myButtons[ii];
    if (item.hitTarget.contains(touch)) {
      handled = true;               // hit!
      switch (item.functionIndex)   // do the thing
      {
      case eCANCEL:
        fCancel();
        break;
      case eREBOOT:
        fReboot();
        break;
      default:
        logger.error("Error, unknown function ", item.functionIndex);
        break;
      }
    }
  }
  return handled;   // true=handled, false=controller uses default action
}   // end onTouch()
