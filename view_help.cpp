/*
  File: view_help.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

            +-Hint:-----+-----------------------------+
            |           |                             |
            | Settings  |     Tap for next view       |
            |           |                             |
            |-----------+-----------------------------|
            |                                         |
            |    Tap to change brightness             |
            |                                         |
            +---------------------------------+-------+
   
*/

#include <Arduino.h>
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors
#include "TextField.h"              // Optimize TFT display text for proportional fonts
#include "view.h"                   // Base class for all views

// ========== extern ===========================================
extern Adafruit_ILI9341 tft;        // Griduino.ino

void showNameOfView(String sName, uint16_t fgd, uint16_t bkg);  // Griduino.ino
void setFontSize(int font);               // Griduino.ino
int getOffsetToCenterText(String text);   // Griduino.ino
int getOffsetToCenterTextOnButton(String text, int leftEdge, int width);  // Griduino.ino

// ============== constants ====================================

// ========== globals ==========================================

const int margin = 10;      // slight margin between button border and edge of screen
const int radius = 10;
// these are names for the array indexes, must be named in same order as below
enum txtIndex {
  SETTINGS=0, 
  NEXTVIEW, BRIGHTNESS
};

Button helpButtons[] = {
  //       text                x,y             w,h      r      color
  {"Settings",           margin,margin,      98,105, radius, cBUTTONLABEL},
  {"Tap for next view",  margin+108,margin, 192,105, radius, cBUTTONLABEL},
  {"Tap for brightness", margin,126,        300,105, radius, cBUTTONLABEL},
};
const int nHelpButtons = sizeof(helpButtons)/sizeof(helpButtons[0]);

// ========== helpers ==========================================

// ========== help screen view =================================
void updateHelpScreen() {
  // called on every pass through main()

  // nothing to do in the main loop - this screen has no dynamic items
}
void startHelpScreen() {
  // called once each time this view becomes active
  tft.fillScreen(cBACKGROUND);      // clear screen

  // ----- draw buttons
  setFontSize(12);
  for (int ii=0; ii<nHelpButtons; ii++) {
    Button item = helpButtons[ii];
    tft.fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft.drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- center label on top of button
    int x = getOffsetToCenterTextOnButton(item.text, item.x, item.w);
    
    tft.setCursor(x, item.y+item.h/2+5);
    tft.setTextColor(item.color);
    tft.print(item.text);

    #ifdef SHOW_TOUCH_TARGETS
    tft.drawRect(item.x-2, item.y-2,  // debug: draw outline around hit target
                 item.w+4, item.h+4, 
                 cWARN); 
    #endif

  }

  updateHelpScreen();               // fill in values immediately, don't wait for the main loop to eventually get around to it

  // ----- label this view in upper left corner
  showNameOfView("Hint: ", cWARN, cBACKGROUND);
}
bool onTouchHelp(Point touch) {
  Serial.println("->->-> Touched help screen.");
  return false;                     // true=handled, false=controller uses default action
}

// ========== class ViewHelp
void ViewHelp::updateScreen() {
  // called on every pass through main()
  ::updateHelpScreen();           // delegate to old code     TODO: migrate old code into new class
}
void ViewHelp::startScreen() {
  // called once each time this view becomes active
  ::startHelpScreen();            // delegate to old code     TODO: migrate old code into new class
}

bool ViewHelp::onTouch(Point touch) {
  return ::onTouchHelp(touch);    // delegate to old code     TODO: migrate old code into new class
}
