/*
  Griduino -- Grid Square Navigator with GPS

  Versions: 
  2019-12-20  v9.0 generates sound by synthesized sine wave intended for decent fidelity
                   from a small speaker. The hardware goal is for spoken-word output.
  2019-12-30  v9.2 adds Morse Code announcements via generated audio waveform on DAC.
  2020-01-01  v9.3 makes the Morse Code actually work, and replaces view_stat_screen
  2020-01-02  v9.4 adds a new view for controlling audio volume
              v9.8 adds saving settings in 2MB RAM
              v10.0 add altimeter
              v10.1 add GPS save/restore to visually power up in the same place as previous
              v0.12 refactors screen writing to class TextField
              v0.13 begin implementing our own TouchScreen functions
              v0.15 add simulated GPS track (class MockModel)
  2020-06-03  v0.16 add GMT Clock view
  2020-06-23  v0.17 add Settings control panel, and clear breadcrumb trail
  2020-07-06  v0.18 runtime selection of GPS receiver vs simulated trail
  2020-08-14  v0.20 add icons for gear, arrow
  2020-08-20  v0.21 fix audio volume set-save-restore bug
  2020-08-23  v0.22 add setting to show distance in miles/kilometers
  2020-09-02  v0.23 refactor views into base class "View" and derived classes

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This program runs a GPS display for your vehicle's dashboard to 
            show your position in your Maidenhead Grid Square, with distances 
            to nearby squares. This is optimized for ham radio rovers. 
            Read about the Maidenhead Locator System (grid squares) 
            at https://en.wikipedia.org/wiki/Maidenhead_Locator_System

            +---------------------------------------+
            |              CN88  30.1 mi            |
            |      +-------------------------+      |
            |      |                      *  |      |
            | CN77 |      CN87               | CN97 |
            | 61.2 |                         | 37.1 |
            |      |                         |      |
            |      +-------------------------+      |
            |               CN86  39.0 mi           |
            |            47.5644, -122.0378         |
            +---------------------------------------+

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)
            Spec: https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341
            Spec: http://adafru.it/1743
            How to: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2
            SPI Wiring: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/spi-wiring-and-test
            Touchscreen: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/resistive-touchscreen

         3. Adafruit Ultimate GPS
            Spec: https://www.adafruit.com/product/746

         4. One-transistor audio amplifier, digital potentiometer and mini speaker
            This is a commodity item and many similar devices are available.
            We tested a piezo speaker but they're tuned for a narrow frequency and 
            unsatisfactory for anything but a single pitch.
            Here is a breadboard-friendly speaker from Adafruit.
            Spec: https://www.adafruit.com/product/1898

  Source Code Outline:
         1. Hardware Wiring  (pin definitions)
         2. Helper Functions (touchscreen, fonts, grids, distance, etc)
         3. Model
         4. Views
         5. Controller
         6. setup()
         7. loop()
*/

#include "SPI.h"                    // Serial Peripheral Interface
#include "Adafruit_GFX.h"           // Core graphics display library
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "Adafruit_GPS.h"           // Ultimate GPS library
#include "TouchScreen.h"            // Touchscreen built in to 3.2" Adafruit TFT display
#include "Adafruit_BMP3XX.h"        // Precision barometric and temperature sensor
#include "constants.h"              // Griduino constants, colors, typedefs
#include "view.h"                   // Griduino screens
#include "DS1804.h"                 // DS1804 digital potentiometer library
#include "save_restore.h"           // save/restore configuration data to SDRAM
#include "icons.h"                  // bitmaps for icons

