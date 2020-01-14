/*
  Griduino -- Grid Square Navigator with GPS

  Date:     2019-12-20 created v9
            2019-12-30 created v9.2
            2020-01-01 created v9.3
            2020-01-02 created v9.4

  Changelog: v9 generates sound by synthesized sine wave intended
            for decent fidelity from a small speaker. The hardware goal is to 
            have an audio chain sufficient for possible future spoken-word output.
            All files are renamed to use underscore (_) instead of hyphen (-)
            for compatibility on Mac and Linux.

            This version 9 is converging on our final hardware, and is 
            written while Barry powers up a new soldered breadboard, new
            Feather processor, new audio amp, and refining the software.

            v9.2 adds Morse Code announcements via generated audio waveform on DAC.
            v9.3 makes the Morse Code actually work, and replaces view_stat_screen
            v9.4 adds a new view for controlling audio volume

  Software: Barry Hansen, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This program runs a GPS display for your vehicle's dashboard to 
            show your position in your Maidenhead Grid Square, with distances 
            to nearby squares. This is intended for ham radio rovers. 
            Read about Maidenhead Locator System (grid squares) 
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
         1. Arduino Mega 2560 Rev3 (16 MHz ATmega2560)
            Spec: https://www.adafruit.com/product/191
        -OR- 
            Arduino Feather M4 Express (120 MHz SAMD51)
            Spec: https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341
            Spec: http://adafru.it/1743
            How to: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2
            SPI Wiring: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/spi-wiring-and-test
            Touchscreen: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/resistive-touchscreen

         3. Adafruit Ultimate GPS
            Spec: http://www.adafruit.com/product/746

         4. One-transistor audio amplifier, digital potentiometer and mini speaker
            This is a commodity item and many similar devices are available.
            We also tried a piezo speaker but they're tuned for a narrow frequency.
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

#include "SPI.h"                  // Serial Peripheral Interface
#include "Adafruit_GFX.h"         // Core graphics display library
#include "Adafruit_ILI9341.h"     // TFT color display library
#include "Adafruit_GPS.h"         // Ultimate GPS library
#include "TouchScreen.h"          // Touchscreen built in to 3.2" Adafruit TFT display
#include "constants.h"            // Griduino constant declarations
#include "DS1804.h"               // DS1804 digital potentiometer library

// ------- Select features ---------
//efine RUN_UNIT_TESTS            // comment out to save boot-up time

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
   CS   - volume chip select       - n/a        - A1
On-board lights:
   LED  - red activity led        - Digital 13  - D13             - reserved for onboard LED
   NP   - NeoPixel                - n/a         - D6              - reserved for onboard NeoPixel !! no, NeoPixel is ONLY using pin 8 !!
   NP   - NeoPixel                - n/a         - D8              - reserved for onboard NeoPixel
*/

// TFT display and SD card share the hardware SPI interface, and have
// separate 'select' pins to identify the active device on the bus.
#if defined(ARDUINO_AVR_MEGA2560)
  #define TFT_BL   6    // TFT backlight
  #define SD_CCS   7    // SD card select pin - Mega
  #define SD_CD    8    // SD card detect pin - Mega
  #define TFT_DC   9    // TFT display/command pin
  #define TFT_CS  10    // TFT select pin

#elif defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  // To compile for Feather M0/M4, install "additional boards manager"
  // https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup
  
  #define TFT_BL   4    // TFT backlight
  #define TFT_CS   5    // TFT select pin
  #define TFT_DC  12    // TFT display/command pin

  #define SD_CD   10    // SD card detect pin - Feather
  #define SD_CCS  11    // SD card select pin - Feather

#else
  // todo: Unknown platform
  #warning You need to define pins for your hardware

