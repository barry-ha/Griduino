/*
  Barograph -- demonstrate BMP388 barometric sensor

  Version history: 
            2021-01-30 added support for BMP390 and latest Adafruit_BMP3XX library
            2020-05-18 added NeoPixel control to illustrate technique
            2020-05-12 updated TouchScreen code
            2020-03-05 replaced physical button design with touchscreen
            2019-20-18 created from example by John KM7O

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  What can you do with a graphing barometer that knows what time it is?
            This program timestamps each reading and stores it for later.
            It graphs the most recent 48 hours. 
            (Todo) It saves the readings in non-volatile memory and re-displays them on power-up.
            The RTC (realtime clock) is updated from the GPS satellite network.

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)     https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341    https://www.adafruit.com/product/1743

         3. Adafruit BMP388 - Precision Barometric Pressure https://www.adafruit.com/product/3966
            Adafruit BMP390                                 https://www.adafruit.com/product/4816

         4. Adafruit Ultimate GPS                           https://www.adafruit.com/product/746

*/

#include "Adafruit_GFX.h"             // Core graphics display library
#include "Adafruit_ILI9341.h"         // TFT color display library
#include "TouchScreen.h"              // Touchscreen built in to 3.2" Adafruit TFT display
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "Adafruit_GPS.h"             // Ultimate GPS library
#include "Adafruit_BMP3XX.h"          // Precision barometric and temperature sensor
#include "Adafruit_NeoPixel.h"        // On-board color addressable LED
#include "bitmaps.h"                  // our definition of graphics displayed

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "Barograph Demo"
#define PROGRAM_VERSION "v0.31"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

#define SCREEN_ROTATION 1             // 1=landscape, 3=landscape 180-degrees

//--------------CONFIG--------------
//float elevCorr = 4241;              // elevation correction in Pa, 
// use difference between altimeter setting and station pressure: https://www.weather.gov/epz/wxcalc_altimetersetting

float elevCorr = 0;
float maxP = 104000;                  // in Pa
float minP = 98000;                   // in Pa
const int yShift = 9;                 // in pixels
const int xIndent = 12;               // in pixels, text on main screen

enum units { eMetric, eEnglish };
int units = eMetric;                  // units on startup: 0=english=inches mercury, 1=metric=millibars

// ---------- Hardware Wiring ----------
/* Same as Griduino platform
*/

// Adafruit Feather M4 Express pin definitions
// To compile for Feather M0/M4, install "additional boards manager"
// https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup

#define TFT_BL   4                  // TFT backlight
#define TFT_CS   5                  // TFT chip select pin
#define TFT_DC  12                  // TFT display/command pin
#define BMP_CS  13                  // BMP388 sensor, chip select

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Touch Screen
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// This sketch has just one touch area that covers the entire screen

// Adafruit Feather M4 Express pin definitions
#define PIN_XP  A3                    // Touchscreen X+ can be a digital pin
#define PIN_XM  A4                    // Touchscreen X- must be an analog pin, use "An" notation
#define PIN_YP  A5                    // Touchscreen Y+ must be an analog pin, use "An" notation
#define PIN_YM   9                    // Touchscreen Y- can be a digital pin

TouchScreen ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, 295);

// ---------- Barometric and Temperature Sensor
Adafruit_BMP3XX baro;                 // hardware SPI

// ---------- Feather's onboard lights
//efine PIN_NEOPIXEL 8                // already defined in Feather's board variant.h
//efine PIN_LED 13                    // already defined in Feather's board variant.h

#define NUMPIXELS 1                   // Feather M4 has one NeoPixel on board
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

const int MAXBRIGHT = 255;            // = 100% brightness = maximum allowed on individual LED
const int BRIGHT = 32;                // = tolerably bright indoors
const int HALFBR = 20;                // = half of tolerably bright
const int OFF = 0;                    // = turned off

const uint32_t colorRed    = pixel.Color(HALFBR, OFF,    OFF);
const uint32_t colorGreen  = pixel.Color(OFF,    HALFBR, OFF);
const uint32_t colorBlue   = pixel.Color(OFF,    OFF,    BRIGHT);
const uint32_t colorPurple = pixel.Color(HALFBR, OFF,    HALFBR);

