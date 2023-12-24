// Please format this file with clang before check-in to GitHub
/*
  TFT Touchscreen Demo - Touch screen with X, Y and Z (pressure) readings

  Date:     2023-12-12 improved debounce by adding hysteresis
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

// ------- TFT 4-Wire Resistive Touch Screen configuration parameters
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
#define XP_XM_OHMS 295   // Resistance in ohms between X+ and X- to calibrate touch pressure
                         // measure this with an ohmmeter while Griduino turned off

#define START_TOUCH_PRESSURE 200   // Minimum pressure threshold considered start of "press"
#define END_TOUCH_PRESSURE   50    // Maximum pressure threshold required before end of "press"
#define X_MIN_OHMS    140          // Expected range of measured X-axis readings
#define X_MAX_OHMS    800
#define Y_MIN_OHMS    320          // Expected range of measured Y-axis readings
#define Y_MAX_OHMS    760

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "Touch Screen Demo"
#define PROGRAM_VERSION "v1.14"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

#define SCREEN_ROTATION 1             // 1=landscape, 3=landscape 180-degrees

// ---------- Hardware Wiring ----------
/* Same as Griduino platform
*/

// TFT display and SD card share the hardware SPI interface, and have
// separate 'select' pins to identify the active device on the bus.

#if defined(SAMD_SERIES)
  #warning----- Compiling for Arduino Feather M4 Express -----
  // Adafruit Feather M4 Express pin definitions
  // To compile for Feather M0/M4, install "additional boards manager"
  // https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup
  
  #define TFT_BL   4                  // TFT backlight
  #define TFT_CS   5                  // TFT chip select pin
  #define TFT_DC  12                  // TFT display/command pin

#elif defined(ARDUINO_AVR_MEGA2560)
  #define TFT_BL   6                  // TFT backlight
  #define TFT_DC   9                  // TFT display/command pin
  #define TFT_CS  10                  // TFT chip select pin

#else
  #warning You need to define pins for your hardware
  #error Hardware platform unknown.
#endif

// ---------- TFT display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Touch Screen
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
#if defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  #define PIN_XP  A3    // Touchscreen X+ can be a digital pin
  #define PIN_XM  A4    // Touchscreen X- must be an analog pin, use "An" notation
  #define PIN_YP  A5    // Touchscreen Y+ must be an analog pin, use "An" notation
  #define PIN_YM   9    // Touchscreen Y- can be a digital pin
#elif defined(ARDUINO_AVR_MEGA2560)
  // Arduino Mega 2560 and others
  #define PIN_XP   4    // Touchscreen X+ can be a digital pin
  #define PIN_XM  A3    // Touchscreen X- must be an analog pin, use "An" notation
  #define PIN_YP  A2    // Touchscreen Y+ must be an analog pin, use "An" notation
  #define PIN_YM   5    // Touchscreen Y- can be a digital pin
#else
  #warning You need to define pins for your hardware

#endif
TouchScreen ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, XP_XM_OHMS);

// ------------ typedef's
struct Point {
  int x, y;
};

// ------------ definitions
#define gScreenWidth 320              // pixels wide, landscape orientation

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND     0x00A           // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cLABEL          ILI9341_GREEN
#define cVALUE          ILI9341_YELLOW  // 255, 255, 0
#define cINPUT          ILI9341_WHITE
#define cBUTTONFILL     ILI9341_NAVY
#define cBUTTONOUTLINE  ILI9341_BLUE    // 0,   0, 255 = darker than cyan
#define cTEXTCOLOR      ILI9341_CYAN    // 0, 255, 255
#define cTEXTFAINT      0x0514          // 0, 160, 160 = blue, between CYAN and DARKCYAN
#define cWARN           0xF844          // brighter than ILI9341_RED but not pink
#define cTOUCHTARGET    ILI9341_RED     // outline touch-sensitive areas

// ============== touchscreen helpers ==========================
bool gTouching = false;               // keep track of previous state
bool newScreenTap(Point* pPoint) {
  // find leading edge of a screen touch
  // returns TRUE only once on initial screen press
  // if true, also return screen coordinates of the touch

  bool result = false;                // assume no touch
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
      result = true;

      // touchscreen point object has (x,y,z) coordinates, where z = pressure
      TSPoint touch = ts.getPoint();

      // convert resistance measurements into screen pixel coords
      mapTouchToScreen(touch, pPoint);
      Serial.print("Screen touched at ("); Serial.print(pPoint->x);
      Serial.print(","); Serial.print(pPoint->y); Serial.println(")");
    }
  }
  //delay(10);   // no delay: code above completely handles debouncing without blocking the loop
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
  int z2 = 1023-analogRead(PIN_YP);

  return (uint16_t) ((z1+z2)/2);
}