// ---------- Hardware Wiring ----------
/*                                Arduino       Adafruit
  ___Label__Description______________Mega_______Feather M4__________Resource____
TFT Power:
   GND  - Ground                  - ground      - J2 Pin 13
   VIN  - VCC                     - 5v          - Pin 10 J5 Vusb
TFT SPI: 
   SCK  - SPI Serial Clock        - Digital 52  - SCK (J2 Pin 6)  - uses hardw SPI
   MISO - SPI Master In Slave Out - Digital 50  - MI  (J2 Pin 4)  - uses hardw SPI
   MOSI - SPI Master Out Slave In - Digital 51  - MO  (J2 Pin 5)  - uses hardw SPI
   CS   - SPI Chip Select         - Digital 10  - D5  (Pin 3 J5)
   D/C  - SPI Data/Command        - Digital  9  - D12 (Pin 8 J5)
TFT MicroSD:
   CD   - SD Card Detection       - Digital  8  - D10 (Pin 6 J5)
   CCS  - SD Card Chip Select     - Digital  7  - D11 (Pin 7 J5)
TFT Backlight:
   BL   - Backlight               - Digital 12  - D4  (J2 Pin 1)  - uses hardw PWM
TFT Resistive touch:
   X+   - Touch Horizontal axis   - Digital  4  - A3  (Pin 4 J5)
   X-   - Touch Horizontal        - Analog  A3  - A4  (J2 Pin 8)  - uses analog A/D
   Y+   - Touch Vertical axis     - Analog  A2  - A5  (J2 Pin 7)  - uses analog A/D
   Y-   - Touch Vertical          - Digital  5  - D9  (Pin 5 J5)
TFT No connection:
   3.3  - 3.3v output             - n/c         - n/c
   RST  - Reset                   - n/c         - n/c
   IM0/3- Interface Control Pins  - n/c         - n/c
GPS:
   VIN  - VCC                     - 5v          - Vin
   GND  - Ground                  - ground      - ground
   >RX  - data into GPS           - TX1 pin 18  - TX  (J2 Pin 2)  - uses hardware UART
   <TX  - data out of GPS         - RX1 pin 19  - RX  (J2 Pin 3)  - uses hardware UART
Audio Out:
   DAC0 - audio signal            - n/a         - A0  (J2 Pin 12) - uses onboard digital-analog converter
Digital potentiometer:
   VINC - volume increment        - n/a         - D6
   VUD  - volume up/down          - n/a         - A2
   CS   - volume chip select      - n/a         - A1
On-board lights:
   LED  - red activity led        - Digital 13  - D13             - reserved for onboard LED
   NP   - NeoPixel                - n/a         - D8              - reserved for onboard NeoPixel
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
  #define BMP_CS  13    // BMP388 sensor, chip select

  #define SD_CD   10    // SD card detect pin - Feather
  #define SD_CCS  11    // SD card select pin - Feather

#elif defined(ARDUINO_AVR_MEGA2560)
  #define TFT_BL   6    // TFT backlight
  #define SD_CCS   7    // SD card select pin - Mega
  #define SD_CD    8    // SD card detect pin - Mega
  #define TFT_DC   9    // TFT display/command pin
  #define TFT_CS  10    // TFT chip select pin
  #define BMP_CS  13    // BMP388 sensor, chip select

#else
  #warning You need to define pins for your hardware

#endif

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Touch Screen
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// This sketch has only two touch areas to make it easy for operator to select
// "top half" and "bottom half" without looking. Touch target precision is not essential.
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
TouchScreen ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, 295);

// ---------- Barometric and Temperature Sensor
Adafruit_BMP3XX baro(BMP_CS); // hardware SPI

// ---------- Feather's onboard lights
#define RED_LED 13          // diagnostics RED LED
//efine PIN_LED 13          // already defined in Feather's board variant.h

// ---------- GPS ----------
/* "Ultimate GPS" pin wiring is connected to a dedicated hardware serial port
    available on an Arduino Mega, Arduino Feather and others.

    The GPS' LED indicates status:
        1-sec blink = searching for satellites
        15-sec blink = position fix found
*/

// Hardware serial port for GPS
Adafruit_GPS GPS(&Serial1);

// ------------ Audio output
#define DAC_PIN      DAC0     // onboard DAC0 == pin A0
#define PIN_SPEAKER  DAC0     // uses DAC

// Adafruit Feather M4 Express pin definitions
#define PIN_VCS      A1       // volume chip select
#define PIN_VINC      6       // volume increment
#define PIN_VUD      A2       // volume up/down

