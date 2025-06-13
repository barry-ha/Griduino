#pragma once   // Please format this file with clang before check-in to GitHub
//------------------------------------------------------------------------------
//  File name: TFT_Compass.h
//
//  Description: A class that draws a directional arrow and vehicle speed on 320x240 TFT display
//
//  Version history:
//            2025-06-01 created
//
// Example Usage:
//      Declare             TFT_Compass compass(tft);
//      Draw compass rose   compass.rose(center, radius);
//      Draw pointer        compass.draw(angle in degrees);
//      Force redraw        compass.dirty();
//      Draw speedometer    compass.drawSpeedometer(mph, coord);
//
//  Notes:
//      1. Optimized pointer will not redraw given the same angle
//      2. Smallest angle change is 2 degrees
//      3. Pointer may flicker if called faster than 10 Hz
//      4. Origin is upper left corner
//      5. Uses 'const int' whenever possible for performance
//------------------------------------------------------------------------------

#include <Adafruit_ILI9341.h>   // TFT color display library
#include "hardware.h"           // Griduino pin definitions
#include "constants.h"          // Griduino constants, colors, typedefs
#include "TextField.h"          // Optimize TFT display text for proportional fonts

class TFT_Compass {
public:
  Adafruit_ILI9341 *tft;   // an instance of the TFT Display
  int oldDegrees = 0;       // previous compass pointer angle
  Point old0, old1, old2;   // cached corners of triangle pointer
  bool dirty = true;        // true=force redraw even if old=new
  uint16_t cForeground = cCOMPASSPOINTER;
  uint16_t cBackground = cBACKGROUND;

  // screen coordinates
  const Point center;       // screen coord of center of compass
  const int radiusCircle;   //
  const int base   = 22;    // base width of triangle
  const int height = 52;    // height of triangle
  TextField *speedometer;

  Point p0, p1, p2;        // starting corners of triangular pointer

  TFT_Compass(Adafruit_ILI9341 *vtft, Point vcenter, int vradius, TextField *vspeedo)   // ctor
      : tft(vtft), center(vcenter), radiusCircle(vradius), speedometer(vspeedo) {
    p0 = {center.x - base / 2, center.y};   // starting corners of triangular pointer
    p1 = {center.x + base / 2, center.y};
    p2 = {center.x + 0, center.y - height};
  }

  void drawRose(Point center, int radiusCircle) {
    // ----- draw compass rose
    tft->drawCircle(center.x, center.y, radiusCircle, cCOMPASSCIRCLE);
  }

  void drawCompassPoints() {
    int size = 2;   // size multiplier for single letter compass points
    // adjust letter placement to center it on the circle
    // 'drawChar' coords are the top left of a letter, so subtract half the text width (tw)
    int tw = (size * 7) / 2;

    tft->drawChar(center.x - tw, center.y - radiusCircle - tw, 'N', cCOMPASSLETTERS, cBACKGROUND, size);
    tft->drawChar(center.x - tw, center.y + radiusCircle - tw, 'S', cCOMPASSLETTERS, cBACKGROUND, size);
    tft->drawChar(center.x + radiusCircle - tw, center.y - tw, 'E', cCOMPASSLETTERS, cBACKGROUND, size);
    tft->drawChar(center.x - radiusCircle - tw, center.y - tw, 'W', cCOMPASSLETTERS, cBACKGROUND, size);
  }

  // helper
  Point rotate(Point p, Point o, float radians) {
    // rotate a point (px, py) around another point (ox, oy) by a given angle ?
    // p - point to rotate
    // o - center to rotate around
    // returns new x,y coordinates
    // using the formulas:
    //    px' = cos(?) * (px - ox) - sin(?) * (py - oy) + ox
    //    py' = sin(?) * (px - ox) + cos(?) * (py - oy) + oy
    float rSin = sin(radians);
    float rCos = cos(radians);
    Point result;
    result.x = (int)(rCos * (p.x - o.x) - rSin * (p.y - o.y) + o.x);
    result.y = (int)(rSin * (p.x - o.x) + rCos * (p.y - o.y) + o.y);
    return result;
  }
  void setBackground(uint16_t color) {
    this->cBackground = color;
  }

  //=========== main work routines ===================================
  void drawPointer(int speed, int degrees) {
    // speed   = mph or kph
    // degrees = compass direction, 0 = north
    if (degrees != oldDegrees || dirty) {
      // convert compass degrees (North=0) to screen angle (East=0)
      float angle = (degrees + 90) * radiansPerDegree;  

      // rotate triangle (three points) around the center of the compass
      Point new0 = rotate(p0, center, angle);
      Point new1 = rotate(p1, center, angle);
      Point new2 = rotate(p2, center, angle);

      tft->fillTriangle(old0.x, old0.y, old1.x, old1.y, old2.x, old2.y, cBackground);       // erase old
      if (speed == 0) {
        tft->fillTriangle(new0.x, new0.y, new1.x, new1.y, new2.x, new2.y, cDISABLED);   // draw new
      } else {
        tft->fillTriangle(new0.x, new0.y, new1.x, new1.y, new2.x, new2.y, cCOMPASSPOINTER);   // draw new
      }

      old0  = new0;   // save to erase ourselves next pass
      old1  = new1;
      old2  = new2;
      dirty = false;

      // show pivot point
      tft->fillCircle(center.x, center.y, 4, cCOMPASSPIVOT);
    }
  }

  void drawSpeedometer(int speed, int angle) {
    // speed = mph or kph, 0..999
    // angle = direction of travel, degrees 0..359 (unused)
    speedometer->print(speed);
  }

};   // end class TFT_Compass