// "isTouching()" is defined in touch.h but is not implemented Adafruit's TouchScreen library
// Note - For Griduino, if this function takes longer than 8 msec it can cause erratic GPS readings
// so we recommend against using https://forum.arduino.cc/index.php?topic=449719.0
bool TouchScreen::isTouching(void) {
  static bool button_state = false;
  uint16_t pres_val = ::myPressure();

  if ((button_state == false) && (pres_val > START_TOUCH_PRESSURE)) {
    Serial.print(". pressed, pressure = "); Serial.println(pres_val);     // debug
    button_state = true;
  }

  if ((button_state == true) && (pres_val < END_TOUCH_PRESSURE)) {
    Serial.print(". released, pressure = "); Serial.println(pres_val);       // debug
    button_state = false;
  }

  return button_state;
}

void mapTouchToScreen(TSPoint touch, Point* screen) {
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
  screen->x = map(touch.y,  X_MIN_OHMS,X_MAX_OHMS,    0, tft.width());
  screen->y = map(touch.x,  Y_MAX_OHMS,Y_MIN_OHMS,    0, tft.height());
  if (SCREEN_ROTATION == 3) {
    // if display is flipped, then also flip both x,y touchscreen coords
    screen->x = tft.width() - screen->x;
    screen->y = tft.height() - screen->y;
  }
  return;
}

// ========== splash screen helpers ============================
// splash screen layout
// When using default system fonts, screen pixel coordinates will identify top left of character cell

const int xLabel = 8;             // indent labels, slight margin on left edge of screen
#define yRow1   8                 // title: "Touchscreen Demo"
#define yRow2   yRow1 + 40        // program version
#define yRow3   yRow2 + 20        // compiled date
#define yRow4   yRow3 + 20        // author line 1
#define yRow5   yRow4 + 20        // author line 2
#define yRow6   yRow5 + 40        // "Pressure threshhold = "

void startSplashScreen() {
  tft.setTextSize(2);

  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR);
  tft.print(PROGRAM_TITLE);

  tft.setCursor(xLabel, yRow2);
  tft.setTextColor(cLABEL);
  tft.print(PROGRAM_VERSION);

  tft.setCursor(xLabel, yRow3);
  tft.print(__DATE__ " " __TIME__);  // Report our compiled date
  
  tft.setCursor(xLabel, yRow4);
  tft.println(PROGRAM_LINE1);

  tft.setCursor(xLabel, yRow5);
  tft.println(PROGRAM_LINE2);

  tft.setCursor(xLabel, yRow6);
  tft.setTextColor(cTEXTCOLOR);
  tft.print("Pressure threshhold: ");
  tft.print(ts.pressureThreshhold);
}

void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  static int addDotX = 10;   // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count   = 0;
  const int SCALEF   = 64;   // how much to slow it down so it becomes visible

  count = (count + 1) % SCALEF;
  if (count == 0) {
    addDotX = (addDotX + 1) % tft.width();    // advance
    rmvDotX = (rmvDotX + 1) % tft.width();    // advance
    tft.drawPixel(addDotX, row, foreground);  // write new
    tft.drawPixel(rmvDotX, row, background);  // erase old
  }
}

//=========== setup ============================================
void setup() {

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);           // init for debugging in the Arduino IDE

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init screen appearance
  tft.begin();                        // initialize TFT display
  tft.setRotation(SCREEN_ROTATION);   // landscape (default is portrait)
  clearScreen();

  // ----- init touch screen
  ts.pressureThreshhold = 200;

  // ----- announce ourselves
  startSplashScreen();

}

//=========== main work loop ===================================

void loop() {
  // a point object holds x y and z coordinates
  TSPoint p = ts.getPoint();          // read touch screen

  // ----- Testing the built-in Adafruit_TouchScreen library
  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means "no press"
  // Source:  File > Examples > Adafruit touchscreen > touchscreendemo
  if (p.z > ts.pressureThreshhold) {
    // send to debug console
    Serial.print("x,y = "); Serial.print(p.x);
    Serial.print(","); Serial.print(p.y);
    Serial.print("\tPressure = "); Serial.println(p.z);

    // show on screen
    tft.setCursor(p.x, p.y);
    tft.print("x");
  }

  // ----- Testing Barry's replacement "touch" functions
  // if there's touchscreen input, handle it
  Point touch;
  if (newScreenTap(&touch)) {

    const int radius = 2;    // debug
    tft.fillCircle(touch.x, touch.y, radius, cVALUE);  // debug
  
  }

  // small activity bar crawls along bottom edge to give
  // a sense of how frequently the main loop is executing
  showActivityBar(tft.height() - 1, ILI9341_RED, ILI9341_BLACK);
}
