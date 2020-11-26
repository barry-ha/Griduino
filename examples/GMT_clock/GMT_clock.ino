/*
  GMT_clock - bright colorful Greenwich Mean Time based on GPS

  Date:     2020-06-03 merged this clock into the main Griduino program as another view
            2020-05-13 proportional fonts
            2020-05-12 updated TouchScreen code
            2020-05-01 added save/restore to nonvolatile RAM
            2020-04-29 added touch adjustment of local time zone
            2020-04-22 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This program offers a bright colorful GMT clock for your shack
            or dashboard. After all, once a rover arrives at a destination
            they no longer need grid square navigation.

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)     https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341    https://www.adafruit.com/product/1743
            How to:      https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2
            SPI Wiring:  https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/spi-wiring-and-test
            Touchscreen: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/resistive-touchscreen

         3. Adafruit Ultimate GPS
            Spec: https://www.adafruit.com/product/746

*/

#include "SPI.h"                      // Serial Peripheral Interface
#include "Adafruit_GFX.h"             // Core graphics display library
#include <Adafruit_ILI9341.h>         // TFT color display library
#include "Adafruit_GPS.h"             // Ultimate GPS library
#include "Adafruit_BMP3XX.h"          // Precision barometric and temperature sensor
#include "Adafruit_NeoPixel.h"        // On-board color addressable LED
#include "TouchScreen.h"              // Touchscreen built in to 3.2" Adafruit TFT display
#include "save_restore.h"             // Save configuration in non-volatile RAM
#include "TextField.h"                // Optimize TFT display text for proportional fonts

// ------- TFT 4-Wire Resistive Touch Screen configuration parameters
#define TOUCHPRESSURE 200             // Minimum pressure threshhold considered an actual "press"
#define XP_XM_OHMS    295             // Resistance in ohms between X+ and X- to calibrate pressure

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "Griduino GMT Clock"
#define PROGRAM_VERSION "v0.29"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

//#define ECHO_GPS_SENTENCE           // comment out to quiet down the serial monitor
//#define SHOW_TOUCH_TARGET           // comment out for production

// ========== forward reference ================================
void timeZonePlus();
void timeZoneMinus();

// ---------- Hardware Wiring ----------
/* Same as Griduino platform
*/

// TFT display and SD card share the hardware SPI interface, and have
// separate 'select' pins to identify the active device on the bus.
#if defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  // To compile for Feather M0/M4, install "additional boards manager"
  // https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup
  
  #define TFT_BL   4                  // TFT backlight
  #define TFT_CS   5                  // TFT chip select pin
  #define TFT_DC  12                  // TFT display/command pin
  #define BMP_CS  13                  // BMP388 sensor, chip select

#elif defined(ARDUINO_AVR_MEGA2560)
  #define TFT_BL   6                  // TFT backlight
  #define TFT_DC   9                  // TFT display/command pin
  #define TFT_CS  10                  // TFT chip select pin
  #define BMP_CS  13                  // BMP388 sensor, chip select

#else
  #warning You need to define pins for your hardware

#endif

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Touch Screen
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// This sketch has only two touch areas to make it easy for operator to select
// "left half" and "right half" without looking. Touch target precision is not essential.
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

// ---------- Barometric and Temperature Sensor
Adafruit_BMP3XX baro(BMP_CS);         // hardware SPI

// ---------- Feather's onboard lights
//efine PIN_NEOPIXEL 8                // already defined in Feather's board variant.h
#define RED_LED 13                    // diagnostics RED LED
//efine PIN_LED 13                    // already defined in Feather's board variant.h

#define NUMPIXELS 1                   // Feather M4 has one NeoPixel on board
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

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
struct Rect {
  Point ul;
  Point size;
  bool contains(const Point touch) {
    if (ul.x <= touch.x && touch.x <= ul.x+size.x
     && ul.y <= touch.y && touch.y <= ul.y+size.y) {
      return true;
     } else {
      return false;
     }
  }
};
typedef void (*simpleFunction)();
typedef struct {
  char text[26];
  int x, y;
  int w, h;
  Rect hitTarget;
  int radius;
  uint16_t color;
  simpleFunction function;
} Button;

// ------------ definitions
const int howLongToWait = 4;          // max number of seconds at startup waiting for Serial port to console
#define SCREEN_ROTATION 1             // 1=landscape, 3=landscape 180 degrees

