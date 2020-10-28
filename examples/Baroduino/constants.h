#pragma once

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "Griduino"
#define PROGRAM_VERSION "v0.26"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ------- Select testing features ---------
//#define RUN_UNIT_TESTS              // comment out to save boot-up time
//#define USE_SIMULATED_GPS           // comment out to use real GPS, or else it simulates driving around (see model_gps.h)
//#define ECHO_GPS                    // use this to see GPS detailed info on IDE console for debug
//#define ECHO_GPS_SENTENCE           // use this to see once-per-second GPS sentences
//#define SHOW_TOUCH_TARGETS          // use this to outline touchscreen sensitive buttons
//#define SHOW_SCREEN_BORDER          // use this to outline the screen's displayable area
//#define SHOW_SCREEN_CENTERLINE      // use this visual aid to help layout the screen

// ------- TFT screen definitions ---------
#define gScreenWidth 320              // screen pixels wide
#define gScreenHeight 240             // screen pixels high
                                      // we use #define here instead of reading it from "tft.width()" because this
                                      // screen layout is specifically designed for landscape orientation on 3.2" ILI9341

#define gBoxWidth 180                 // grid square width as shown on display, pixels
#define gBoxHeight 160                // grid square height as shown on display, pixels


// ------- Physical constants ---------
const float gridWidthDegrees = 2.0;   // size of one grid square, degrees
const float gridHeightDegrees = 1.0;

#define feetPerMeters 3.28084         // altitude conversion
#define mphPerKnots   1.15078
const double degreesPerRadian = 57.2957795; // conversion factor = (360 degrees)/(2 pi radians)

#define SECS_PER_5MIN  ((time_t)(300UL))
#define SECS_PER_15MIN ((time_t)(900UL))

// ----- load/save configuration using SDRAM
//#define EXTERNAL_FLASH_USE_QSPI     // 2020-02-11 added by BarryH, since it seems to be missing from 
                                      // c:\Users\barry\AppData\Local\Arduino15\packages\adafruit\hardware\samd\1.5.7\variants\feather_m4\variant.h
#define CONFIG_FOLDER  "/Griduino"

// ----- alias names for SCREEN_ROTATION
#define SCREEN_ROTATION 1             // 1=landscape, 3=landscape 180-degrees
enum {
  eSCREEN_ROTATE_0 = 1,               // 1=landscape
  eSCREEN_ROTATE_180 = 3,             // 3=landscape 180-degrees
};

// ----- alias names for fGetDataSource()
enum {
  eGPSRECEIVER = 1,                   // use the GPS receiver hardware
  eGPSSIMULATOR,                      // use a GPS simulator
};

// ----- alias names for setFontSize()
enum {
  eFONTGIANT    = 36,
  eFONTBIG      = 24,
  eFONTSMALL    = 12,
  eFONTSMALLEST = 9,
  eFONTSYSTEM   = 0,
  eFONTUNSPEC   = -1,
};

// ------- NeoPixel brightness ---------
const int MAXBRIGHT = 255;            // = 100% brightness = maximum allowed on individual LED
const int BRIGHT = 32;                // = tolerably bright indoors
const int HALFBR = 20;                // = half of tolerably bright
const int OFF = 0;                    // = turned off

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define BACKGROUND      0x00A           // a little darker than ILI9341_NAVY
#define cBACKGROUND     0x00A           // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cGRIDNAME       ILI9341_GREEN
#define cLABEL          ILI9341_GREEN
#define cDISTANCE       ILI9341_YELLOW
#define cVALUE          ILI9341_YELLOW  // 255, 255, 0
#define cVALUEFAINT     0xbdc0          // darker than cVALUE
#define cHIGHLIGHT      ILI9341_WHITE
#define cBUTTONFILL     ILI9341_NAVY
#define cBUTTONOUTLINE  ILI9341_CYAN
#define cBREADCRUMB     ILI9341_CYAN
#define cTEXTCOLOR      ILI9341_CYAN    // 0, 255, 255
#define cTEXTFAINT      0x0514          // 0, 160, 160 = blue, between CYAN and DARKCYAN
//define cBOXDEGREES    0x0514          // 0, 160, 160 = blue, between CYAN and DARKCYAN
#define cBOXDEGREES     0x0410          // 0, 128, 128 = blue, between CYAN and DARKCYAN
#define cBUTTONLABEL    ILI9341_YELLOW
#define cCOMPASS        ILI9341_BLUE    // a little darker than cBUTTONOUTLINE
#define cWARN           0xF844          // brighter than ILI9341_RED but not pink

// ------------ typedef's
struct Point {
  int x, y;
};
struct PointGPS {
  public:
    double lat, lng;
};

class BaroReading {
  public:
    float pressure;                   // in millibars, from BMP388 sensor
    time_t time;                      // in GMT, from realtime clock 
};

struct TwoPoints {
  int x1, y1;
  int x2, y2;
  uint16_t color;
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

struct Label {
  char text[26];
  int x, y;
  uint16_t color;
};

typedef void (*simpleFunction)();
struct Button {
  char text[26];
  int x, y;
  int w, h;
  int radius;
  uint16_t color;
  simpleFunction function;
};

struct TimeButton {
  // TimeButton is like Button, but has a larger specifiable hit target
  // This allows very small buttons with a large sensitive area to make it easy to press
  char text[26];
  int x, y;
  int w, h;
  Rect hitTarget;
  int radius;
  uint16_t color;
  simpleFunction function;
};

struct FunctionButton {
  // FunctionButton is like Button, but has a larger specifiable hit target
  // It's also like TimeButton, but specifies the function by enum, rather than pointer to function
  // and this allows its usage in classes derived from "class View"
  char text[26];
  int x, y;
  int w, h;
  Rect hitTarget;
  int radius;
  uint16_t color;
  int functionIndex;
};

class Location {
  public:
    PointGPS loc;                     // has-a lat/long, degrees
    int hh, mm, ss;                   // has-a GMT time
    void reset() {
      loc.lat = loc.lng = 0.0;
      hh = mm = ss = 0;
    }
    bool isEmpty() {
      return (loc.lat==0.0 && loc.lng==0.0);
    }
};