// ctor         DS1804( ChipSel pin, Incr pin,  U/D pin,  maxResistance (K) )
DS1804 volume = DS1804( PIN_VCS,     PIN_VINC,  PIN_VUD,  DS1804_TEN );
int gWiper = 15;              // initial digital potentiometer wiper position, 0..99
int gFrequency = 1100;        // initial Morse code sidetone pitch
int gWordsPerMinute = 18;     // initial Morse code sending speed

// ------------ definitions
const int howLongToWait = 6;  // max number of seconds at startup waiting for Serial port to console

// ---------- Morse Code ----------
#include "morse_dac.h"      // Morse Code using digital-audio converter DAC0
DACMorseSender dacMorse(DAC_PIN, gFrequency, gWordsPerMinute);

// "morse_dac.cpp"  replaced  "morse.h"
//#include "morse.h"         // Morse Code Library for Arduino with Non-Blocking Sending
//                           // https://github.com/markfickett/arduinomorse

// 2. Helper Functions
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
  //delay(10);   // no delay: code above completely handles debouncing without blocking the loop
  return result;
}

void showWhereTouched(Point touch) {
  #ifdef SHOW_TOUCH_TARGETS
    const int radius = 3;     // debug
    tft.fillCircle(touch.x, touch.y, radius, cWARN);  // debug - show dot
  #endif
}
// WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
// 2020-05-12 barry@k7bwh.com
// We need to replace TouchScreen::pressure() and implement TouchScreen::isTouching()

/*#define USE_ORIGINAL_TOUCHSCREEN_CODE*/
#ifdef USE_ORIGINAL_TOUCHSCREEN_CODE
// 2019-11-12 barry@k7bwh.com 
// "isTouching()" is defined in touch.h but is not implemented Adafruit's TouchScreen library
// Note - For Griduino, if this function takes longer than 8 msec it can cause erratic GPS readings
// Here's a function provided by https://forum.arduino.cc/index.php?topic=449719.0
bool TouchScreen::isTouching(void) {

  #define MEASUREMENTS    3
  uint16_t nTouchCount = 0, nTouch = 0;

  for (uint8_t nI = 0; nI < MEASUREMENTS; nI++) {
    nTouch = pressure();    // read current pressure level
    // Minimum and maximum pressure we consider true pressing
    if (nTouch > 100 && nTouch < 900) {
      nTouchCount++;
    }

    // pause between samples, but not after the last sample
    //if (nI < (MEASUREMENTS-1)) {
    //  delay(1);             // 2019-12-20 bwh: added for Feather M4 Express
    //}
  }
  // Clean the touchScreen settings after function is used
  // Because LCD may use the same pins
  pinMode(_xm, OUTPUT);     digitalWrite(_xm, LOW);
  pinMode(_yp, OUTPUT);     digitalWrite(_yp, HIGH);
  pinMode(_ym, OUTPUT);     digitalWrite(_ym, LOW);
  pinMode(_xp, OUTPUT);     digitalWrite(_xp, HIGH);

  bool ret = (nTouchCount >= MEASUREMENTS);
  return ret;
}
#else
// 2020-05-03 CraigV and barry@k7bwh.com
uint16_t myPressure(void) {
  pinMode(PIN_XP, OUTPUT);   digitalWrite(PIN_XP, LOW);   // Set X+ to ground
  pinMode(PIN_YM, OUTPUT);   digitalWrite(PIN_YM, HIGH);  // Set Y- to VCC

  digitalWrite(PIN_XM, LOW); pinMode(PIN_XM, INPUT);      // Hi-Z X-
  digitalWrite(PIN_YP, LOW); pinMode(PIN_YP, INPUT);      // Hi-Z Y+

  int z1 = analogRead(PIN_XM);
  int z2 = 1023-analogRead(PIN_YP);

  return (uint16_t) ((z1+z2)/2);
}

