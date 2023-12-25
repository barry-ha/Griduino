// Please format this file with clang before check-in to GitHub
/*
  File:     Touch.cpp
  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Contains the touchscreen code to get it out of the way

*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include <Adafruit_GFX.h>       // Core graphics display library
#include <TouchScreen.h>        // Touchscreen built in to 3.2" Adafruit TFT display
#include "constants.h"          // Griduino constants, colors, typedefs
#include "hardware.h"           // Griduino pin definitions
#include "logger.h"             // conditional printing to Serial port

// ========== extern ===========================================
extern Logger logger;          // Griduino.ino
extern Adafruit_ILI9341 tft;   // Griduino.ino

// ---------- forward reference
void mapTouchToScreen(TSPoint touch, Point *screen);

// ---------- Touch Screen
TouchScreen ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, XP_XM_OHMS);

void initTouchScreen(void) {
  ts.pressureThreshhold = START_TOUCH_PRESSURE;
}

// ============== touchscreen helpers ==========================

bool gTouching = false;   // keep track of previous state
bool newScreenTap(Point *pPoint) {
  // find leading edge of a screen touch
  // returns TRUE only once on initial screen press
  // if true, also return screen coordinates of the touch

  bool result = false;   // assume no touch
  if (gTouching) {
    // the touch was previously processed, so ignore continued pressure until they let go
    if (!ts.isTouching()) {
      // Touching ==> Not Touching transition
      gTouching = false;
    }
  } else {
    // here, we know the screen was not being touched in the last pass,
    // so look for a new touch on this pass
    // Our replacement "isTouching" function does some of the debounce and threshold detection needed
    if (ts.isTouching()) {
      gTouching = true;
      result    = true;

      // touchscreen point object has (x,y,z) coordinates, where z = pressure
      TSPoint touch = ts.getPoint();

      // convert resistance measurements into screen pixel coords
      int orientation = tft.getRotation();
      mapTouchToScreen(touch, pPoint);
      logger.info("Screen touched at (%d,%d)", pPoint->x, pPoint->y);
    }
  }
  // delay(10);   // no delay: code above completely handles debouncing without blocking the loop
  return result;
}

// 2020-05-12 barry@k7bwh.com
// We need to replace TouchScreen::pressure() and implement TouchScreen::isTouching()

// 2020-05-03 CraigV and barry@k7bwh.com
uint16_t myPressure(void) {
  pinMode(PIN_XP, OUTPUT);
  digitalWrite(PIN_XP, LOW);    // Set X+ to ground
  pinMode(PIN_YM, OUTPUT);      //
  digitalWrite(PIN_YM, HIGH);   // Set Y- to VCC

  digitalWrite(PIN_XM, LOW);
  pinMode(PIN_XM, INPUT);      // Set X- to Hi-Z
  digitalWrite(PIN_YP, LOW);   //
  pinMode(PIN_YP, INPUT);      // Set Y+ to Hi-Z

  int z1 = analogRead(PIN_XM);
  int z2 = 1023 - analogRead(PIN_YP);

  return (uint16_t)((z1 + z2) / 2);
}

// "isTouching()" is defined in touch.h but is not implemented Adafruit's TouchScreen library
// Note - For Griduino, if this function takes longer than 8 msec it can cause erratic GPS readings
// so we recommend against using https://forum.arduino.cc/index.php?topic=449719.0
bool TouchScreen::isTouching(void) {
  static bool button_state = false;
  uint16_t pres_val        = ::myPressure();

  if ((button_state == false) && (pres_val > START_TOUCH_PRESSURE)) {
    // Serial.print(". finger pressure = "); Serial.println(pres_val);     // debug
    button_state = true;
  }

  if ((button_state == true) && (pres_val < END_TOUCH_PRESSURE)) {
    // Serial.print(". released, pressure = "); Serial.println(pres_val);       // debug
    button_state = false;
  }

  return button_state;
}

void mapTouchToScreen(TSPoint touch, Point *screen) {
  // convert from X+,Y+ resistance measurements to screen coordinates
  // param touch = resistance readings from touchscreen
  // param screen = result of converting touch into screen coordinates
  //
  // Measured readings in Barry's landscape orientation were:
  //   +---------------------+ X=876
  //   |                     |
  //   |                     |
  //   |                     |
  //   +---------------------+ X=160
  //  Y=110                Y=892
  //
  // Typical measured pressures=200..549

  // setRotation(1) = landscape orientation = x-,y-axis exchanged
  //          map(value         in_min,in_max,       out_min,out_max)
  screen->x = map(touch.y, Y_MIN_OHMS, Y_MAX_OHMS, 0, tft.width());
  screen->y = map(touch.x, X_MAX_OHMS, X_MIN_OHMS, 0, tft.height());

  // keep all touches within boundaries of the screen
  screen->x = constrain(screen->x, 0, tft.width());
  screen->y = constrain(screen->y, 0, tft.height());

  if (tft.getRotation() == 3) {
    // if display is flipped, then also flip both x,y touchscreen coords
    screen->x = tft.width() - screen->x;
    screen->y = tft.height() - screen->y;
  }
  return;
}
