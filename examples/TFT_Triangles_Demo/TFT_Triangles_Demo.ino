// Please format this file with clang before check-in to GitHub
/*
  This is a playground for animating triangles.

  Version history:
            2025-06-01 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include "hardware.h"           // Griduino pin definitions
#include "constants.h"          // Griduino constants, colors, typedefs

// ------- Identity for splash screen and console --------
#define PROGRAM_NAME "Triangles Demo"

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ------------ definitions
const int howLongToWait = 5;   // max number of seconds at startup waiting for Serial port to console

void clearScreen(uint16_t color) {
  tft.fillScreen(color);
}

float angle = 0.0;   // radians
Point old0, old1, old2;

const Point center             = {320 / 2, 240 / 2};   // center point of triangle (and screen)
const int radiusBoundingCircle = 90;

//=========== setup ============================================
void setup() {

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 0xC0);   // backlight 75% brightness to reduce glare, screen is mostly white

  // ----- init TFT display
  tft.begin();                     // initialize TFT display
  tft.setRotation(LANDSCAPE);      // 1=landscape (default is 0=portrait)
  tft.fillScreen(ILI9341_BLACK);   // note that "begin()" does not clear screen

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);   // init for debugging in the Arduino IDE
  delay(20);              // minimal delay, this is the screen usually shown while waiting for Serial
  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_NAME " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init Feather M4 onboard lights
  pinMode(RED_LED, OUTPUT);     // diagnostics RED LED
  digitalWrite(PIN_LED, LOW);   // turn off little red LED
  Serial.println("NeoPixel initialized and turned off");

  clearScreen(ILI9341_WHITE);   // too slow and blinky for loop()

  // ----- draw compass rose
  tft.drawCircle(center.x, center.y, radiusBoundingCircle, ILI9341_DARKGREEN);

  tft.drawChar(center.x - 8, center.y - radiusBoundingCircle - 10, 'N', ILI9341_RED, ILI9341_WHITE, 3);
  tft.drawChar(center.x - 8, center.y + radiusBoundingCircle - 10, 'S', ILI9341_RED, ILI9341_WHITE, 3);
  tft.drawChar(center.x + radiusBoundingCircle - 6, center.y - 8, 'E', ILI9341_RED, ILI9341_WHITE, 3);
  tft.drawChar(center.x - radiusBoundingCircle - 6, center.y - 8, 'W', ILI9341_RED, ILI9341_WHITE, 3);
}

Point rotate(Point p, Point o, float radians) {
  // rotate a point (px, py) around another point (ox, oy) by a given angle θ
  // p - point to rotate
  // o - center to rotate around
  // returns new x,y coordinates
  // using the formulas:
  //    px' = cos(θ) * (px - ox) - sin(θ) * (py - oy) + ox
  //    py' = sin(θ) * (px - ox) + cos(θ) * (py - oy) + oy
  float rSin = sin(radians);
  float rCos = cos(radians);
  Point result;
  result.x = (int)(rCos * (p.x - o.x) - rSin * (p.y - o.y) + o.x);
  result.y = (int)(rSin * (p.x - o.x) + rCos * (p.y - o.y) + o.y);
  return result;
}

//=========== main work loop ===================================
void loop() {
  const int base   = 28;   // base width of triangle
  const int height = 58;   // height of triangle

  Point p0 = {center.x - base / 2, center.y + 0 * height / 8};
  Point p1 = {center.x + base / 2, center.y + 0 * height / 8};
  Point p2 = {center.x + 0, center.y - (8 * height / 8)};

  // rotate triangle (three points) around the center of the display
  Point new0 = rotate(p0, center, angle);
  Point new1 = rotate(p1, center, angle);
  Point new2 = rotate(p2, center, angle);

  tft.fillTriangle(old0.x, old0.y, old1.x, old1.y, old2.x, old2.y, ILI9341_WHITE);   // erase old
  tft.fillTriangle(new0.x, new0.y, new1.x, new1.y, new2.x, new2.y, ILI9341_BLACK);   // draw new

  old0 = new0;   // save to erase ourselves next pass
  old1 = new1;
  old2 = new2;

  // show pivot point
  tft.fillCircle(center.x, center.y, 4, ILI9341_RED);

  // increment angle of rotation
  const int stepDegrees = 2;   // warning: 3 degrees is too much, too chunky
  angle += (2.0 * 3.14159) / 360 * stepDegrees;
  delay(128);
}