// "isTouching()" is defined in touch.h but is not implemented Adafruit's TouchScreen library
// Note - For Griduino, if this function takes longer than 8 msec it can cause erratic GPS readings
// so we recommend against using https://forum.arduino.cc/index.php?topic=449719.0
bool TouchScreen::isTouching(void) {
  #define TOUCHPRESSURE 200       // Minimum pressure we consider true pressing
  static bool button_state = false;
  uint16_t pres_val = ::myPressure();

  if ((button_state == false) && (pres_val > TOUCHPRESSURE)) {
    Serial.print(". pressed, pressure = "); Serial.println(pres_val);     // debug
    button_state = true;
  }

  if ((button_state == true) && (pres_val < TOUCHPRESSURE)) {
    //Serial.print(". released, pressure = "); Serial.println(pres_val);       // debug
    button_state = false;
  }

  // Clean the touchScreen settings after function is used
  // Because LCD may use the same pins
  // todo - is this actually necessary?
  pinMode(PIN_XM, OUTPUT);     digitalWrite(PIN_XM, LOW);
  pinMode(PIN_YP, OUTPUT);     digitalWrite(PIN_YP, HIGH);
  pinMode(PIN_YM, OUTPUT);     digitalWrite(PIN_YM, LOW);
  pinMode(PIN_XP, OUTPUT);     digitalWrite(PIN_XP, HIGH);

  return button_state;
}
#endif
// MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM

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

void drawAllIcons() {
  // draw gear (settings) and arrow (next screen)
  //             ul x,y                     w,h   color
  tft.drawBitmap(   5,5, iconGear20,       20,20, cTEXTFAINT);  // "settings" upper left
  tft.drawBitmap( 300,5, iconRightArrow18, 14,18, cTEXTFAINT);  // "next screen" upper right
}

void showScreenBorder() {
  // optionally outline the display hardware visible border
  #ifdef SHOW_SCREEN_BORDER
    tft.drawRect(0, 0, gScreenWidth, gScreenHeight, ILI9341_BLUE);  // debug: border around screen
  #endif
}

// ========== font management helpers ==========================
/* Using fonts: https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts

  "Fonts" folder is inside \Documents\User\Arduino\libraries\Adafruit_GFX_Library\fonts
*/
#include "Fonts/FreeSans18pt7b.h"       // eFONTGIANT    36 pt (see constants.h)
#include "Fonts/FreeSansBold24pt7b.h"   // eFONTBIG      24 pt
#include "Fonts/FreeSans12pt7b.h"       // eFONTSMALL    12 pt
#include "Fonts/FreeSans9pt7b.h"        // eFONTSMALLEST  9 pt
// (built-in)                           // eFONTSYSTEM    8 pt

void setFontSize(int font) {
  // input: "font" = point size
  switch (font) {
    case 36:  // eFONTGIANT
      tft.setFont(&FreeSans18pt7b);
      tft.setTextSize(2);
      break;

    case 24:  // eFONTBIG
      tft.setFont(&FreeSansBold24pt7b);
      tft.setTextSize(1);
      break;

    case 12:  // eFONTSMALL
      tft.setFont(&FreeSans12pt7b);
      tft.setTextSize(1);
      break;

    case 9:   // eFONTSMALLEST
      tft.setFont(&FreeSans9pt7b);
      tft.setTextSize(1);
      break;

    case 0:   // eFONTSYSTEM
      tft.setFont();
      tft.setTextSize(2);
      break;

    default:
      Serial.print("Error, unknown font size ("); Serial.print(font); Serial.println(")");
      break;
  }
}

int getOffsetToCenterText(String text) {
  // measure width of given text in current font and 
  // calculate X-offset to make it centered left-right on screen
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);  // compute "pixels wide"
  return (gScreenWidth - w) / 2;
}

int getOffsetToCenterTextOnButton(String text, int leftEdge, int width ) {
  // measure width of given text in current font and 
  // calculate X-offset to make it centered left-right within given bounds
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);  // compute "pixels wide"
  return leftEdge + (width - w) / 2;
}

void showNameOfView(String sName, uint16_t fgd, uint16_t bkg) {
  // All our various "view" routines want to label themselves in the upper left corner
  setFontSize(0);
  tft.setTextColor(fgd, bkg);
  tft.setCursor(1,1);
  tft.print(sName);
}

