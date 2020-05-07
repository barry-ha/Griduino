/*
  TFT Touchscreen Demo - Touch screen with X, Y and Z (pressure) readings

  Date:     2019-11-15 created v6

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Touch screen library with X Y and Z (pressure) readings as well
            as oversampling to avoid 'bouncing'
            This demo code returns raw readings.
            Use SERIAL MONITOR in the Arduino workbench to see results of touching screen.

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)
            Spec: https://www.adafruit.com/product/3857
        -OR- 
            Arduino Mega 2560 Rev3 (16 MHz ATmega2560)
            Spec: https://www.adafruit.com/product/191

         2. Adafruit 3.2" TFT color LCD display ILI-9341
            Spec: http://adafru.it/1743
            How to: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2
            SPI Wiring: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/spi-wiring-and-test
            Touchscreen: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/resistive-touchscreen

*/

#include "SPI.h"                  // Serial Peripheral Interface
#include "Adafruit_GFX.h"         // Core graphics display library
#include "Adafruit_ILI9341.h"     // TFT color display library
#include "TouchScreen.h"          // Touchscreen built in to 3.2" Adafruit TFT display

// ------- Identity for console
#define PROGRAM_TITLE   "Touch Screen Demo"
#define PROGRAM_VERSION "v0.9"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"

#define SCREEN_ROTATION 3         // 1=landscape rsu, 3=landscape usd

// ---------- Hardware Wiring ----------
/*                                Arduino       Adafruit
  ___Label__Description______________Mega_______Feather M4__________Resource____
TFT Power:
   GND  - Ground                  - ground      - J2 Pin 13
   VIN  - VCC                     - 5v          - Pin 10 J5 Vusb
TFT Resistive touch:
   X+   - Touch Horizontal axis   - Digital  4  - A3  (Pin 4 J5)
   X-   - Touch Horizontal        - Analog  A3  - A4  (J2 Pin 8)  - uses analog A/D
   Y+   - Touch Vertical axis     - Analog  A2  - A5  (J2 Pin 7)  - uses analog A/D
   Y-   - Touch Vertical          - Digital  5  - D9  (Pin 5 J5)
*/

// TFT display and SD card share the hardware SPI interface, and have
// separate 'select' pins to identify the active device on the bus.
#if defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  // To compile for Feather M0/M4, install "additional boards manager"
  // https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup
  
  #define TFT_BL   4    // TFT backlight
  #define TFT_CS   5    // TFT chip select pin
  #define TFT_DC  12    // TFT display/command pin

#elif defined(ARDUINO_AVR_MEGA2560)
  #define TFT_BL   6    // TFT backlight
  #define SD_CCS   7    // SD card select pin - Mega
  #define SD_CD    8    // SD card detect pin - Mega
  #define TFT_DC   9    // TFT display/command pin
  #define TFT_CS  10    // TFT chip select pin

#else
  // todo: Unknown platform
  #warning You need to define pins for your hardware

#endif

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Touch Screen
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// The demo program used 300 ohms across the X plate
// Barry's display, ILI-9341, measured 295 ohms across the X plate.
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
  // todo: Unknown platform
  #warning You need to define pins for your hardware

#endif
TouchScreen ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, 295);

// ------------ typedef's
typedef struct {
  int x;
  int y;
} Point;

// ------------ definitions
#define gScreenWidth 320      // pixels wide

// ----- screen layout
// using default fonts - screen pixel coordinates will identify top left of character cell

// splash screen layout
const int xLabel = 8;             // indent labels, slight margin on left edge of screen
#define yRow1   0                 // program title: "Griduino GMT Clock"
#define yRow2   yRow1 + 40        // program version
#define yRow3   yRow2 + 20        // compiled date
#define yRow4   yRow3 + 20        // author line 1
#define yRow5   yRow4 + 20        // author line 2
#define yRow6   yRow5 + 40        // "Pressure threshhold = "


// ----- color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND     0x00A             // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cTEXTCOLOR      ILI9341_CYAN      // 0, 255, 255
#define cTEXTFAINT      0x514             // 0, 160, 160 = blue, between CYAN and DARKCYAN
#define cLABEL          ILI9341_GREEN
#define cVALUE          ILI9341_YELLOW
#define cINPUT          ILI9341_WHITE
#define cBUTTONFILL     ILI9341_NAVY
#define cBUTTONOUTLINE  ILI9341_BLUE      // 0,   0, 255 = darker than cyan
#define cWARN           0xF844            // brighter than ILI9341_RED but not pink