// ---------- GPS ----------
/* "Ultimate GPS" pin wiring is connected to a dedicated hardware serial port
    available on an Arduino Mega, Arduino Feather and others.

    The GPS' LED indicates status:
        1-sec blink = searching for satellites
        15-sec blink = position fix found
*/

// Hardware serial port for GPS
Adafruit_GPS GPS(&Serial1);

// ------------ typedef's
struct Point {
  int x, y;
};
struct Reading {
  float pressure;   // in millibars, from BMP388 sensor
  int hh, mm, ss;   // in GMT, from realtime clock
};

// ------------ definitions
const int howLongToWait = 4;  // max number of seconds at startup waiting for Serial port to console
#define SCREEN_ROTATION 1             // 1=landscape, 3=landscape 180 degrees

#define FEET_PER_METER 3.28084
#define SEA_LEVEL_PRESSURE_HPA (1013.25)

//efine MILLIBARS_PER_INCHES_MERCURY (33864.0)  // todo - is this wrong? https://www.calculateme.com/pressure/
#define MILLIBARS_PER_INCHES_MERCURY (0.02953)
#define BARS_PER_INCHES_MERCURY      (0.033864)
#define PASCALS_PER_INCHES_MERCURY   (3386.4)

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND     0x00A           // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cSCALECOLOR     ILI9341_YELLOW
#define cGRAPHCOLOR     ILI9341_WHITE   // graphed line of baro pressure
#define cTEXTCOLOR      ILI9341_GREEN
#define cLABEL          ILI9341_GREEN
//efine cVALUE          ILI9341_YELLOW
//efine cVALUEFAINT     0xbdc0          // darker than cVALUE
//efine cHIGHLIGHT      ILI9341_WHITE
//efine cBUTTONFILL     ILI9341_NAVY
//efine cBUTTONOUTLINE  ILI9341_BLUE    // 0,   0, 255 = darker than cyan
//efine cBUTTONLABEL    ILI9341_YELLOW
//efine cTEXTCOLOR      ILI9341_CYAN    // 0, 255, 255
//efine cTEXTFAINT      0x0514          // 0, 160, 160 = blue, between CYAN and DARKCYAN
#define cICON           ILI9341_CYAN
//efine cINPUT          ILI9341_WHITE
#define cWARN           0xF844          // brighter than ILI9341_RED but not pink
#define cSINKING        0xF882          // highlight rapidly sinking barometric pressure
#define cTOUCHTARGET    ILI9341_RED     // outline touch-sensitive areas

// ------------ global barometric data
float inchesHg;
float gPressure;
float hPa;
float feet;
float tempF;

float deltaPressure1h = 5000;             // 5k is arbitrary, it just needs to be out of range to prevent display on cold start
float deltaPressure3h = 5000;