void clearScreen() {
  tft.fillScreen(ILI9341_BLACK);  // (cBACKGROUND);
}

// ============== grid helpers =================================

// given a position, find the longitude of the next grid line
// this is always an even integer number since grids are 2-degrees wide
float nextGridLineEast(float longitudeDegrees) {
  return ceil(longitudeDegrees / 2) * 2;
}
float nextGridLineWest(float longitudeDegrees) {
  return floor(longitudeDegrees / 2) * 2;
}
float nextGridLineSouth(float latitudeDegrees) {
  return floor(latitudeDegrees);
}
void calcLocator(char* result, double lat, double lon, int precision) {
  // Converts from lat/long to Maidenhead Grid Locator
  // From: https://ham.stackexchange.com/questions/221/how-can-one-convert-from-lat-long-to-grid-square
  // Input: char result[7];
  int o1, o2, o3;
  int a1, a2, a3;
  double remainder;

  // longitude
  remainder = lon + 180.0;
  o1 = (int)(remainder / 20.0);
  remainder = remainder - (double)o1 * 20.0;
  o2 = (int)(remainder / 2.0);
  remainder = remainder - 2.0 * (double)o2;
  o3 = (int)(12.0 * remainder);

  // latitude
  remainder = lat + 90.0;
  a1 = (int)(remainder / 10.0);
  remainder = remainder - (double)a1 * 10.0;
  a2 = (int)(remainder);
  remainder = remainder - (double)a2;
  a3 = (int)(24.0 * remainder);

  result[0] = (char)o1 + 'A';
  result[1] = (char)a1 + 'A';
  result[2] = (char)o2 + '0';
  result[3] = (char)a2 + '0';
  result[4] = (char)0;
  if (precision > 4) {
    result[4] = (char)o3 + 'a';
    result[5] = (char)a3 + 'a';
    result[6] = (char)0;
  }
  return;
}

void floatToCharArray(char* result, int maxlen, double fValue, int decimalPlaces) {
  String temp = String(fValue, decimalPlaces);
  temp.toCharArray(result, maxlen);
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong*1000;
  int x = 0;
  int y = 0;
  int w = gScreenWidth;
  int h = gScreenHeight;
  clearScreen();
  
  while (millis() < targetTime) {
    if (Serial) break;
    x += 2;
    y += 2;
    w -= 4;
    h -= 4;
    if (x > gScreenWidth) {
      x = y = 0;
      w = gScreenWidth;
      h = gScreenHeight;
      clearScreen();          // erase entire screen
    }
    tft.drawRect(x, y, w, h, cLABEL);   // look busy
    delay(15);
  }
}

//==============================================================
//
//      Model
//      This is MVC (model-view-controller) design pattern
//
//    This model collects data from the GPS sensor
//    on a schedule determined by the Controller.
//
//    The model knows about:
//    - current grid info, such as lat/long and grid square name
//    - nearby grids and distances
//    - when we enter a new grid
//    - when we lose GPS signal
//
//    We use "class" instead of the usual collection of random subroutines 
//    to help guide the programmer into designing an independent Model object 
//    with very specific functionality and interfaces.
//==============================================================
#include "model.cpp"

// create an instance of the model
Model modelGPS;             // normal: use real GPS hardware
MockModel modelSimulator;   // test: simulated travel (see model.cpp)

// at power-on, we choose to always start with real GPS receiver hardware 
// because I don't want to bother saving/restoring this selection right now
Model* model = &modelGPS;

void fSetReceiver() {
  model = &modelGPS;        // use "class Model" for GPS receiver hardware
}
void fSetSimulated() {
  model = &modelSimulator;  // use "class MockModel" for simulated track
}
int fGetDataSource() {
  // this function allows the user interface to display which one is active
  // returns: enum
  if (model == &modelGPS) {
    return eGPSRECEIVER;
  } else {
    return eGPSSIMULATOR;
  }
}