// ============== touchscreen helpers ==========================

bool gTouching = false;             // keep track of previous state
bool newScreenTap(Point* pPoint) {
  // find leading edge of a screen touch
  // returns TRUE only once on initial screen press
  // if true, also return screen coordinates of the touch

  bool result = false;        // assume no touch
  if (gTouching) {
    // the touch was previously processed, so ignore continued pressure until they let go
    if (!ts.isTouching()) {
      // Touching ==> Not Touching transition
      gTouching = false;
    }
  } else {
    // here, we know the screen was not being touched in the last pass,
    // so look for a new touch on this pass
    // The built-in "isTouching" function does most of the debounce and threshhold detection needed
    if (ts.isTouching()) {
      gTouching = true;
      result = true;

      // touchscreen point object has (x,y,z) coordinates, where z = pressure
      TSPoint touch = ts.getPoint();

      // convert resistance measurements into screen pixel coords
      mapTouchToScreen(touch, pPoint);
      Serial.print("Screen touch detected ("); Serial.print(pPoint->x);
      Serial.print(","); Serial.print(pPoint->y); Serial.println(")");
    }
  }
  //delay(100);   // no delay: code above completely handles debouncing without blocking the loop
  return result;
}

// 2020-05-03 CraigV and barry@k7bwh.com
// "isTouching()" is defined in touch.h but is not implemented Adafruit's TouchScreen library
// Note - For Griduino, if this function takes longer than 8 msec it can cause erratic GPS readings
// so we recommend against using https://forum.arduino.cc/index.php?topic=449719.0
bool TouchScreen::isTouching(void) {
  #define TOUCHPRESSURE 200       // Minimum pressure we consider true pressing
  static bool button_state = false;
  uint16_t pres_val = pressure();

  if ((button_state == false) && (pres_val > TOUCHPRESSURE)) {
    Serial.println("button down");     // debug
    button_state = true;
  }

  if ((button_state == true) && (pres_val < TOUCHPRESSURE)) {
    Serial.println("button up");       // debug
    button_state = false;
  }

  // Clean the touchScreen settings after function is used
  // Because LCD may use the same pins
  // todo - is this actually necessary?
  //pinMode(_xm, OUTPUT);     digitalWrite(_xm, LOW);
  //pinMode(_yp, OUTPUT);     digitalWrite(_yp, HIGH);
  //pinMode(_ym, OUTPUT);     digitalWrite(_ym, LOW);
  //pinMode(_xp, OUTPUT);     digitalWrite(_xp, HIGH);

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
  //          map(value    in_min,in_max, out_min,out_max)
  screen->x = map(touch.y,  225,825,      0, tft.width());
  screen->y = map(touch.x,  800,300,      0, tft.height());
  if (SCREEN_ROTATION == 3) {
    // if display is flipped, then also flip both x,y touchscreen coords
    screen->x = tft.width() - screen->x;
    screen->y = tft.height() - screen->y;
  }
  return;
}

// ========== screen helpers ===================================
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
  static int addDotX = 10;                   // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count = 0;
  const int SCALEF = 32;                     // how much to slow it down so it becomes visible

  count = (count + 1) % SCALEF;
  if (count == 0) {
    addDotX = (addDotX + 1) % gScreenWidth;   // advance
    rmvDotX = (rmvDotX + 1) % gScreenWidth;   // advance
    tft.drawPixel(addDotX, row, foreground);   // write new
    tft.drawPixel(rmvDotX, row, background);   // erase old
  }
}

//=========== setup ============================================
void setup() {

  // ----- init serial monitor
  Serial.begin(115200);           // init for debuggging in the Arduino IDE

  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " __DATE__ " " __TIME__);  // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init TFT display
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
  TSPoint p = ts.getPoint();     // read touch screen

  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means "no press"
  if (p.z > ts.pressureThreshhold) {
     Serial.print("x,y = "); Serial.print(p.x);
     Serial.print(","); Serial.print(p.y);
     Serial.print("\tPressure = "); Serial.println(p.z);

     tft.setCursor(p.x, p.y);
     tft.print("x");
  }

  // if there's touchscreen input, handle it
  Point touch;
  if (newScreenTap(&touch)) {
    const int radius = 3;    // debug
    tft.fillCircle(touch.x, touch.y, radius, cVALUE);  // debug

    
  }

  // make a small progress bar crawl along bottom edge
  // this gives a sense of how frequently the main loop is executing
  showActivityBar(239, ILI9341_RED, ILI9341_BLACK);
}