#endif

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Touch Screen
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// This sketch has only two touch areas to make it easy for operator to select
// "top half" and "bottom half" without looking. Exact precision is not essential.
#if defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  #define PIN_XP  A3    // Touchscreen X+ can be a digital pin
  #define PIN_XM  A4    // Touchscreen X- must be an analog pin, use "An" notation
  #define PIN_YP  A5    // Touchscreen Y+ must be an analog pin, use "An" notation
  #define PIN_YM   9    // Touchscreen Y- can be a digital pin
#else
  // Arduino Mega 2560 and others
  #define PIN_XP   4    // Touchscreen X+ can be a digital pin
  #define PIN_XM  A3    // Touchscreen X- must be an analog pin, use "An" notation
  #define PIN_YP  A2    // Touchscreen Y+ must be an analog pin, use "An" notation
  #define PIN_YM   5    // Touchscreen Y- can be a digital pin
#endif
TouchScreen ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, 295);

// ---------- Onboard LED
#define RED_LED 13    // diagnostics RED LED

// ---------- GPS ----------
/* "Ultimate GPS" pin wiring uses its own dedicated hardware serial port
    available on an Arduino Mega, Arduino Feather and others.

    The GPS' LED indicates status:
        1-sec blink = searching for satellites
        10-sec blink = position fix found
*/

// Hardware serial port for GPS
Adafruit_GPS GPS(&Serial1);

// ------------ Audio output
#define DAC_PIN      DAC0     // onboard DAC0 == A0
#define PIN_SPEAKER  DAC0     // uses DAC

// Adafruit Feather M4 Express pin definitions
#define PIN_VCS      A1       // volume chip select
#define PIN_VINC      6       // volume increment
#define PIN_VUD      A2       // volume up/down

// ctor         DS1804( ChipSel pin,Incr pin,  U/D pin,  maxResistance (K) )
DS1804 volume = DS1804( PIN_VCS,    PIN_VINC,  PIN_VUD,  DS1804_TEN );
int gVolume = 15;             // initial digital potentiometer wiper position, 0..99
int gFrequency = 1100;        // initial Morse code sidetone pitch
int gWordsPerMinute = 18;     // initial Morse code sending speed

// ------------ typedef's
typedef struct {
  int x;
  int y;
} Point;

// ------------ definitions
const int gNumViews = 5;      // total number of different views (screens) we've implemented
int gViewIndex = 0;           // selects which view to show
                              // init to a safe value, override in setup()
const int howLongToWait = 8;  // max number of seconds at startup waiting for Serial port to console

// ------------ global scope
int gTextSize;                          // no such function as "tft.getTextSize()" so remember it on our own
int gUnitFontWidth, gUnitFontHeight;    // character cell size for TextSize(1)
int gCharWidth, gCharHeight;            // character cell size for TextSize(n)

// ---------- Morse Code ----------
#include "morse_dac.h"
DACMorseSender dacMorse(DAC_PIN, gFrequency, gWordsPerMinute);

// "morse_dac.cpp"  replaced  "morse.h"
//#include "morse.h"         // Morse Code Library for Arduino with Non-Blocking Sending
//                           // https://github.com/markfickett/arduinomorse
//SpeakerMorseSender spkrMorse(
//  A0,             // PIN_SPEAKER
//  2000,           // tone frequency
//  0,              // carrier frequency
//  18);            // wpm

// ========== extern ===========================================
void updateGridScreen();    void startGridScreen();   bool onTouchGrid(Point touch);    // view_grid.cpp
void updateStatusScreen();  void startStatScreen();   bool onTouchStatus(Point touch);  // view_status.cpp
void updateVolumeScreen();  void startVolumeScreen(); bool onTouchVolume(Point touch);  // view_volume.cpp
void updateSplashScreen();  void startSplashScreen(); bool onTouchSplash(Point touch);  // view_splash.cpp
void updateHelpScreen();    void startHelpScreen();   bool onTouchHelp(Point touch);    // view_help.cpp

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
  //delay(100);   // no delay: code above completely handles debouncing without blocking the loop
  return result;
}