// 144 steps at 20 minute refresh time is a 2880 minute (48 hr) graph with 20 minute resolution.
// with 2px per step, we get 10 minutes/px, and 288px per 48 hrs, leaving some space for graph labels
const int maxReadings = 144;
Reading pressureStack[maxReadings] = {};    // array to hold pressure data, fill with zeros
const int lastIndex = maxReadings - 1;      // index to the last element in pressure array

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
    // The built-in "isTouching" function does most of the debounce and threshhold detection needed
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
  #define TOUCHPRESSURE 200           // Minimum pressure we consider true pressing
  static bool button_state = false;
  uint16_t pres_val = ::myPressure();

  if ((button_state == false) && (pres_val > TOUCHPRESSURE)) {
    Serial.print(". pressed, pressure = "); Serial.println(pres_val);     // debug
    button_state = true;
  }

  if ((button_state == true) && (pres_val < TOUCHPRESSURE)) {
    Serial.print(". released, pressure = "); Serial.println(pres_val);       // debug
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

// ======== barometer and temperature helpers ==================
void getBaroPressure() {
  // returns: gPressure (global var)
  //          hPa       (global var)
  //          inchesHg  (global var)
  if (!baro.performReading()) {
    Serial.println("Error, failed to read barometer");
  }
  // continue anyway, for demo
  gPressure = baro.pressure + elevCorr;   // Pressure is returned in the SI units of Pascals. 100 Pascals = 1 hPa = 1 millibar
  hPa = gPressure / 100;
  inchesHg = 0.0002953 * gPressure;
}

// ========== font management helpers ==========================
/* Using fonts: https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts

  "Fonts" folder is inside \Documents\User\Arduino\libraries\Adafruit_GFX
*/
#include "Fonts/FreeSans18pt7b.h"   // we use double-size 18-pt font
void initFontSizeBig() {            // otherwise the largest single-size font is 24-pt
  tft.setFont(&FreeSans18pt7b);     // which looks smoother but is not big enough
  tft.setTextSize(2);
}
#include "Fonts/FreeSans12pt7b.h"   // 12-pt font for local time and temperature
void initFontSizeSmall() {
  tft.setFont(&FreeSans12pt7b);
  tft.setTextSize(1);
}
#include "Fonts/FreeSans9pt7b.h"    // smallest font for program name
void initFontSizeSmallest() {
  tft.setFont();                    // no arg = system font
  tft.setTextSize(1);
}

// ========== splash screen helpers ============================
// splash screen layout
#define yRow1   54                // program title: "Barograph"
#define yRow2   yRow1 + 28        // program version
#define yRow3   yRow2 + 48        // author line 1
#define yRow4   yRow3 + 32        // author line 2

TextField txtSplash[] = {
  //        text               x,y       color  
  {PROGRAM_TITLE,    -1,yRow1,  cTEXTCOLOR}, // [0] program title, centered
  {PROGRAM_VERSION,  -1,yRow2,  cLABEL},     // [1] normal size text, centered
  {PROGRAM_LINE1,    -1,yRow3,  cLABEL},     // [2] credits line 1, centered
  {PROGRAM_LINE2,    -1,yRow4,  cLABEL},     // [3] credits line 2, centered
  {"Compiled " PROGRAM_COMPILED,       
                     -1,228,    cTEXTCOLOR}, // [4] "Compiled", bottom row
};
const int numSplashFields = sizeof(txtSplash)/sizeof(TextField);

void startSplashScreen() {
  clearScreen();                                    // clear screen
  txtSplash[0].setBackground(cBACKGROUND);          // set background for all TextFields
  TextField::setTextDirty( txtSplash, numSplashFields ); // make sure all fields are updated

  initFontSizeSmall();
  for (int ii=0; ii<4; ii++) {
    txtSplash[ii].print();
  }

  initFontSizeSmallest();
  for (int ii=4; ii<numSplashFields; ii++) {
    txtSplash[ii].print();
  }
}

// ========== graph screen layout ==============================
const int graphHeight = 135;    // in pixels
const int xLeft = 31;           // in pixels
const int xRight = 319;         // in pixels
const int xMiddle = (xLeft + xRight)/2;
const int yLine1 = 20;          // in pixels, top row, main screen
const int yLine2 = yLine1 + 26;
const int yLine3 = yLine1 + 52;

// ========== screen helpers ===================================
void blinky(int qty, int waitTime) {
  for (int i = 0; i <= qty; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(waitTime);
    digitalWrite(PIN_LED, LOW);
    delay(waitTime);
  }
}
void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

enum { valPressure, valHour1, valHour3,
       unitPressure, unitHour1, unitHour3 };
TextField txtReading[] = {
  TextField{"30.00",  xIndent+72, yLine1, cTEXTCOLOR, FLUSHRIGHT},   // [valPressure]
  TextField{"---",    xIndent+72, yLine2, cTEXTCOLOR, FLUSHRIGHT},   // [valHour1]
  TextField{"---",    xIndent+72, yLine3, cTEXTCOLOR, FLUSHRIGHT},   // [valHour3]
  TextField{"inHg",   xIndent+82, yLine1, cTEXTCOLOR},               // [unitPressure]
  TextField{"inHg/h", xIndent+82, yLine2, cTEXTCOLOR},               // [unitHour1]
  TextField{"inHg/3h",xIndent+82, yLine3, cTEXTCOLOR},               // [unitHour3]
};
const int numReadings = sizeof(txtReading) / sizeof(TextField);

void showReadings(int units) {
  clearScreen();
  txtSplash[0].setBackground(cBACKGROUND);          // set background for all TextFields
  TextField::setTextDirty( txtReading, numReadings ); // make sure all fields are updated
  initFontSizeSmall();

  printPressure();
  printDeltaP();
  printDeltaP3h();
  drawScale();
  tickMarks(3, 5);      // args: (hours, height)
  tickMarks(6, 8);
  scaleMarks(500, 5);   // args: (Pa, length)
  scaleMarks(1000, 8);
  drawGraph();
  drawIcon();
}

// ----- print current value of pressure reading
void printPressure() {
  float fPressure;
  char* sUnits;
  char inHg[] = "inHg";
  char hPa[] = "hPa";
  switch (units) {
    case eEnglish:
      fPressure = gPressure / 3386.4;
      sUnits = inHg;
      break;
    case eMetric:
      fPressure = gPressure / 100;
      sUnits = hPa;
      break;
    default:
      Serial.println("Internal Error! Unknown units");
      break;
  }
  txtReading[valPressure].print( fPressure, 2 );
  txtReading[unitPressure].print( sUnits );
  Serial.print(fPressure, 2); Serial.println(sUnits);
}

// ----- print 1-hour pressure change
void printDeltaP() {

  // choose color
  if (deltaPressure1h < -116) {
    txtReading[valHour1].color = cSINKING;
    txtReading[unitHour1].color = cSINKING;
  } else {
    txtReading[valHour1].color = cTEXTCOLOR;
    txtReading[unitHour1].color = cTEXTCOLOR;
  }
  // show value of 1-hour change
  switch (units) {
    case eEnglish:
      if (abs(deltaPressure1h) > 400) {
        // no readings yet, show placeholder
        txtReading[valHour1].print("---");
      } else {
        txtReading[valHour1].print( deltaPressure1h, 0 );
      }
      break;
    case eMetric:
      if (abs(deltaPressure1h) > 2000) {
        // no readings yet, show placeholder
        txtReading[valHour1].print("---");
      } else {
        txtReading[valHour1].print( deltaPressure1h / 3386.4, 2 );
      }
      break;
  }
  // show units of 1-hour change
  txtReading[unitHour1].dirty = true;
  txtReading[unitHour1].print();
  Serial.print(". unitHour1 = "); Serial.println( txtReading[unitHour1].text );
}

// ----- print 3-hour pressure change
void printDeltaP3h() {

  // choose color
  if (deltaPressure3h < -348) {
    txtReading[valHour3].color = cSINKING;
    txtReading[unitHour3].color = cSINKING;
  } else {
    txtReading[valHour3].color = cTEXTCOLOR;
    txtReading[unitHour3].color = cTEXTCOLOR;
  }
  // show value of 3-hour change
  switch (units) {
    case eMetric:
      if (abs(deltaPressure3h) > 2000) {
        // no readings yet, show placeholder
        txtReading[valHour3].print("---");
      } else {
        txtReading[valHour3].print( deltaPressure3h / 3386.4, 2 );  // pascals per inch Mercury
      }
      break;
    case eEnglish:
      if (abs(deltaPressure3h) > 400) {
        // no readings yet, show placeholder
        txtReading[valHour3].print("---");
      } else {
        txtReading[valHour3].print( deltaPressure3h, 0 );
      }
      break;
  }
  // show units of 3-hour change 
  txtReading[unitHour3].print();
  Serial.print(". unitHour3 = "); Serial.println( txtReading[unitHour3].text );
}

void drawIcon() {
  // todo - erase previous icon
  if ( (-2000) < deltaPressure1h && deltaPressure1h <= (-200)) {
    tft.drawBitmap(230, 5, vRapidlyFalling, 80, 80, cICON);
  }
  else if ( (-200) < deltaPressure1h && deltaPressure1h <= (-116)) {
    tft.drawBitmap(230, 5, rapidlyFalling, 80, 80, cICON);
  }
  else if ( (-116) < deltaPressure1h && deltaPressure1h <= (-50)) {
    //               x, y  bitmap    w, h   color
    tft.drawBitmap(230, 5, falling, 80, 80, cICON);
  }
  else if ( (-50) <= deltaPressure1h && deltaPressure1h < 50) {
    tft.drawBitmap(230, 5, rising, 80, 80, cICON);
  }
  else if ( 50 <= deltaPressure1h && deltaPressure1h < 116) {
    tft.drawBitmap(230, 5, rising, 80, 80, cICON);
  }
  else if ( 116 <= deltaPressure1h && deltaPressure1h < 200) {
    tft.drawBitmap(230, 5, rapidlyRising, 80, 80, cICON);
  }
  else if ( 200 <= deltaPressure1h && deltaPressure1h < 2000) {
    tft.drawBitmap(230, 5, vRapidlyRising, 80, 80, cICON);
  }
  tft.drawBitmap(230, 5, steady, 80, 80, cICON);   // catch-all, should not happen
}

void tickMarks(int t, int h) {    // t in hours
  int deltax = t * 6;
  for (int x=xRight; x>xLeft; x = x - deltax) {
    tft.drawLine(x, 239 - yShift, x, 239 - yShift - h, cSCALECOLOR);
  }
}

void scaleMarks(int p, int len) { //t in hours
  int y = 239 - yShift;
  int deltay = map(p, 0, maxP - minP, 0, graphHeight);
  for (y = 239 - yShift; y > 239 - graphHeight - yShift + 5; y = y - deltay) {
    tft.drawLine(xLeft, y,  xLeft + len,  y, cSCALECOLOR);
    tft.drawLine(xRight, y, xRight - len, y, cSCALECOLOR);
  }
}

void drawScale() {
  // write limits of pressure scale in consistent units
  initFontSizeSmallest();
  tft.setTextColor(cSCALECOLOR);
  switch (units) {
    case eEnglish:
      tft.setCursor(1, 233 - yShift);
      tft.print(minP / 3386.4, 1);
      tft.setCursor(1, 239 - graphHeight - yShift);
      tft.print(maxP / 3386.4, 1);
      tft.setCursor(1, 233 - (graphHeight / 2) - yShift);
      tft.print((minP + (maxP - minP) / 2) / 3386.4, 1);
      break;
    case eMetric:
      tft.setCursor(10, 233 - yShift);
      tft.print(minP / 100, 0);
      tft.setCursor(4, 239 - graphHeight - yShift);
      tft.print(maxP / 100, 0);
      tft.setCursor(4, 236 - (graphHeight / 2) - yShift);
      tft.print((minP + (maxP - minP) / 2) / 100, 0);
      break;
    default:
      Serial.print("Internal Error: Unknown units ("); Serial.print(units); Serial.println(")");
      break;
  }

  // draw horizontal lines
  const int yLine1 = 239 - graphHeight - yShift;
  const int yLine2 = 239 - yShift;
  const int yMid   = (yLine1 + yLine2)/2;
  tft.drawLine(xLeft, yLine1, xRight, yLine1, cSCALECOLOR);
  tft.drawLine(xLeft, yLine2, xRight, yLine2, cSCALECOLOR);
  for (int ii=xLeft; ii<xRight; ii+=10) {   // dotted line
    tft.drawPixel(ii, yMid, cSCALECOLOR);
    tft.drawPixel(ii+1, yMid, cSCALECOLOR);
  }

  // draw vertical lines
  tft.drawLine(xRight,  239 - graphHeight - yShift, xRight,  239 - yShift, cSCALECOLOR);
  tft.drawLine(xMiddle, 239 - graphHeight - yShift, xMiddle, 239 - yShift, cSCALECOLOR);
  tft.drawLine(xLeft,   239 - graphHeight - yShift, xLeft,   239 - yShift, cSCALECOLOR);

  // label vertical lines at 24, 48 hours
  tft.setCursor(xMiddle, 232);
  tft.print(24);
  tft.setCursor(xLeft, 232);;
  tft.print(48);
}

void drawGraph() {
  int deltax = 2;
  int x = xRight;
  for (int ii = lastIndex; ii > 0 ; ii--) {
    int p1 =  map(pressureStack[ii-0].pressure, minP, maxP, 0, graphHeight);
    int p2 =  map(pressureStack[ii-1].pressure, minP, maxP, 0, graphHeight);
    if (pressureStack[ii - 1].pressure != 0) {
      tft.drawLine(x, 239 - p1 - yShift, x - deltax, 239 - p2 - yShift, cGRAPHCOLOR);
    }
    x = x - deltax;
  }
}

void adjustUnits() {
  if (units == eMetric) {
    units = eEnglish;
  } else {
    units = eMetric;
  }

  char sInchesHg[] = "inHg";
  char sInchesHg1h[] = "inHg/1h";
  char sInchesHg3h[] = "inHg/3h";
  char sHectoPa[] = "hPa";
  char sPascal1h[] = "Pa/h";
  char sPascal3h[] = "Pa/3h";
  switch (units) {
    case eEnglish:
      Serial.print("Units changed to English: "); Serial.println(units);
      txtReading[unitPressure].print( sInchesHg );
      txtReading[unitHour1].print( sInchesHg1h );
      txtReading[unitHour3].print( sInchesHg3h );
      break;
    case eMetric:
      Serial.print("Units changed to Metric: "); Serial.println(units);
      txtReading[unitPressure].print( sHectoPa );
      txtReading[unitHour1].print( sPascal1h );
      txtReading[unitHour3].print( sPascal3h );
      break;
    default:
      Serial.println("Internal Error! Unknown units");
      break;
  }

  Serial.print("- unitPressure = "); Serial.println( txtReading[unitPressure].text );
  Serial.print("- unitHour1 = "); Serial.println( txtReading[unitHour1].text );
  Serial.print("- unitHour3 = "); Serial.println( txtReading[unitHour3].text );
  getBaroPressure();
  showReadings(units);
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong*1000;
  while (millis() < targetTime) {
    if (Serial) break;
  }
}

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  static int addDotX = 10;                    // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count = 0;
  const int SCALEF = 2048;                    // how much to slow it down so it becomes visible

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

  // ----- init serial monitor
  Serial.begin(115200);               // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);       // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init GPS
  GPS.begin(9600);                              // 9600 NMEA is the default baud rate for Adafruit MTK GPS's
  delay(50);                                    // is delay really needed?
  GPS.sendCommand(PMTK_SET_BAUD_57600);         // set baud rate to 57600
  delay(50);
  GPS.begin(57600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); // turn on RMC (recommended minimum) and GGA (fix data) including altitude

  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    // 1 Hz update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ); // Once every 5 seconds update

  Serial.print("Last element index = "); Serial.println(lastIndex);

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(SCREEN_ROTATION);   // 1=landscape (default is 0=portrait)
  clearScreen();

  // ----- init Feather M4 onboard lights
  pixel.begin();
  pixel.clear();                      // turn off NeoPixel
  digitalWrite(PIN_LED, LOW);         // turn off little red LED
  Serial.println("NeoPixel initialized and turned off");

  /* ***** test code commented out...
  delay(3000);     // waiting for serial console to attach
  for (int ii=0; ii<10; ii++) {
    Serial.print(ii); 
    Serial.print(". Red ");
    pixel.setPixelColor(0, colorRed);
    pixel.show();
    digitalWrite(PIN_LED, HIGH);       // little red LED on
    delay(800);

    Serial.print(". Green ");
    pixel.setPixelColor(0, colorGreen);
    pixel.show();
    digitalWrite(PIN_LED, LOW);        // little red LED off
    delay(800);

    Serial.print(". Blue ");
    pixel.setPixelColor(0, colorBlue);
    pixel.show();
    digitalWrite(PIN_LED, HIGH);       // little red LED on
    delay(800);
    
    Serial.print(". Purple ");
    pixel.setPixelColor(0, colorPurple);
    pixel.show();
    digitalWrite(PIN_LED, LOW);        // little red LED off
    delay(800);

    Serial.println();
  }
  pixel.clear();
  ... end test code ***** */

  // ----- announce ourselves
  startSplashScreen();

  delay(4000);         // milliseconds
  clearScreen();

  // ----- init BMP388 or BMP390 barometer
  if (baro.begin_SPI(BMP_CS)) {
    // success
  } else {
    // failed to initialize hardware
    Serial.println("Error, unable to initialize BMP388, check your wiring");
    tft.setCursor(0, yLine1);
    tft.setTextColor(cWARN);
    initFontSizeSmall();
    tft.println("Error!\n Unable to init\n  BMP388 sensor\n   check wiring");
    delay(4000);
  }

  // Set up BMP388 oversampling and filter initialization
  // https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf
  // Section 3.5 Filter Selection, page 17
  baro.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);
  baro.setPressureOversampling(BMP3_OVERSAMPLING_32X);
  baro.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_127);
  // baro.setOutputDataRate(BMP3_ODR_50_HZ);

  // Get first data point (done twice because first reading is always bad)
  getBaroPressure();
  pressureStack[lastIndex].pressure = gPressure + elevCorr;
  showReadings(units);

  getBaroPressure();
  pressureStack[lastIndex].pressure = gPressure + elevCorr;
  showReadings(units);
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTimer1 = millis();    // timer for value update (5 min)
uint32_t prevTimer2 = millis();    // timer for graph/array update (20 min)
uint32_t prevTimer3 = millis();    // timer for emergency alert (10 sec)

