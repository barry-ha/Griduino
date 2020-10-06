/*
 * File: TextField.cpp
 */

#include <Arduino.h>
//#include "Adafruit_GFX.h"           // Core graphics display library
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants, colors and typedefs
#include "TextField.h"              // Optimize TFT display text for proportional fonts

// ========== extern ==================================
extern Adafruit_ILI9341 tft;        // Griduino.ino  TODO: eliminate this global
extern void setFontSize(int font);  // Griduino.ino  TODO: eliminate this extern

uint16_t TextField::cBackground;    // background color

void TextField::eraseOld() {
  // we remember the area to erase from the previous print()
  tft.fillRect(xPrev, yPrev, wPrev, hPrev, cBackground); // erase the requested width of old text
  //tft.drawRect(xPrev-2, yPrev-2, wPrev+4, hPrev+4, ILI9341_RED); // debug: show what area was erased
}
void TextField::printNew(const char* pText) {
  int16_t x1, y1;
  uint16_t w, h;

  if (fontsize != eFONTUNSPEC) {
    setFontSize(fontsize);
  }

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

void TextButton::print() {            // override base class: buttons draw their own outline
  tft.fillRoundRect(buttonArea.ul.x, buttonArea.ul.y, 
                    buttonArea.size.x, buttonArea.size.y, 
                    radius, cBUTTONFILL);
  tft.drawRoundRect(buttonArea.ul.x, buttonArea.ul.y, 
                    buttonArea.size.x, buttonArea.size.y, 
                    radius, cBUTTONOUTLINE);

  // center text horizontally and vertically withing visible button boundary
  int16_t x1, y1;
  uint16_t w1, h1;
  tft.getTextBounds(text, buttonArea.ul.x, buttonArea.ul.y, &x1, &y1, &w1, &h1);

  int leftEdge = buttonArea.ul.x + buttonArea.size.x/2 - 1/2;
  int topEdge = buttonArea.ul.y + buttonArea.size.y/2;

  x = leftEdge;
  y = topEdge;

  Serial.print("Placement of text: "); Serial.println(text);
  char temp[255];
  snprintf(temp, sizeof(temp), 
          ". button outline (%d,%d,%d,%d), text posn(%d,%d)",
          buttonArea.ul.x, buttonArea.ul.y, buttonArea.size.x, buttonArea.size.y, leftEdge, topEdge);
  Serial.println(temp);   // debug

  #ifdef SHOW_TOUCH_TARGETS
  tft.drawRect(hitTarget.ul.x, hitTarget.ul.y,  // debug: draw outline around hit target
               hitTarget.size.x, hitTarget.size.y, 
               cWARN); 
  #endif
  
  // base class will draw text
  TextField::print(text);
}
