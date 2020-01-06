/* File: view_splash_screen.cpp

  When you power up the Griduino, you are greeted with this view.
  We announce ourselves and give credit to developers.

  +-----------------------------------+
  | Welcome:                          |
  |      Griduino                     |
  |       version 1.0                 |
  |                                   |
  |    Barry K7BWH                    |
  |    John KM7O                      |
  |                                   |
  +-----------------------------------+
*/

#include <Arduino.h>
#include <Adafruit_GFX.h>           // Core graphics display library
#include <Adafruit_ILI9341.h>       // TFT color display library
#include "constants.h"              // Griduino constant definitions

// ========== extern ==================================
extern Adafruit_ILI9341 tft;        // Griduino.ino
extern int gTextSize;               // no such function as "tft.getTextSize()" so remember it on our own
extern int gCharWidth, gCharHeight; // character cell size for TextSize(n)
extern int gUnitFontWidth, gUnitFontHeight; // character cell size for TextSize(1)

void showNameOfView(String sName, uint16_t fgd, uint16_t bkg);  // Griduino.ino
void initFontSizeSmall();           // Griduino.ino
void initFontSizeBig();             // Griduino.ino
int getOffsetToCenterText(String text); // Griduino.ino

// ------------ typedef's
typedef struct {
  int x;
  int y;
} Point;

// ========== constants ===============================
// vertical placement of text rows
const int gSplashyTitle = 84;       // ~= (gCharHeight * 2);
const int gSplashyVersion = 110;    // ~= (gSplashyTitle + gCharHeight);
const int gSplashyCredit1 = 164;    // ~= (gScreenHeight / 2);
const int gSplashyCredit2 = 196;    // ~= (gSplashyCredit1 + gCharHeight*1.5);

// ========== globals =================================

// ========== helpers =================================

// ========== splash screen view ========================
void startSplashScreen() {
  tft.fillScreen(cBACKGROUND);
  initFontSizeBig();

  // ----- large title
  // figure out how to center title l-r across screen
  int llX = getOffsetToCenterText(PROGRAM_TITLE);
  tft.setCursor(llX, gSplashyTitle);
  tft.setTextColor(ILI9341_WHITE);
  tft.print(PROGRAM_TITLE);

  // ----- version
  initFontSizeSmall();
  llX = getOffsetToCenterText(PROGRAM_VERSION);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(llX, gSplashyVersion);
  tft.print(PROGRAM_VERSION);

  // ----- small credits
  llX = getOffsetToCenterText(PROGRAM_LINE1);
  tft.setCursor(llX, gSplashyCredit1);
  tft.print(PROGRAM_LINE1);

  llX = getOffsetToCenterText(PROGRAM_LINE2);
  tft.setCursor(llX, gSplashyCredit2);
  tft.print(PROGRAM_LINE2);

  // ----- label this view in upper left corner
  //showNameOfView("Welcome!", ILI9341_YELLOW, ILI9341_BLUE);

#ifdef USE_MORSE_CODE
  // ----- announce in Morse code, so vehicle's driver doesn't have to look at the screen
  spkrMorse.setMessage(String(" de k7bwh  ")); // lowercase is required
  //spkrMorse.startSending();       // non-blocking (TODO: does not send evenly)
  spkrMorse.sendBlocking();         // blocking
#endif

  //delay(4000);                    // no delay - the controller handles the schedule
  //tft.fillScreen(cBACKGROUND);    // no clear - this screen is visible until the next view clears it
}
void updateSplashScreen() {
  // nothing to do in the main loop - this screen has no dynamic items
}
bool onTouchSplash(Point touch) {
  //Serial.println("->->-> Touched splash screen.");
  return false;                     // ignore touch, let controller handle with default action
}
