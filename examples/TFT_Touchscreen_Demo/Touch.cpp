// Please format this file with clang before check-in to GitHub
/*
  File:     Touch.cpp
  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Contains the touchscreen code to get it out of the way

*/

#include <Adafruit_GFX.h>   // Core graphics display library
#include <TouchScreen.h>    // Touchscreen built in to 3.2" Adafruit TFT display
#include "constants.h"      // Griduino constants, colors, typedefs
#include "hardware.h"       // Griduino pin definitions

// ---------- Touch Screen
TouchScreen ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, XP_XM_OHMS);

void initTouchScreen(void) {
  ts.pressureThreshhold = START_TOUCH_PRESSURE;
}

// ============== touchscreen helpers ==========================

void mapTouchToScreen(TSPoint touchOhms, TSPoint *screenCoord, int orientation) {
  // convert from X+,Y+ resistance measurements to screen coordinates
  // param touchOhms = resistance readings from touchscreen
  // param screenCoord = result of converting touchOhms into screen coordinates
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
  //          map(value    in_min,in_max,          out_min,out_max)
  screenCoord->x = map(touchOhms.y, X_MIN_OHMS, X_MAX_OHMS, 0, gScreenWidth);
  screenCoord->y = map(touchOhms.x, Y_MAX_OHMS, Y_MIN_OHMS, 0, gScreenHeight);
  screenCoord->z = touchOhms.z;

  // keep all touches within boundaries of the screenCoord
  screenCoord->x = constrain(screenCoord->x, 0, gScreenWidth);
  screenCoord->y = constrain(screenCoord->y, 0, gScreenHeight);

  if (orientation == FLIPPED_LANDSCAPE) {
    // if display is flipped, then also flip both x,y touchscreen coords
    screenCoord->x = gScreenWidth - screenCoord->x;
    screenCoord->y = gScreenHeight - screenCoord->y;
  }
  return;
}

bool gTouching = false;   // keep track of previous state
bool newScreenTap(TSPoint *pScreenCoord, int orientation) {
  // find leading edge of a screen touch
  // returns TRUE only once on initial screen press
  // if true, also return screen coordinates and touch pressure

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
    // Our replacement "isTouching" function does most of the debounce and threshhold detection needed
    if (ts.isTouching()) {
      gTouching = true;
      result    = true;

      // touchscreen point object has (x/Ohms,y/Ohms,z/Pressure)
      TSPoint touch = ts.getPoint();

      // convert resistance measurements into screen pixel coords
      mapTouchToScreen(touch, pScreenCoord, orientation);
      Serial.print("Screen touched at (");
      Serial.print(pScreenCoord->x);
      Serial.print(",");
      Serial.print(pScreenCoord->y);
      Serial.print(") pressure=");
      Serial.println(pScreenCoord->z);
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
    Serial.print(". pressed, pressure = ");
    Serial.println(pres_val);   // debug
    button_state = true;
  }

  if ((button_state == true) && (pres_val < END_TOUCH_PRESSURE)) {
    Serial.print(". released, pressure = ");
    Serial.println(pres_val);   // debug
    button_state = false;
  }

  // Clean the touchScreen settings after function is used
  // Because LCD may use the same pins
  // todo - is this actually necessary?
  // pinMode(_xm, OUTPUT);     digitalWrite(_xm, LOW);
  // pinMode(_yp, OUTPUT);     digitalWrite(_yp, HIGH);
  // pinMode(_ym, OUTPUT);     digitalWrite(_ym, LOW);
  // pinMode(_xp, OUTPUT);     digitalWrite(_xp, HIGH);

  return button_state;
}
