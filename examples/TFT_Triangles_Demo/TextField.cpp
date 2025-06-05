// Please format this file with clang before check-in to GitHub
/*
  File:     TextField.cpp - screen handling

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

*/

#include <Arduino.h>            // for "strncpy" and others
#include "Adafruit_ILI9341.h"   // TFT color display library
#include "constants.h"          // Griduino constants, colors and typedefs
#include "TextField.h"          // Optimize TFT display text for proportional fonts

// ========== extern ===========================================
extern Adafruit_ILI9341 tft;         // Griduino.ino  TODO: eliminate this global
extern void setFontSize(int font);   // Griduino.ino  TODO: eliminate this extern

uint16_t TextField::cBackground;   // background color

// ========== TextField ===============================
void TextField::eraseOld() {
  // we remember the area to erase from the previous print()
  // dump();                                                                 // debug
  // tft.drawRect(xPrev - 2, yPrev - 2, wPrev + 4, hPrev + 4, ILI9341_RED);  // debug
  tft.fillRect(xPrev, yPrev, wPrev, hPrev, cBackground);   // erase the requested width of old text
}
void TextField::printNew(const char *pText) {
  int16_t x1, y1;
  uint16_t w, h;

  if (fontsize != eFONTUNSPEC) {
    setFontSize(fontsize);
  }

  int leftedge = x;
  if (align == ALIGNCENTER) {
    // centered text left-right (ignore any given x-coordinate)
    tft.getTextBounds(pText, 0, y, &x1, &y1, &w, &h);
    leftedge = (tft.width() - w) / 2;
  } else if (align == ALIGNRIGHT) {
    tft.getTextBounds(pText, 0, y, &x1, &y1, &w, &h);
    leftedge = x - w;   // move text origin by width of text
  }

  tft.setCursor(leftedge, y);
  tft.setTextColor(color);
  tft.print(pText);

  // remember region so it can be erased next time
  tft.getTextBounds(pText, leftedge, y, &xPrev, &yPrev, &wPrev, &hPrev);
}

void TextButton::print() {   // override base class: buttons draw their own outline
  tft.fillRoundRect(buttonArea.ul.x, buttonArea.ul.y,
                    buttonArea.size.x, buttonArea.size.y,
                    radius, cBUTTONFILL);
  tft.drawRoundRect(buttonArea.ul.x, buttonArea.ul.y,
                    buttonArea.size.x, buttonArea.size.y,
                    radius, cBUTTONOUTLINE);

  // center text horizontally and vertically within visible button boundary
  int16_t x1, y1;
  uint16_t w1, h1;
  tft.getTextBounds(text, buttonArea.ul.x, buttonArea.ul.y, &x1, &y1, &w1, &h1);

  int leftEdge = buttonArea.ul.x + buttonArea.size.x / 2 - 1 / 2;
  int topEdge  = buttonArea.ul.y + buttonArea.size.y / 2;

  x = leftEdge;
  y = topEdge;

  char temp[255];
  snprintf(temp, sizeof(temp),
           ". button outline (%d,%d,%d,%d), text posn(%d,%d)",
           buttonArea.ul.x, buttonArea.ul.y, buttonArea.size.x, buttonArea.size.y, leftEdge, topEdge);

  // base class will draw text
  TextField::print(text);
}

// ========== font management helpers ==========================
/* Using fonts: https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts

  "Fonts" folder is inside \Documents\User\Arduino\libraries\Adafruit_GFX_Library\fonts
*/
#include "Fonts/FreeSans18pt7b.h"       // eFONTGIANT    36 pt (see constants.h)
#include "Fonts/FreeSansBold24pt7b.h"   // eFONTBIG      24 pt
#include "Fonts/FreeSans12pt7b.h"       // eFONTSMALL    12 pt
#include "Fonts/FreeSans9pt7b.h"        // eFONTSMALLEST  9 pt
// (built-in)                           // eFONTSYSTEM    8 pt

void setFontSize(int font) {
  // input: "font" = point size
  switch (font) {
  case 36:   // eFONTGIANT
    tft.setFont(&FreeSans18pt7b);
    tft.setTextSize(2);
    break;

  case 24:   // eFONTBIG
    tft.setFont(&FreeSansBold24pt7b);
    tft.setTextSize(1);
    break;

  case 12:   // eFONTSMALL
    tft.setFont(&FreeSans12pt7b);
    tft.setTextSize(1);
    break;

  case 9:   // eFONTSMALLEST
    tft.setFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    break;

  case 0:   // eFONTSYSTEM
    tft.setFont();
    tft.setTextSize(2);
    break;

  default:
    // logger.log(SCREEN, ERROR, "Error, unknown font size (%d)");
    break;
  }
}

int getOffsetToCenterText(String text) {
  // measure width of given text in current font and
  // calculate X-offset to make it centered left-right on screen
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);   // compute "pixels wide"
  return (gScreenWidth - w) / 2;
}

int getOffsetToCenterTextOnButton(String text, int leftEdge, int width) {
  // measure width of given text in current font and
  // calculate X-offset to make it centered left-right within given bounds
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);   // compute "pixels wide"
  return leftEdge + (width - w) / 2;
}