//==============================================================
//
//      Views
//      This is MVC (model-view-controller) design pattern
//
//    "startXxxxScreen" is one-time setup of visual elements that never change
//    "updateXxxxScreen" is dynamic and displays things that change over time
//==============================================================

// alias names for the views - must be in same alphabetical order as array below
enum {
  GRID_VIEW = 0,
  HELP_VIEW,
  SETTING2_VIEW,
  SETTING3_VIEW,
  SPLASH_VIEW,
  STATUS_VIEW,
  TIME_VIEW,
  VOLUME_VIEW,
  //VOLUME2_VIEW,
  GOTO_SETTINGS,          // command the state machine to show control panel
  GOTO_NEXT_VIEW,         // command the state machine to show next screen
};
// list of objects derived from "class View", in alphabetical order:
//      Grid, Help, Settings2, Settings3, Splash, Status, Time, Volume
View* pView;                           // pointer to a derived class

ViewGrid   gridView(&tft, GRID_VIEW);             // instantiate derived classes
ViewHelp   helpView(&tft, HELP_VIEW);
ViewSettings2 settings2View(&tft, SETTING2_VIEW);
ViewSettings3 settings3View(&tft, SETTING3_VIEW);
ViewSplash splashView(&tft, SPLASH_VIEW);
ViewStatus statusView(&tft, STATUS_VIEW);
ViewTime   timeView(&tft, TIME_VIEW);
ViewVolume volumeView(&tft, VOLUME_VIEW);

void selectNewView(int cmd) {
  // this is a state machine to select next view, given current view and type of command
  View* viewTable[] = {
        &gridView,         // [GRID_VIEW]
        &helpView,         // [HELP_VIEW]
        &settings2View,    // [SETTING2_VIEW]
        &settings3View,    // [SETTING3_VIEW]
        &splashView,       // [SPLASH_VIEW]
        &statusView,       // [STATUS_VIEW]
        &timeView,         // [TIME_VIEW]
        &volumeView,       // [VOLUME_VIEW]
  };

  int currentView = pView->screenID;
  int nextView = GRID_VIEW;       // default
  if (cmd == GOTO_NEXT_VIEW) {
    // operator requested the next NORMAL user view
    switch (currentView) {
      case GRID_VIEW:   nextView = STATUS_VIEW; break;
      case STATUS_VIEW: nextView = TIME_VIEW; break;
      case TIME_VIEW:   nextView = GRID_VIEW; break;
      // none of above: we must be showing some settings view, so go to the first normal user view
      default:          nextView = GRID_VIEW; break;
    }
  } else {
    // operator requested the next SETTINGS view
    switch (currentView) {
      case VOLUME_VIEW:  nextView = SETTING2_VIEW; break;
      //se VOLUME2_VIEW:  nextView = SETTING2_VIEW; break;
      case SETTING2_VIEW: nextView = SETTING3_VIEW; break;
      case SETTING3_VIEW: nextView = VOLUME_VIEW; break;
      // none of above: we must be showing some normal user view, so go to the first settings view
      default:           nextView = VOLUME_VIEW; break;
    }
  }
  Serial.print("selectNewView() from "); Serial.print(currentView);
  Serial.print(" to "); Serial.println(nextView);

  pView = viewTable[ nextView ];

  // Every view has an initial setup to prepare its layout
  // After initial setup the view can assume it "owns" the screen
  // and can safely repaint only the parts that change
  pView->startScreen();
  pView->updateScreen();
}

//==============================================================
//
//      Controller
//      This is MVC (model-view-controller) design pattern
//
//==============================================================
// ----- adjust screen brightness
const int gNumLevels = 3;
const int gaBrightness[gNumLevels] = { 255, 80, 20 }; // global array of preselected brightness
int gCurrentBrightnessIndex = 0;    // current brightness
void adjustBrightness() {
  // increment display brightness
  gCurrentBrightnessIndex = (gCurrentBrightnessIndex + 1) % gNumLevels;
  int brightness = gaBrightness[gCurrentBrightnessIndex];
  analogWrite(TFT_BL, brightness);
}

