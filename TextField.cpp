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
  // find the height of erasure
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(prevText, x, y, &x1, &y1, &w, &h);

  if (align == FLUSHRIGHT) {
    x1 -= w;          // move erasure by width of text
  }

  tft.fillRect(x1-2, y1-2, w+4, h+4, cBackground); // erase the requested width of old text
  //tft.drawRect(x1-2, y1-2, w+4, h+4, ILI9341_RED); // debug: show what area was erased
}
void TextField::printNew(const char* pText) {
  int leftedge = x;
  if (align == FLUSHRIGHT) {
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(pText, x, y, &x1, &y1, &w, &h);
    leftedge -= w;    // move text origin by width of text
  }
  tft.setCursor(leftedge, y);
  tft.setTextColor(color);
  tft.print(pText);
}
