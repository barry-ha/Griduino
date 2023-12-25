// Please format this file with clang before check-in to GitHub
/*
  TFT Touchscreen Demo - Touch screen with X, Y and Z (pressure) readings

  Version history:
            2023-12-24 improved debounce by adding hysteresis
            2019-11-15 created v6
            2020-05-12 updated TouchScreen code

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Touch screen library with X Y and Z (pressure) readings as well
            as oversampling to reduce 'bouncing'
            This demo code returns raw readings.
            Use SERIAL MONITOR in the Arduino workbench to see results of touching screen.

  Test 1:   Let Griduino run idle for several minutes, without touching anything.
            Watch the serial console output.
  Result 1: Several bogus results are logged, e.g.:
                09:00:06.841 -> x,y = 2,759	Pressure = 294
                09:02:30.939 -> x,y = 286,740	Pressure = 18669
                09:05:36.706 -> x,y = 2,760	Pressure = 294
                09:06:21.323 -> x,y = 310,755	Pressure = 18182
                09:06:21.508 -> x,y = 2,760	Pressure = 294
            These show that built-in library Adafruit_Touchscreen is _not_ reliable.

  Test 2:   Gently press TFT screen with small tipped pointer to add yellow dots.
            Verify the yellow dots are near the actual touches.
            These should show that Barry's replacement software _is_ reliable.

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)     https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341    https://www.adafruit.com/product/1743
            How to:      https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2
            SPI Wiring:  https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/spi-wiring-and-test
            Touchscreen: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/resistive-touchscreen

*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include <TouchScreen.h>        // Touchscreen built in to 3.2" Adafruit TFT display
#include "constants.h"          // Griduino constants, colors, typedefs
#include "hardware.h"           // Griduino pin definitions

// ------- Identity for splash screen and console --------
#define PROGRAM_NAME    "Touch Screen Demo"

// ---------- extern
extern bool newScreenTap(TSPoint *pPoint, int orientation);   // Touch.cpp
extern void initTouchScreen(void);                            // Touch.cpp

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Touch Screen
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
TouchScreen tsn = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, XP_XM_OHMS);

// ============== touchscreen helpers ==========================
// ========== splash screen helpers ============================
// splash screen layout
// When using default system fonts, screen pixel coordinates will identify top left of character cell

const int xLabel = 8;      // indent labels, slight margin on left edge of screen
#define yRow1 8            // title: "Touchscreen Demo"
#define yRow2 yRow1 + 40   // program version
#define yRow3 yRow2 + 20   // compiled date
#define yRow4 yRow3 + 20   // author line 1
#define yRow5 yRow4 + 20   // author line 2
#define yRow6 yRow5 + 40   // "Pressure threshhold = "

void startSplashScreen() {
  tft.setTextSize(2);

  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR);
  tft.print(PROGRAM_NAME);

  tft.setCursor(xLabel, yRow2);
  tft.setTextColor(cLABEL);
  tft.print(PROGRAM_VERSION);

  tft.setCursor(xLabel, yRow3);
  tft.print(__DATE__ " " __TIME__);   // Report our compiled date

  tft.setCursor(xLabel, yRow4);
  tft.println(PROGRAM_LINE1);

  tft.setCursor(xLabel, yRow5);
  tft.println(PROGRAM_LINE2);

  tft.setCursor(xLabel, yRow6);
  tft.setTextColor(cTEXTCOLOR);
  tft.print("Pressure threshhold: ");
  tft.print(START_TOUCH_PRESSURE);
}

void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  static int addDotX = 10;   // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count   = 0;
  const int SCALEF   = 32;   // how much to slow it down so it becomes visible

  count = (count + 1) % SCALEF;
  if (count == 0) {
    addDotX = (addDotX + 1) % tft.width();     // advance
    rmvDotX = (rmvDotX + 1) % tft.width();     // advance
    tft.drawPixel(addDotX, row, foreground);   // write new
    tft.drawPixel(rmvDotX, row, background);   // erase old
  }
}

//=========== setup ============================================
void setup() {

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);   // start at full brightness

  // ----- init TFT display
  tft.begin();                         // initialize TFT display
  tft.setRotation(eSCREEN_ROTATE_0);   // 1=landscape (default is 0=portrait)
  clearScreen();                       // note that "begin()" does not clear screen

  // ----- announce ourselves
  startSplashScreen();

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);   // init for debugging in the Arduino IDE

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_NAME " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init touch screen
  initTouchScreen();

}

//=========== main work loop ===================================

void loop() {
  // a point object holds x y and z coordinates
  TSPoint p = tsn.getPoint();   // read touch screen

  // ----- Testing the built-in Adafruit_TouchScreen library
  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means "no press"
  // Source:  File > Examples > Adafruit touchscreen > touchscreendemo
  if (p.z > tsn.pressureThreshhold) {
    // send to debug console
    Serial.print("x,y = ");
    Serial.print(p.x);
    Serial.print(",");
    Serial.print(p.y);
    Serial.print("\tPressure = ");
    Serial.println(p.z);

    // show on screen
    tft.setCursor(p.x, p.y);
    tft.print("x");
  }

  // ----- Testing Barry's replacement "touch" functions
  // if there's touchscreen input, handle it
  TSPoint screenLoc;
  if (newScreenTap(&screenLoc, tft.getRotation())) {

    // debug: show where touched
    const int radius = 2;
    tft.fillCircle(screenLoc.x, screenLoc.y, radius, cTOUCHTARGET);
  }

  // small activity bar crawls along bottom edge to give
  // a sense of how frequently the main loop is executing
  showActivityBar(tft.height() - 1, ILI9341_RED, ILI9341_BLACK);
}