// Software reset
void SoftwareReset(void)
{
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

  screen->x = 0;
  screen->y = 0;

  // setRotation(1) = landscape orientation = x-,y-axis exchanged
  screen->x = map(touch.y, 100, 900,  0, tft.width());
  screen->y = map(touch.x, 900, 100,  0, tft.height());
  return;
}

// ========== font management routines =========================
/* Using fonts: https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts

  "Fonts" folder inside Adafruit_GFX, and here are some of the "Sans" fonts I tried.
                                  sugg.
  ---font name---             ---gTextSize---  ---unit size---
  (default)                     gTextSize=5      6 x  8

  FreeSans9pt7b.h
  FreeSansBold9pt7b.h
  FreeSansOblique9pt7b.h
  FreeSansBoldOblique9pt7b.h    gTextSize=4     12 x 16

  FreeSans12pt7b.h              gTextSize=3     15 x 20 \
  FreeSansBold12pt7b.h          gTextSize=3     15 x 20  |
  FreeSansOblique12pt7b.h       gTextSize=3     15 x 20  |
  FreeSansBoldOblique12pt7b.h   gTextSize=3     16 x 20 /

  FreeSans18pt7b.h
  FreeSansBold18pt7b.h          gTextSize=2     22 x 30
  FreeSansOblique18pt7b.h
  FreeSansBoldOblique18pt7b.h   gTextSize=2     22 x 30

  FreeSans24pt7b.h              gTextSize=2     30 x 38 \
  FreeSansBold24pt7b.h          gTextSize=1     30 x 38  |  this font for big bold items
  FreeSansOblique24pt7b.h       gTextSize=2     30 x 38  |
  FreeSansBoldOblique24pt7b.h   gTextSize=1     30 x 38 /
*/
#include "Fonts/FreeSansBold24pt7b.h"   // comment out to save 10KB program space
void initFontSizeBig() {
  tft.setFont(&FreeSansBold24pt7b);
  gTextSize = 1;
  tft.setTextSize(gTextSize);

  gUnitFontWidth = 32;
  gUnitFontHeight = 38;
  gCharWidth = gUnitFontWidth * gTextSize;
  gCharHeight = gUnitFontHeight * gTextSize;
}
#include "Fonts/FreeSans12pt7b.h"
void initFontSizeSmall() {
  tft.setFont(&FreeSans12pt7b);
  gTextSize = 1;
  tft.setTextSize(gTextSize);

  gUnitFontWidth = 16;
  gUnitFontHeight = 20;
  gCharWidth = gUnitFontWidth * gTextSize;
  gCharHeight = gUnitFontHeight * gTextSize;
}
void initFontSizeSystemSmall() {
  tft.setFont();
  gTextSize = 2;
  tft.setTextSize(gTextSize);

  // default font has character cell of 6px by 8px for TextSize(1) including margin.
  // Other fonts are multiples of this, e.g. TextSize(2) = 12px by 16px
  gUnitFontWidth = 6;
  gUnitFontHeight = 8;
  gCharWidth = gUnitFontWidth * gTextSize;
  gCharHeight = gUnitFontHeight * gTextSize;
}
int getOffsetToCenterText(String text) {
  // measure width of given text in current font and 
  // calculate X-offset to make it centered left-right on screen
  
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);  // compute "pixels wide"
  return (gScreenWidth - w) / 2;
}
void drawProportionalText(int ulX, int ulY, String prevText, String newText, bool dirty) {
  // input: (x,y) of upper left corner
  //        prevText = old text string to erase
  //        newText  = new text string to write
  //        dirty    = force refresh screen, even if value has not changed

  if (!dirty && prevText == newText) {
    // if the text is unchanged from previous write
    // then don't do anything -  this avoids blinking during erase-then-write operation
    return;
  }

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(prevText, ulX, ulY, &x1, &y1, &w, &h);
  tft.fillRect(x1, y1, w, h, ILI9341_BLACK);  // erase the old text

  // show new grid name, e.g. "CN87"
  tft.setCursor(ulX, ulY);
  tft.setTextSize(gTextSize);
  tft.print(newText);

  // debug: draw rectangle around new text
  // If everything works perfectly, this rectangle will be erased via our next call
  //tft.getTextBounds(newText, ulX, ulY, &x1, &y1, &w, &h);
  //tft.drawRect(x1, y1, w, h, ILI9341_RED);
}
void showNameOfView(String sName, uint16_t fgd, uint16_t bkg) {
  // All our various "view" routines want to label themselves in the upper left corner
  initFontSizeSystemSmall();
  tft.setTextColor(fgd, bkg);
  tft.setCursor(1,1);
  tft.print(sName);
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
#ifdef RUN_UNIT_TESTS
void testNextGridLineEast(float fExpected, double fLongitude) {
  // unit test helper for finding grid line crossings
  float result = nextGridLineEast(fLongitude);
  Serial.print("Grid Crossing East: given = "); Serial.print(fLongitude);
  Serial.print(", expected = "); Serial.print(fExpected);
  Serial.print(", result = "); Serial.print(result);
  if (result == fExpected) {
    Serial.println("");
  }
  else {
    Serial.println(" <-- Unequal");
  }
}
void testNextGridLineWest(float fExpected, double fLongitude) {
  // unit test helper for finding grid line crossings
  float result = nextGridLineWest(fLongitude);
  Serial.print("Grid Crossing West: given = "); Serial.print(fLongitude);
  Serial.print(", expected = "); Serial.print(fExpected);
  Serial.print(", result = "); Serial.print(result);
  if (result == fExpected) {
    Serial.println("");
  }
  else {
    Serial.println(" <-- Unequal");
  }
}
void testCalcLocator(String sExpected, double lat, double lon) {
  // unit test helper function to display results
  String sResult;
  sResult = calcLocator(lat, lon);
  Serial.print("Test: expected = "); Serial.print(sExpected);
  Serial.print(", gResult = "); Serial.print(sResult);
  if (sResult == sExpected) {
    Serial.println("");
  }
  else {
    Serial.println(" <-- Unequal");
  }
}
#endif // RUN_UNIT_TESTS
String calcLocator(double lat, double lon) {
  // Converts from lat/long to Maidenhead Grid Locator
  // From: https://ham.stackexchange.com/questions/221/how-can-one-convert-from-lat-long-to-grid-square
  char result[7];
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
  result[4] = (char)o3 + 'a';
  result[5] = (char)a3 + 'a';
  result[6] = (char)0;
  return (String)result;
}
// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for all this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong*1000;
  while (millis() < targetTime) {
    if (Serial) break;
  }
}