void sendMorseLostSignal() {
  // commented out -- this occurs too frequently and is distracting
  return;

  String msg(PROSIGN_AS);   // "wait" symbol
  dacMorse.setMessage(msg);
  dacMorse.sendBlocking();  // TODO - use non-blocking
}
void sendMorseGrid4(String gridName) {
  // announce new grid by Morse code
  String grid4 = gridName.substring(0, 4);
  grid4.toLowerCase();

  dacMorse.setMessage( grid4 );
  dacMorse.sendBlocking();

  //spkrMorse.setMessage( grid4 );
  //spkrMorse.startSending();   // would prefer non-blocking but some bug causes random dashes to be too long
  //spkrMorse.sendBlocking();
}

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  static int addDotX = 10;                    // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count = 0;
  const int SCALEF = 32;                      // how much to slow it down so it becomes visible

  count = (count + 1) % SCALEF;
  if (count == 0) {
    addDotX = (addDotX + 1) % gScreenWidth;   // advance
    rmvDotX = (rmvDotX + 1) % gScreenWidth;   // advance
    tft.drawPixel(addDotX, row, foreground);  // write new
    tft.drawPixel(rmvDotX, row, background);  // erase old
  }
}

//=========== setup ============================================
void setup() {

  // ----- init TFT display
  tft.begin();                                  // initialize TFT display
  tft.setRotation(SCREEN_ROTATION);             // 1=landscape (default is 0=portrait)
  clearScreen();

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init touch screen
  ts.pressureThreshhold = 200;

  // ----- init serial monitor
  Serial.begin(115200);                               // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);                       // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  #if defined(SAMD_SERIES)
    Serial.println("Compiled for Adafruit Feather M4 Express (or equivalent)");
  #else
    Serial.println("Sorry, your hardware platform is not recognized.");
  #endif

  // ----- init GPS
  GPS.begin(9600);                              // 9600 NMEA is the default baud rate for Adafruit MTK GPS's
  delay(200);                                   // is delay really needed?
  GPS.sendCommand(PMTK_SET_BAUD_57600);         // set baud rate to 57600
  delay(200);
  GPS.begin(57600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); // turn on RMC (recommended minimum) and GGA (fix data) including altitude

  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    // 1 Hz update rate
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ); // Once every 5 seconds update
  //GPS.sendCommand(PGCMD_ANTENNA);             // Request updates on whether antenna is connected or not (comment out to keep quiet)

  // ----- query GPS
  Serial.print("Sending command to query GPS Firmware version");
  Serial.println(PMTK_Q_RELEASE);     // Echo query to console
  GPS.sendCommand(PMTK_Q_RELEASE);    // Send query to GPS unit
                                      // expected reply: $PMTK705,AXN_2.10...
  //GPS.sendCommand(PGCMD_ANTENNA);   // Request antenna status (comment out to keep quiet)
                                      // expected reply: $PGTOP,11,...

  // ----- init digital potentiometer
  volume.unlock();                    // unlock digipot (in case someone else, like an example pgm, has locked it)
  volume.setToZero();                 // set digipot hardware to match its ctor (wiper=0) because the chip cannot be read
                                      // and all "setWiper" commands are really incr/decr pulses. This gets it sync.
  volume.setWiperPosition( gWiper );  // set default volume in digital pot

  volumeView.loadConfig();            // restore volume setting from non-volatile RAM

  // ----- init DAC for audio/morse code
  #if defined(SAMD_SERIES)
    // Only set DAC resolution on devices that have a DAC
    analogWriteResolution(12);        // 1..32, sets DAC output resolution to 12 bit (4096 levels)
                                      // because Feather M4 maximum output resolution is 12 bit
  #endif
  dacMorse.setup();                   // required Morse Code initialization
  dacMorse.dump();                    // debug

  // ----- init onboard LED
  pinMode(RED_LED, OUTPUT);           // diagnostics RED LED
  
  // ----- report on our memory hog
  char temp[200];
  Serial.println("Large resources:");
  snprintf(temp, sizeof(temp), 
          "  Model.history[%d] uses %d bytes/entry = %d bytes total size",
             model->numHistory, sizeof(Location), sizeof(model->history));
  Serial.println(temp);

  // ----- run unit tests, if allowed by "#define RUN_UNIT_TESTS"
  #ifdef RUN_UNIT_TESTS
  void runUnitTest();                 // extern declaration
  runUnitTest();                      // see "unit_test.cpp"
  #endif

  // ----- init first data shown with last known position and driving track history
  model->restore();
  model->gHaveGPSfix = false;          // assume no satellite signal yet
  model->gSatellites = 0;

  // one-time Splash screen
  pView = &splashView;
  pView->startScreen();
  pView->updateScreen();
  delay(2000);

  // one-time Help screen
  pView = &helpView;
  pView->startScreen();
  delay(2000);

  // ----- select opening view screen
  pView = &gridView;
  pView->startScreen();                 // start current view
  pView->updateScreen();                // update current view
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTimeGPS = millis();
//uint32_t prevTimeMorse = millis();

