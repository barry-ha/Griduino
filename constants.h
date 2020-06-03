#ifndef _GRIDUINO_CONSTANTS_H
#define _GRIDUINO_CONSTANTS_H

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "Griduino"
#define PROGRAM_VERSION "v0.16"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"

#define SCREEN_ROTATION 1   // 1=landscape, 3=landscape 180-degrees
#define gScreenWidth 320    // screen pixels wide
#define gScreenHeight 240   // screen pixels high
                            // we use #define here instead of reading it from "tft.width()" because this
                            // screen layout is specifically designed for landscape orientation on 3.2" ILI9341

#define gBoxWidth 180       // grid square width as shown on display, pixels
#define gBoxHeight 160      // grid square height as shown on display, pixels

const float gridWidthDegrees = 2.0;   // size of one grid square, degrees
const float gridHeightDegrees = 1.0;

#define feetPerMeters 3.28084
#define mphPerKnots   1.15078
const double degreesPerRadian = 57.2957795;

// ------- Select features ---------
//#define RUN_UNIT_TESTS            // comment out to save boot-up time
//#define USE_SIMULATED_GPS         // comment out to use real GPS, or else it simulates driving around (see model.cpp)
//#define ECHO_GPS                  // use this to see GPS detailed info on IDE console for debug
//#define ECHO_GPS_SENTENCE         // use this to see once-per-second GPS sentences

// ----- load/save configuration using SDRAM
//#define EXTERNAL_FLASH_USE_QSPI   // 2020-02-11 added by BarryH, since it seems to be missing from 
                                    // c:\Users\barry\AppData\Local\Arduino15\packages\adafruit\hardware\samd\1.5.7\variants\feather_m4\variant.h
#define CONFIG_FOLDER  "/Griduino"

// ----- color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define BACKGROUND      0x00A             // a little darker than ILI9341_NAVY
#define cBACKGROUND     0x00A             // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cGRIDNAME       ILI9341_GREEN
#define cLABEL          ILI9341_GREEN
#define cDISTANCE       ILI9341_YELLOW
#define cVALUE          ILI9341_YELLOW
#define cHIGHLIGHT      ILI9341_WHITE
#define cBUTTONFILL     ILI9341_NAVY
#define cBUTTONOUTLINE  ILI9341_CYAN
#define cBREADCRUMB     ILI9341_CYAN
//efine cBOXDEGREES     0x0514            // 0, 160, 160 = blue, between CYAN and DARKCYAN
#define cBOXDEGREES     0x0410            // 0, 128, 128 = blue, between CYAN and DARKCYAN
#define cBUTTONLABEL    ILI9341_YELLOW
#define cCOMPASS        ILI9341_BLUE      // a little darker than cBUTTONOUTLINE
#define cWARN           0xF844            // brighter than ILI9341_RED but not pink

// ------------ typedef's
struct Point {
  int x, y;
};
struct PointGPS{
  public:
    double lat, lng;
};

struct Rectangle {
  int left, top;
  int width, height;
  char label[12];
};
struct Label {
  char text[26];
  int x, y;
  uint16_t color;
};

typedef void (*simpleFunction)();
typedef struct {
  char text[26];
  int x, y;
  int w, h;
  int radius;
  uint16_t color;
  simpleFunction function;
} Button;

class Location {
  public:
    PointGPS loc;       // has-a lat/long, degrees
    int hh, mm, ss;     // has-a GMT time
    void reset() {
      loc.lat = loc.lng = 0.0;
      hh = mm = ss = 0;
    }
    bool isEmpty() {
      return (loc.lat==0.0 && loc.lng==0.0);
    }
};

#endif // _GRIDUINO_CONSTANTS_H