// ------------ global scope
int gTimeZone = -7;                   // default local time Pacific (-7 hours), saved in nonvolatile memory
int gSatellites = 0;                  // number of satellites

// ----- alias names for setFontSize()
enum { eFONTGIANT=36, eFONTBIG=24, eFONTSMALL=12, eFONTSMALLEST=9, eFONTSYSTEM=0 };

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define BACKGROUND      0x00A           // a little darker than ILI9341_NAVY
#define cBACKGROUND     0x00A           // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cLABEL          ILI9341_GREEN
#define cVALUE          ILI9341_YELLOW  // 255, 255, 0
#define cINPUT          ILI9341_WHITE
//efine cHIGHLIGHT      ILI9341_WHITE
#define cBUTTONFILL     ILI9341_NAVY
#define cBUTTONOUTLINE  ILI9341_BLUE    // 0,   0, 255 = darker than cyan
#define cTEXTCOLOR      ILI9341_CYAN    // 0, 255, 255
#define cTEXTFAINT      0x0514          // 0, 160, 160 = blue, between CYAN and DARKCYAN
#define cBUTTONLABEL    ILI9341_YELLOW
#define cWARN           0xF844          // brighter than ILI9341_RED but not pink
#define cTOUCHTARGET    ILI9341_RED     // outline touch-sensitive areas

// ============== GPS helpers ==================================
void processGPS() {
  // keep track of number of GPS satellites in view as a confidence indicator
  gSatellites = GPS.satellites;

  // this sketch doesn't use GPS lat/long, so we ignore NMEA sentences (except #satellites)
  // IF the GPS has a battery, then its RTC (realtime clock) remembers the time
  // and for all practical purposes the GMT reading is always right.
}

// Formatted Local time
void getTimeLocal(char* result, int len) {
  // @param result = char[10] = string buffer to modify
  // @param len = string buffer length
  int hh = GPS.hour + gTimeZone;      // 24-hour clock (format matches GMT time)
  hh = (hh + 24) % 24;                // ensure positive number of hours
  int mm = GPS.minute;
  int ss = GPS.seconds;
  snprintf(result, len, "%02d:%02d:%02d",
                          hh,  mm,  ss);
}

// Did the GPS report a valid date?
bool isDateValid(int yy, int mm, int dd) {
  if (yy < 19) {
    return false;
  }
  if (mm < 1 || mm > 12) {
    return false;
  }
  if (dd < 1 || dd > 31) {
    return false;
  }
  return true;
}

// Formatted GMT date "Jan 12, 2020"
void getDate(char* result, int maxlen) {
  // @param result = char[15] = string buffer to modify
  // @param maxlen = string buffer length
  // Note that GPS can have a valid date without a position; we can't rely on GPS.fix()
  // to know if the date is correct or not. So we deduce it from the yy/mm/dd values.
  char sDay[3];                       // "12"
  char sYear[5];                      // "2020"
  char aMonth[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "err" };

  uint8_t yy = GPS.year;
  int mm = GPS.month;
  int dd = GPS.day;

  if (isDateValid(yy,mm,dd)) {
    int year = yy + 2000;             // convert two-digit year into four-digit integer
    int month = mm - 1;               // GPS month is 1-based, our array is 0-based
    snprintf(result, maxlen, "%s %d, %4d", 
                  aMonth[month], dd, year);
  } else {
    // GPS does not have a valid date, we will display it as "0000-00-00"
    snprintf(result, maxlen, "0000-00-00");
  }
}

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
      Serial.print("Screen touch detected ("); Serial.print(pPoint->x);
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
float getTemperature() {
  // returns temperature in Farenheight
  if (!baro.performReading()) {
    Serial.println("Error, failed to read barometer");
  }
  // continue anyway, for demo
  float tempF = baro.temperature * 9 / 5 + 32;
  return tempF;
}

// ========== font management helpers ==========================
/* Using fonts: https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts

  "Fonts" folder is inside \Documents\User\Arduino\libraries\Adafruit_GFX
*/
#include "Fonts/FreeSans18pt7b.h"     // eFONTGIANT    36 pt (see constants.h)
#include "Fonts/FreeSansBold24pt7b.h" // eFONTBIG      24 pt
#include "Fonts/FreeSans12pt7b.h"     // eFONTSMALL    12 pt
#include "Fonts/FreeSans9pt7b.h"      // eFONTSMALLEST  9 pt
// (built-in)                         // eFONTSYSTEM    8 pt

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