const int GPS_PROCESS_INTERVAL = 1000;  // milliseconds between updating the model's GPS data

void loop() {

  // if our timer or system millis() wrapped around, reset it
  if (prevTimeGPS > millis()) {
    prevTimeGPS = millis();
  }
  //if (prevTimeMorse > millis()) {
  //  prevTimeMorse = millis();
  //}

  GPS.read();   // if you can, read the GPS serial port every millisecond in an interrupt

  if (GPS.newNMEAreceived()) {
    // sentence received -- verify checksum, parse it
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and thereby might miss other sentences
    // so be very wary if using OUTPUT_ALLDATA, which generates loads of sentences,
    // since trying to print them all out is time-consuming
    // GPS parsing: https://learn.adafruit.com/adafruit-ultimate-gps/parsed-data-output
    if (!GPS.parse(GPS.lastNMEA())) {
      // parsing failed -- restart main loop to wait for another sentence
      // this also sets the newNMEAreceived() flag to false
      return;
    } else {
      #ifdef ECHO_GPS_SENTENCE
      Serial.print(GPS.lastNMEA());   // debug
      #endif
    }
  }

  // periodically, ask the model to process and save the current GPS location
  if (millis() - prevTimeGPS > GPS_PROCESS_INTERVAL) {
    prevTimeGPS = millis();           // restart another interval

    model->processGPS();               // update model

    // update View - call the current viewing function
    pView->updateScreen();             // update current view, eg, updateGridScreen()
    //gaUpdateView[gViewIndex](); 
  }

  //if (!spkrMorse.continueSending()) {
  //  // give processing time to SpeakerMorseSender component
  //  // "continueSending" returns false after the message finishes sending
  //}

  // if there's an alert, tell the user
  if (model->signalLost()) {
    model->indicateSignalLost();      // update model
    sendMorseLostSignal();            // announce GPS signal lost by Morse code
  }

  if (model->enteredNewGrid()) {
    pView->updateScreen();            // update display so they can see new grid while listening to audible announcement
    char newGrid4[5];
    calcLocator(newGrid4, model->gLatitude, model->gLongitude, 4);
    sendMorseGrid4( newGrid4 );       // announce new grid by Morse code
  }

  // if there's touchscreen input, handle it
  Point touch;
  if (newScreenTap(&touch)) {

    #ifdef SHOW_TOUCH_TARGETS
      showWhereTouched(touch);        // debug: show where touched
    #endif

    bool touchHandled = pView->onTouch(touch);

    if (!touchHandled) {
      // not handled by one of the views, so run our default action
      if (touch.y < gScreenHeight * 1 / 4) {
        if (touch.x < gScreenWidth / 3) {
          selectNewView(GOTO_SETTINGS);   // advance to next settings view
        } else {
          selectNewView(GOTO_NEXT_VIEW);  // advance to next normal user view
        }
      } else if (touch.y > gScreenHeight *3 / 4) {
        adjustBrightness();               // change brightness
      }
    }
  }

  // small activity bar crawls along bottom edge to give 
  // a sense of how frequently the main loop is executing
  showActivityBar(tft.height()-1, ILI9341_RED, cBACKGROUND);
}
