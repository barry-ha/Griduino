/*
  File: view_help_screen.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  +-----------------------------------------+
  | Hint:                                   |
  |    Tap to change view                   |
  |                                         |
  |-----------------------------------------|
  |                                         |
  |    Tap to change brightness             |
  |                                         |
  +-------+-------------------------+-------+
   
*/

#include <Arduino.h>
#include "Adafruit_GFX.h"           // Core graphics display library
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors

// ========== extern ===========================================
extern Adafruit_ILI9341 tft;        // Griduino.ino

void showNameOfView(String sName, uint16_t fgd, uint16_t bkg);  // Griduino.ino
void setFontSize(int font);            // Griduino.ino
int getOffsetToCenterText(String text); // Griduino.ino

// ============== constants ====================================

// ========== globals ==========================================

const int nHelpButtons = 2;
const int margin = 10;      // slight margin between button border and edge of screen
const int radius = 10;
Button helpButtons[nHelpButtons] = {
  //       text                  x,y           w,h        r      color
  {"Tap to change view",       margin,margin,  300, 100,  radius, cBUTTONLABEL},  // width=gScreenWidth - margin*2, height=gScreenHeight/2 - margin*2
  {"Tap to change brightness", margin,126,     300, 100,  radius, cBUTTONLABEL},
};

// ========== helpers ==========================================

// ========== help screen view =================================
void updateHelpScreen() {
  // called on every pass through main()

  // nothing to do in the main loop - this screen has no dynamic items
}
void startHelpScreen() {
  // called once each time this view becomes active
  tft.fillScreen(cBACKGROUND);      // clear screen
  setFontSize(12);

  // ----- draw buttons
  for (int ii=0; ii<nHelpButtons; ii++) {
    Button item = helpButtons[ii];
    tft.fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft.drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterText(item.text);
    tft.setCursor(xx, item.y+32);
    tft.setTextColor(item.color);
    tft.print(item.text);
  }

  updateHelpScreen();               // fill in values immediately, don't wait for the main loop to eventually get around to it

  // ----- label this view in upper left corner
  showNameOfView("Hint: ", cWARN, cBACKGROUND);
}
bool onTouchHelp(Point touch) {
  Serial.println("->->-> Touched help screen.");
  return false;                     // true=handled, false=controller uses default action
}
