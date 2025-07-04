// Please format this file with clang before check-in to GitHub
/*
  This is a playground for animating triangles.

  Version history:
            2025-06-01 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include "constants.h"          // Griduino constants, colors, typedefs
#include "hardware.h"           // Griduino pin definitions
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "TFT_Compass.h"        // Compass and speedometer

// ------- Identity for splash screen and console --------
#define PROGRAM_NAME "Triangles Demo"

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// ---------- TFT display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ========== helpers ==========================================

void drawGridOutline() {
  tft.drawRect(gMarginX, gMarginY, gBoxWidth, gBoxHeight, ILI9341_CYAN);
}

// ========== text screen layout ===================================

// ----- dynamic screen text
TextField txtSpeedo = {"55 mph", 102, gMarginY + 20, cSPEEDOMETER, ALIGNLEFT, eFONTSMALL};   // SPEEDOMETER

// create an instance of the Compass display
const Point center     = {320 / 2, 240 / 2};   // center point of compass (and triangle and screen)
const int radiusCircle = 90;                   // outer edge of compass rose circle
TFT_Compass compass(&tft, center, radiusCircle, &txtSpeedo);

// ------------ definitions
const int howLongToWait = 6;   // max number of seconds at startup waiting for Serial port to console

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connection to the PC
  // and the operator takes awhile to restart the IDE console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong * 1000;

  bool done = false;

  while (millis() < targetTime) {
    if (Serial)
      break;
    if (done)
      break;
    delay(15);
  }
}

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
  analogWrite(TFT_BL, 0xC0);   // backlight 75% brightness to reduce glare

  // ----- init TFT display
  tft.begin();                  // initialize TFT display
  tft.setRotation(LANDSCAPE);   // 1=landscape (default is 0=portrait)
  clearScreen(cBACKGROUND);     // note that "tft.begin()" does not clear screen

  // ----- init serial monitor (do not "Serial.print" or "logger.log" before this, it won't show up in console)
  Serial.begin(115200);   // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_NAME " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init Feather M4 onboard lights
  pinMode(RED_LED, OUTPUT);     // diagnostics RED LED
  digitalWrite(PIN_LED, LOW);   // turn off little red LED
  Serial.println("NeoPixel initialized and turned off");

  drawGridOutline();   // box outline around grid
  txtSpeedo.setBackground(cBACKGROUND);
  compass.setBackground(cBACKGROUND);
  compass.drawRose(center, radiusCircle);
  compass.drawCompassPoints();
}

//=========== main work loop ===================================
void loop() {
  // ----- update compass pointer
  compass.drawPointer(newAngle, newSpeed);   // compass pointer

  // ----- update speedometer
  char ss[40];
  snprintf(ss, sizeof(ss), "%d mph", newSpeed);
  txtSpeedo.print(ss);   // speedometer

  // increment loop variables
  const int stepDegrees = 2;   // warning: 3 degrees is too much, too chunky
  oldAngle              = newAngle;
  newAngle              = (newAngle + stepDegrees) % 360;   // wrap around from 360 to 0 degrees

  oldSpeed  = newSpeed;
  float rad = 2.0 * PI * (float)newAngle / 360.0;
  newSpeed  = (55.0 / 2.0) * (1.0 + cos(rad));   // simulate driving 0..55 mph

  delay(150);
}