//=========== distance helpers =================================

String calcDistanceLat(double fromLat, double toLat) {
  // calculate distance in N-S direction (miles)
  // returns 4-character String
  String sDistance("12.3");   // result

  const double R = 3958.8;             // average Earth radius (miles)
  const double degreesPerRadian = 57.2957795;
  double angleDegrees = fabs(fromLat - toLat);
  double angleRadians = angleDegrees / degreesPerRadian;
  double distance = angleRadians * R;
  sDistance = String(distance, 1);
  return sDistance;
}
String calcDistanceLong(double lat, double fromLong, double toLong) {
  // calculate distance in E-W direction (degrees)
  // returns 4-character String (miles)
  String sDistance("123.4");   // result

  const double R = 3958.8;             // average Earth radius (miles)
  const double degreesPerRadian = 57.2957795; // conversion factor
  double scaleFactor = fabs(cos(lat / degreesPerRadian)); // grids are narrower as you move from equator to north/south pole
  double angleDegrees = fabs(fromLong - toLong);
  double angleRadians = angleDegrees / degreesPerRadian * scaleFactor;
  double distance = angleRadians * R;
  if (distance > 100.0) {
    sDistance = String(distance, 0) + " ";  // make strlen=4
  } else if (distance > 10.0) {
    sDistance = String(distance, 1);
  } else {
    sDistance = String(distance, 2);
  }
  return sDistance;
}
#ifdef RUN_UNIT_TESTS
void testDistanceLat(String sExpected, double fromLat, double toLat) {
  // unit test helper function to calculate N-S distances
  String sResult = calcDistanceLat(fromLat, toLat);
  Serial.print("N-S Distance Test: expected = "); Serial.print(sExpected);
  Serial.print(", result = "); Serial.println(sResult);
}
void testDistanceLong(String sExpected, double lat, double fromLong, double toLong) {
  // unit test helper function to calculate E-W distances
  String sResult = calcDistanceLong(lat, fromLong, toLong);
  Serial.print("E-W Distance Test: expected = "); Serial.print(sExpected);
  Serial.print(", result = "); Serial.print(sResult);
  if (sResult == sExpected) {
    Serial.println("");
  }
  else {
    Serial.println(" <-- Unequal");
  }
}
#endif // RUN_UNIT_TESTS

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
Model model;

