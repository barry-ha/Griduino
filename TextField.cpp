/*
 * File: TextField.cpp
 */

#include <Arduino.h>
#include "Adafruit_GFX.h"           // Core graphics display library
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "TextField.h"              // Optimize TFT display text for proportional fonts

// ========== extern ==================================
extern Adafruit_ILI9341 tft;        // Griduino.ino  TODO: eliminate this global

uint16_t TextField::cBackground;    // background color

void TextField::eraseOld() {
  // we remember the area to erase from the previous print()
  tft.fillRect(xPrev, yPrev, wPrev, hPrev, cBackground); // erase the requested width of old text
  //tft.drawRect(xPrev-2, yPrev-2, wPrev+4, hPrev+4, ILI9341_RED); // debug: show what area was erased
}
void TextField::printNew(const char* pText) {
  int16_t x1, y1;
  uint16_t w, h;

  int leftedge = x;
  if (align == ALIGNCENTER) {
    // centered text left-right (ignore any given x-coordinate)
    tft.getTextBounds(pText, 0, y, &x1, &y1, &w, &h);
    leftedge = (tft.width() - w)/2;
  }
  else if (align == ALIGNRIGHT) {
    tft.getTextBounds(pText, 0, y, &x1, &y1, &w, &h);
    leftedge = x - w;    // move text origin by width of text
  }
  tft.setCursor(leftedge, y);
  tft.setTextColor(color);
  tft.print(pText);

  // remember region so it can be erased next time
  tft.getTextBounds(pText, leftedge, y, &xPrev, &yPrev, &wPrev, &hPrev);
}