// ========== splash screen helpers ============================
// splash screen layout
#define yRow1   54                    // program title: "Griduino GMT Clock"
#define yRow2   yRow1 + 28            // program version
#define yRow3   yRow2 + 48            // author line 1
#define yRow4   yRow3 + 32            // author line 2

TextField txtSplash[] = {
  //     text               x,y       color  
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

  setFontSize(eFONTSMALL);
  for (int ii=0; ii<4; ii++) {
    txtSplash[ii].print();
  }

  setFontSize(eFONTSMALLEST);
  for (int ii=4; ii<numSplashFields; ii++) {
    txtSplash[ii].print();
  }
}

// ========== main clock view helpers ==========================
// these are names for the array indexes, must be named in same order as below
enum txtIndex {
  TITLE=0, 
  HOURS, COLON1, MINUTES, COLON2, SECONDS,
  GMTDATE, DEGREES, LOCALTIME, TIMEZONE, NUMSATS,
};

TextField txtClock[] = {
  //       text             x,y    color             index
  {PROGRAM_TITLE,    -1, 14, cTEXTCOLOR},  // [TITLE]     program title, centered
  {"hh",             12, 90, cVALUE},      // [HOURS]     giant clock hours
  {":",              94, 90, cVALUE},      // [COLON1]    :
  {"mm",            120, 90, cVALUE},      // [MINUTES]   giant clock minutes
  {":",             206, 90, cVALUE},      // [COLON2]    :
  {"ss",            230, 90, cVALUE},      // [SECONDS]   giant clock seconds
  {"MMM dd, yyyy",   94,130, cVALUE},      // [GMTDATE]   GMT date
  {"12.3 F",        132,164, cVALUE},      // [DEGREES]   Temperature
  {"hh:mm:ss",      118,226, cTEXTCOLOR},  // [LOCALTIME] Local time
  {"-7h",             8,226, cTEXTFAINT},  // [TIMEZONE]  addHours time zone
  {"6#",            308,226, cTEXTFAINT, ALIGNRIGHT},  // [NUMSATS]   numSats
};
const int numClockFields = sizeof(txtClock)/sizeof(TextField);

Button timeButtons[] = {
  // For "GMT_clock" we have rather small modest +/- buttons, meant to visually
  // fade a little into the background. However, we want larger touch-targets to 
  // make them easy to press.
  //
  // 3.2" display is 320 x 240 pixels, landscape, (y=239 reserved for activity bar)
  //
  // label  origin   size      touch-target     
  // text    x,y      w,h      x,y      w,h   radius  color     function
  {  "+",   66,204,  36,30, { 30,180, 110,59},  4,  cTEXTCOLOR, timeZonePlus  },  // Up
  {  "-",  226,204,  36,30, {190,180, 110,59},  4,  cTEXTCOLOR, timeZoneMinus },  // Down
};
const int nTimeButtons = sizeof(timeButtons)/sizeof(Button);