//==============================================================
//
//      Views
//      This is MVC (model-view-controller) design pattern
//
//    "startXxxxScreen" is one-time setup of visual elements that never change
//    "updateXxxxScreen" is dynamic and displays things that change over time
//==============================================================

//#include "view_splash.cpp"
//#include "view_help.cpp"
//#include "view_grid.cpp"
//#include "view_status.cpp"
//#include "view_volume.cpp"

// ========== unit tests ========================
void runUnitTest() {
#ifdef RUN_UNIT_TESTS
  tft.fillScreen(ILI9341_BLACK);

  // ----- announce ourselves
  initFontSizeBig();
  drawProportionalText(gCharWidth, gCharHeight, String(""), String("--Unit Test--"), true);

  initFontSizeSystemSmall();
  tft.setCursor(0, tft.height() - gCharHeight);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_YELLOW);
  tft.print("  --Open console monitor to see unit test results--");

  // ----- verify Morse code
  dacMorse.setup();
  dacMorse.dump();
  
  Serial.print("\nStarting dits\n");
  for (int ii=1; ii<=40; ii++) {
    Serial.print(ii); Serial.print(" ");
    if (ii%10 == 0) Serial.print("\n");
    dacMorse.send_dit();
  }
  Serial.print("Finished dits\n");
  dacMorse.send_word_space();
  
  // ----- test dit-dah
  Serial.print("\nStarting dit-dah\n");
  for (int ii=1; ii<=20; ii++) {
    Serial.print(ii); Serial.print(" ");
    if (ii%10 == 0) Serial.print("\n");
    dacMorse.send_dit();
    dacMorse.send_dah();  
  }
  dacMorse.send_word_space();
  Serial.print("Finished dit-dah\n");

  // ----- writing proportional font
  // visual test: if this code works correctly, each string will exactly erase the previous one
  initFontSizeBig();
  int waitTime = 400;    // fast=10 msec, easy-to-read=1000 msec

  // test that prev text is fully erased when new text written
  int middleRowY = tft.height()/2;
  drawProportionalText(24, middleRowY, String("MM88"),   String("abc"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("abc"),    String("defghi"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("defghi"), String("jklmnop"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("jklmnop"),String("xyz"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("xyz"),    String("qrstu"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("qrstu"),  String("vwxyz0"), true); delay(waitTime);
  drawProportionalText(24, middleRowY, String("vwxyz0"), String(""), true);

  // plotting a series of pushpins (bread crumb trail)
  startGridScreen();        // box outline around grid
  
  float lat = 47.1;         // 10% inside of CN87
  float lon = -124.2;
  int steps = 600;          // number of loops
  float stepsize = 1.0 / 200.0; // number of degrees to move each loop
  for (int ii = 0; ii < steps; ii++) {
    model.gLatitude = lat + ii * stepsize;
    model.gLongitude = lon + ii * stepsize * 5/4;
    updateGridScreen();
    //delay(10);    // commented out to save boot time
  }
  delay(3000);    // commented out to save boot time

  // ----- deriving grid square from lat-long coordinates
  // Expected values from: https://www.movable-type.co.uk/scripts/latlong.html
  //              expected     lat        long
  testCalcLocator("CN87us",  47.753000, -122.28470);  // user must read console log for failure messages
  testCalcLocator("EM66pd",  36.165926, -86.723285);  // +,-
  testCalcLocator("OF86cx", -33.014673, 116.230695);  // -,+
  testCalcLocator("FD54oq", -55.315349, -68.794971);  // -,-
  testCalcLocator("PM85ge",  35.205535, 136.565790);  // +,+

  // ----- computing distance
  //              expected    fromLat     toLat
  testDistanceLat("30.1",    47.56441,   48.00000);     // from home to north, 48.44 km = 30.10 miles
  testDistanceLat("39.0",    47.56441,   47.00000);     //  "    "   "  south, 62.76 km = 39.00 miles
  //              expected   lat     fromLong    toLong
  testDistanceLong("13.2", 47.7531, -122.2845, -122.0000);   //  "    "   "  east,  x.xx km =  x.xx miles
  testDistanceLong("79.7", 47.7531, -122.2845, -124.0000);   //  "    "   "  west,  xx.x km = xx.xx miles
  testDistanceLong("52.9", 67.5000, -158.0000, -156.0000);   // width of BP17 Alaska, 85.1 km = 52.88 miles
  testDistanceLong("93.4", 47.5000, -124.0000, -122.0000);   // width of CN87 Seattle, 150.2 km = 93.33 miles
  testDistanceLong("113 ", 35.5000, -116.0000, -118.0000);   // width of DM15 California is >100 miles, 181 km = 112.47 miles
  testDistanceLong("138 ",  0.5000,  -80.0000,  -78.0000);   // width of FJ00 Ecuador is the largest possible, 222.4 km = 138.19 miles

  // ----- finding gridlines on E and W
  //                  expected  fromLongitude
  testNextGridLineEast(-122.0, -122.2836);
  testNextGridLineWest(-124.0, -122.2836);

  testNextGridLineEast(-120.0, -121.8888);
  testNextGridLineWest(-122.0, -121.8888);

  testNextGridLineEast( 14.0, 12.3456);
  testNextGridLineWest( 12.0, 12.3456);

  delay(4000);                      // give user time to inspect display appearance for unit test problems
#endif // RUN_UNIT_TESTS
  //tft.fillScreen(ILI9341_BLACK);  // no clear - the next "startXxxxView" will clear screen
}

//==============================================================
//
//      Controller
//      This is MVC (model-view-controller) design pattern
//
//==============================================================
// first entry is the opening view
void (*gaUpdateView[])() = {
    // array of pointers to functions that take no arguments and return void
    updateGridScreen,     // first entry is the first view displayed after setup()
    updateStatusScreen,
    updateVolumeScreen,
    updateSplashScreen,
    updateHelpScreen,
};
void (*gaStartView[])() = {
    startGridScreen,      // first entry is the first view displayed
    startStatScreen,
    startVolumeScreen,
    startSplashScreen,
    startHelpScreen,
};
bool (*gaOnTouch[])(Point touch) = {
    onTouchGrid,
    onTouchStatus,
    onTouchVolume,
    onTouchSplash,
    onTouchHelp,
};
void selectNewView() {
  Serial.print("selectNewView() from "); Serial.print(gViewIndex);
  gViewIndex = (gViewIndex + 1) % gNumViews;
  Serial.print(" to "); Serial.println(gViewIndex);

  // Every view has an initial setup to prepare its layout
  // After initial setup the view can assume it "owns" the screen
  // and can safely repaint only the parts that change
  gaStartView[gViewIndex]();
}

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

int gAddDotX = 10;                     // current screen column, 0..319 pixels
int gRmvDotX = 0;
void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  gAddDotX = (gAddDotX + 1) % gScreenWidth;   // advance
  gRmvDotX = (gRmvDotX + 1) % gScreenWidth;   // advance
  tft.drawPixel(gAddDotX, row, foreground);   // write new
  tft.drawPixel(gRmvDotX, row, background);   // erase old
}

//=========== setup ============================================
void setup() {

  // ----- init serial monitor
  Serial.begin(115200);           // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " __DATE__ " " __TIME__);  // Report our compiled date
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
  Serial.print("Sending command to query GPS Firmware: ");
  Serial.println(PMTK_Q_RELEASE);     // Echo query to console
  GPS.sendCommand(PMTK_Q_RELEASE);    // Send query to GPS unit
                                      // expected reply: $PMTK705,AXN_2.10...
  //GPS.sendCommand(PGCMD_ANTENNA);   // Request antenna status (comment out to keep quiet)
                                      // expected reply: $PGTOP,11,...

  // ----- init digital potentiometer
  volume.setWiperPosition( gVolume );
  Serial.print("Set wiper position = "); Serial.println(gVolume);

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
  
  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(1);                 // landscape (default is portrait)
  tft.fillScreen(ILI9341_BLACK);      // clear screen

  // ----- unit tests (if allowed by RUN_UNIT_TESTS)
  runUnitTest();

  startSplashScreen();

  // at 18 wpm, it takes 12 seconds to send "de k7bwh es km7o" 
  dacMorse.setMessage("de k7bwh es km7o");
  dacMorse.sendBlocking();            // TODO - send non-blocking
  delay(1000);

  startHelpScreen();
  delay(2000);

  // ----- select opening view screen
  gaStartView[gViewIndex]();          // start current view, eg, startGridScreen()
  gaUpdateView[gViewIndex]();         // update current view, eg, updateGridScreen()
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTimeGPS = millis();
//uint32_t prevTimeMorse = millis();

const int GPS_PROCESS_INTERVAL = 1200;  // milliseconds between updating the model's GPS data

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
      Serial.print(GPS.lastNMEA());   // debug
    }
  }

  // periodically, ask the model to process and save our current location
  if (millis() - prevTimeGPS > GPS_PROCESS_INTERVAL) {
    prevTimeGPS = millis();           // restart another interval

    model.processGPS();               // update model

    // update View - call the current viewing function
    gaUpdateView[gViewIndex]();       // update current view, eg, updateGridScreen()
  }

  //if (!spkrMorse.continueSending()) {
  //  // give processing time to SpeakerMorseSender component
  //  // "continueSending" returns false after the message finishes sending
  //}

  // if there's an alert, tell the user
  if (model.signalLost()) {
    model.indicateSignalLost();       // update model
    sendMorseLostSignal();            // announce GPS signal lost by Morse code
  }

  if (model.enteredNewGrid()) {
    gaUpdateView[gViewIndex]();       // update display first, and then...

    sendMorseGrid4(model.gsGridName); // announce new grid by Morse code

    model.grid4dirty = true;
    model.grid6dirty = true;
  }

  // if there's touchscreen input, handle it
  Point touch;
  if (newScreenTap(&touch)) {
    bool touchHandled = gaOnTouch[gViewIndex](touch);
    if (!touchHandled) {
      // not handled by one of the views, so run our default action
      if (touch.y < gScreenHeight / 2) {
        selectNewView();                // change view
      } else {
        adjustBrightness();             // change brightness
      }
    }
  }

  // make a small progress bar crawl along bottom edge
  // this gives a sense of how frequently the main loop is executing
  showActivityBar(239, ILI9341_RED, ILI9341_BLACK);
}