const int READ_BAROMETER_INTERVAL = 5*60*1000;  // Timer 1
const int LOG_PRESSURE_INTERVAL = 20*60*1000;   // Timer 2
const int CHECK_CRASH_INTERVAL = 10*1000;       // Timer 3

void loop() {

  // if a timer or system millis() wrapped around, reset it
  if (prevTimer1 > millis()) { prevTimer1 = millis(); }
  if (prevTimer2 > millis()) { prevTimer2 = millis(); }
  if (prevTimer3 > millis()) { prevTimer3 = millis(); }

  // check for crashing pressure and blink a LED if it's happening
  if (millis() - prevTimer3 > CHECK_CRASH_INTERVAL) {
    if ( (-200) < deltaPressure1h && deltaPressure1h <= (-116)) {
      blinky(2, 500);
    }
    else if ( (-2000) < deltaPressure1h && deltaPressure1h <= (-200)) {
      blinky(5, 200);
    }
    prevTimer3 = millis();
    return;
  }

  // every 5 minutes acquire/print temp and pressure
  if (millis() - prevTimer1 > READ_BAROMETER_INTERVAL) {
    getBaroPressure();
    showReadings(units);

    // every 20 minutes log, display, and graph pressure/delta pressure
    if (millis() - prevTimer2 > LOG_PRESSURE_INTERVAL) {  // 1200000 for 20 minutes
      //shift array left
      int i = 0;
      for (i = 0; i < lastIndex; i++) {
        pressureStack[i] = pressureStack[i + 1];
      }
      // and put the latest pressure onto the stack
      pressureStack[lastIndex].pressure = gPressure;
      //pressureStack[lastIndex].hh = GPS.hour;     // todo
      //pressureStack[lastIndex].mm = GPS.minute;   // todo
      //pressureStack[lastIndex].ss = GPS.seconds;  // todo
      
      //calculate pressure change and reprint all to screen
      deltaPressure1h = pressureStack[lastIndex].pressure - pressureStack[140].pressure;    // todo: replace magic numbers
      deltaPressure3h = pressureStack[lastIndex].pressure - pressureStack[134].pressure;
      showReadings(units);
      prevTimer2 = millis();
    }
    prevTimer1 = millis();
  }

  // if there's touchscreen input, handle it
  Point touch;
  if (newScreenTap(&touch)) {

    #ifdef SHOW_TOUCH_TARGETS
      const int radius = 3;           // debug
      tft.fillCircle(touch.x, touch.y, radius, cWARN);  // debug - show dot
    #endif
    //touchHandled = true;      // debug - true=stay on same screen

    adjustUnits();              // change between "inches mercury" and "millibars" units
  }


  // make a small progress bar crawl along bottom edge
  // this gives a sense of how frequently the main loop is executing
  //showActivityBar(239, ILI9341_RED, ILI9341_BLACK);
}