void startViewScreen() {
  // one-time setup for static info on the display

  clearScreen();
  setFontSize(eFONTSMALLEST);
  txtClock[TITLE].print();

  setFontSize(eFONTGIANT);
  for (int ii=HOURS; ii<=SECONDS; ii++) {
    txtClock[ii].print();
  }

  setFontSize(eFONTSMALL);
  for (int ii=GMTDATE; ii<=NUMSATS; ii++) {
    txtClock[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALL);
  for (int ii=0; ii<nTimeButtons; ii++) {
    Button item = timeButtons[ii];
    tft.fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft.drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    #ifdef SHOW_TOUCH_TARGETS
    tft.drawRect(item.hitTarget.ul.x, item.hitTarget.ul.y,  // debug: draw outline around hit target
                 item.hitTarget.size.x, item.hitTarget.size.y, 
                 cBUTTONOUTLINE); 
    #endif

    tft.setCursor(item.x+item.w/2-7, item.y+item.h/2+5);
    tft.setTextColor(item.color);
    tft.print(item.text);
  }

  // debug: show centerline on display
  //tft.drawLine(tft.width()/2,0, tft.width()/2,tft.height(), cWARN); // debug
}

void updateView() {
/*
  +-----------------------------------------+
  |            Griduino GMT Clock           |
  |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
  |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
  |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
  |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
  |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
  |                                         |
  |         GMT Date: Apr 29, 2020          |
  |                                         |
  |              Temp:  101.7 F             |
  |                                         |
  +-------+                         +-------+
  | +1 hr |    Local: HH:MM:SS      | -1 hr |
  +-------+-------------------------+-------+
*/  
  // GMT Time
  char sHour[8], sMinute[8], sSeconds[8];
  snprintf(sHour,   sizeof(sHour), "%02d", GPS.hour);
  snprintf(sMinute, sizeof(sMinute), "%02d", GPS.minute);
  snprintf(sSeconds,sizeof(sSeconds), "%02d", GPS.seconds);

  Serial.print("GMT = ("); Serial.print(sHour);
  Serial.print("):(");     Serial.print(sMinute);
  Serial.print("):(");     Serial.print(sSeconds);
  Serial.println(")");
  
  setFontSize(eFONTGIANT);
  txtClock[HOURS].print(sHour);
  txtClock[MINUTES].print(sMinute);
  txtClock[SECONDS].print(sSeconds);
  txtClock[COLON2].dirty = true;
  txtClock[COLON2].print();

  setFontSize(eFONTSMALL);

  // GMT Date
  char sDate[16];                     // strlen("Jan 12, 2020 ") = 14
  getDate(sDate, sizeof(sDate));
  txtClock[GMTDATE].print(sDate);

  // Temperature
  float t = getTemperature();
  double intpart;
  double fractpart= modf(t, &intpart);

  int degr = (int) intpart;
  int frac = (int) fractpart*10;
  
  char sTemp[9];                      // strlen("123.4 F") = 7
  snprintf(sTemp, sizeof(sTemp), "%d.%d F", degr, frac);
  txtClock[DEGREES].print(sTemp);

  // Hours to add/subtract from GMT for local time
  char sign[2] = { 0, 0 };            // prepend a plus-sign when >=0
  sign[0] = (gTimeZone>=0) ? '+' : 0; // (don't need to add a minus-sign bc the print stmt does that for us)
  char sTimeZone[6];      // strlen("-10h") = 4
  snprintf(sTimeZone, sizeof(sTimeZone), "%s%dh", sign, gTimeZone);
  txtClock[TIMEZONE].print(sTimeZone);

  // Local Time
  char sTime[10];                     // strlen("01:23:45") = 8
  getTimeLocal(sTime, sizeof(sTime));
  txtClock[LOCALTIME].print(sTime);

  // Satellite Count
  char sBirds[4];                     // strlen("5#") = 2
  snprintf(sBirds, sizeof(sBirds), "%d#", gSatellites);
  // change colors by number of birds
  txtClock[NUMSATS].color = (gSatellites<1) ? cWARN : cTEXTFAINT;
  txtClock[NUMSATS].print(sBirds);
  //txtClock[NUMSATS].dump();         // debug
}

void clearScreen() {
  tft.fillScreen(cBACKGROUND);
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

//=========== time helpers =====================================
#define TIME_FOLDER  "/GMTclock"      // 8.3 names
#define TIME_FILE    TIME_FOLDER "/AddHours.cfg"
#define TIME_VERSION "v01"

void timeZonePlus() {
  gTimeZone++;
  if (gTimeZone > 12) {
    gTimeZone = -11;
  }
  updateView();
  Serial.print("Time zone changed to "); Serial.println(gTimeZone);
  SaveRestore myconfig = SaveRestore(TIME_FOLDER, TIME_FILE, TIME_VERSION, gTimeZone);
  myconfig.writeConfig();
}
void timeZoneMinus() {
  gTimeZone--;
  if (gTimeZone < -12) {
    gTimeZone = 11;
  }
  updateView();
  Serial.print("Time zone changed to "); Serial.println(gTimeZone);
  SaveRestore myconfig = SaveRestore(TIME_FOLDER, TIME_FILE, TIME_VERSION, gTimeZone);
  myconfig.writeConfig();
}

//=========== distance helpers =================================

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
  delay(200);                                   // is delay really needed?
  GPS.sendCommand(PMTK_SET_BAUD_57600);         // set baud rate to 57600
  delay(200);
  GPS.begin(57600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); // turn on RMC (recommended minimum) and GGA (fix data) including altitude

  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    // 1 Hz update rate
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ); // Once every 5 seconds update

  // ----- query GPS
  Serial.print("Sending command to query GPS Firmware version");
  Serial.println(PMTK_Q_RELEASE);     // Echo query to console
  GPS.sendCommand(PMTK_Q_RELEASE);    // Send query to GPS unit
                                      // expected reply: $PMTK705,AXN_2.10...
  // ----- init onboard LED
  // turn off the solid red led next to the USB connector
  pinMode(RED_LED, OUTPUT);           // diagnostics RED LED
  digitalWrite(RED_LED, LOW);         // this led defaults to "on" so turn it off
  
  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(SCREEN_ROTATION);   // 1=landscape (default is 0=portrait)
  clearScreen();

  // ----- init Feather M4 onboard lights (in case previous state or program left the lights on)
  pixel.begin();
  pixel.clear();                      // turn off NeoPixel
  digitalWrite(PIN_LED, LOW);         // turn off little red LED
  Serial.println("NeoPixel initialized and turned off");

  // ----- announce ourselves
  startSplashScreen();

  delay(4000);         // milliseconds

  // ----- init barometer/thermometer
  if (!baro.begin()) {
    Serial.println("Error, unable to initialize BMP388, check your wiring");

    #define RETRYLIMIT 10
    TextField txtError[] = {
      //        text                 x,y     color  
      {"Error!",                    12, 32,  cWARN},       // [0]
      {"Unable to init barometer",  12, 62,  cWARN},       // [1]
      {"Please check your wiring",  12, 92,  cWARN},       // [2]
      {"Retrying...",               12,152,  cWARN},       // [3]
      {"1",                        150,152,  cTEXTCOLOR, ALIGNRIGHT},  // [4]
      {"of 50",                    162,152,  cWARN},       // [5]
    };
    const int numErrorFields = sizeof(txtError)/sizeof(TextField);

    setFontSize(eFONTSMALL);
    clearScreen();
    for (int ii=0; ii<numErrorFields; ii++) {
      txtError[ii].print();
    }

    for (int ii=1; ii<=RETRYLIMIT; ii++) {
      txtError[4].print(ii);
      if (baro.begin()) {
        break;  // success, baro sensor finally initialized
      }
      delay(1000);
    }
  }

  // Set up BMP388 oversampling and filter initialization
  // baro.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);

  // ----- restore configuration settings
  SaveRestore myconfig = SaveRestore(TIME_FOLDER, TIME_FILE, TIME_VERSION);
  if (myconfig.readConfig()) {
    gTimeZone = myconfig.intSetting;
  }
  Serial.print("Time zone restored to "); Serial.println(gTimeZone);

  startViewScreen();
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTimeGPS = millis();
const int GPS_PROCESS_INTERVAL = 1000;  // milliseconds between updating the model's GPS data
uint32_t prevTimeTouch = millis();
const int TOUCH_PROCESS_INTERVAL = 5;   // milliseconds between polling for touches

void loop() {

  // if our timer or system millis() wrapped around, reset it
  if (prevTimeGPS > millis()) {
    prevTimeGPS = millis();
  }
  if (prevTimeTouch > millis()) {
    prevTimeTouch = millis();
  }

  GPS.read();   // if you can, read the GPS serial port every millisecond in an interrupt
                // this sketch reads the serial port continuously during idle time

  if (GPS.newNMEAreceived()) {
    // sentence received -- verify checksum, parse it
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

  // periodically, process and save the current GPS time
  if (millis() - prevTimeGPS > GPS_PROCESS_INTERVAL) {
    prevTimeGPS = millis();           // restart another interval

    processGPS();                     // update GMT time
    updateView();                     // update current screen
  }

  // if there's touchscreen input, handle it
  if (millis() - prevTimeTouch > TOUCH_PROCESS_INTERVAL) {
    prevTimeTouch = millis();         // start another interval
    Point touch;
    if (newScreenTap(&touch)) {

      #ifdef SHOW_TOUCH_TARGET
      tft.fillCircle(touch.x, touch.y, 3, cWARN);  // debug - show dot, radius=3
      #endif

      for (int ii=0; ii<nTimeButtons; ii++) {
        if (timeButtons[ii].hitTarget.contains( touch )) {
          timeButtons[ii].function(); // dispatch to timeZonePlus() or timeZoneMinus()
          Serial.print("Hit! target = "); Serial.println(ii);
        }
      }
    }
  }

  // make a small progress bar crawl along bottom edge
  // this gives a sense of how frequently the main loop is executing
  showActivityBar(tft.height()-1, ILI9341_RED, cBACKGROUND);
}
