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
#include "TFT_Compass.h"        // Draw directional arrow

// ------- Identity for splash screen and console --------
#define PROGRAM_NAME "Triangles Demo"

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// create an instance of the Compass display
const Point center           = {320 / 2, 240 / 2};             // center point of compass (and triangle and screen)
const int radiusCircle       = 90;                             // outer edge of compass rose circle
const Point upperSpeedometer = {320 / 2 - 50, 240 / 2 - 48};   // coord of MPH message above compass pointer
const Point lowerSpeedometer = {320 / 2 - 50, 240 / 2 + 24};   // coord of MPH message below compass pointer

TFT_Compass compass(&tft, center, radiusCircle, upperSpeedometer, lowerSpeedometer);

// ------------ definitions
const int howLongToWait = 5;   // max number of seconds at startup waiting for Serial port to console

void clearScreen(uint16_t color) {
  tft.fillScreen(color);
}

int newAngle = 0;   // degrees, 0=North
int oldAngle = 0;

int oldSpeed = 0;
int newSpeed = 0;

// display one OR the other speedometer, so it doesn't overlap the compass pointer

//=========== setup ============================================
void setup() {

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 0xC0);   // backlight 75% brightness to reduce glare,

  // ----- init TFT display
  tft.begin();                   // initialize TFT display
  tft.setRotation(LANDSCAPE);    // 1=landscape (default is 0=portrait)
  tft.fillScreen(cBACKGROUND);   // note that "begin()" does not clear screen

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

  compass.rose(center, radiusCircle);
}

//=========== main work loop ===================================
void loop() {
  // ----- update compass pointer
  compass.draw(newAngle);

  // ----- update speedometer
  compass.eraseSpeedometer(newAngle, oldAngle);
  compass.drawSpeedometer(newSpeed, newAngle);

  // increment loop variables
  const int stepDegrees = 2;   // warning: 3 degrees is too much, too chunky
  oldAngle              = newAngle;
  newAngle              = (newAngle + stepDegrees) % 360;   // wrap around from 360 to 0 degrees

  oldSpeed  = newSpeed;
  float rad = 2.0 * PI * (float)newAngle / 360.0;
  newSpeed  = (55.0/2.0) * (1.0 + cos(rad));   // simulate driving 0..55 mph

  delay(150);
}
