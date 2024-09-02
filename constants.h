#pragma once   // Please format this file with clang before check-in to GitHub

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE "Griduino"
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
#define PROGRAM_VERSION "v1.14"
#else
#define PROGRAM_VERSION "v1.14.4"
#endif
#define HARDWARE_VERSION "Rev 12"  // Rev 4 | Rev 7 | Rev 12
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    "John KM7O"
#define PROGRAM_VERDATE  PROGRAM_VERSION ", " __DATE__
#define PROGRAM_COMPILED __DATE__ " " __TIME__
#define PROGRAM_FILE     __FILE__
#define PROGRAM_GITHUB   "https://github.com/barry-ha/Griduino"

// ------- Select testing features ---------
// #define SCOPE_OUTPUT  A0            // use this for performance measurements with oscilloscope
// #define ECHO_GPS                    // use this to see GPS detailed info on IDE console for debug
// #define SHOW_SCREEN_BORDER          // use this to outline the screen's displayable area
// #define SHOW_IGNORED_PRESSURE       // use this to see barometric pressure readings that are out of range and therefore ignored

// ------- TFT screen definitions ---------
#define gScreenWidth  320   // screen pixels wide
#define gScreenHeight 240   // screen pixels high                                                        \
                            // we use #define here instead of reading it from "tft.width()" because this \
                            // screen layout is specifically designed for landscape orientation on 3.2" ILI9341

#define gBoxWidth  180   // grid square width as shown on display, pixels
#define gBoxHeight 160   // grid square height as shown on display, pixels

// ------- Physical constants ---------
const float gridWidthDegrees  = 2.0;   // horiz E-W size of one grid square, degrees
const float gridHeightDegrees = 1.0;   // vert N-S size of one grid square, degrees

const double minLong = gridWidthDegrees / gBoxWidth;     // longitude degrees from one pixel to the next (minimum visible movement)
const double minLat  = gridHeightDegrees / gBoxHeight;   // latitude degrees from one pixel to the next

#define feetPerMeters         (3.28084)       // altitude conversion
#define mphPerKnots           (1.15078)       // speed conversion
#define mphPerMetersPerSecond (0.44704)       // speed conversion
const double degreesPerRadian = 57.2957795;   // conversion factor = (360 degrees)/(2 pi radians)

#define SECS_PER_1MIN  ((time_t)(60UL))
#define SECS_PER_5MIN  ((time_t)(300UL))
#define SECS_PER_10MIN ((time_t)(600UL))
#define SECS_PER_15MIN ((time_t)(900UL))

#define DEFAULT_SEALEVEL_PASCALS (101740.0)
#define DEFAULT_SEALEVEL_HPA     (1017.40)

#define FIRST_RELEASE_YEAR  (2019)   // this date can help filter out bogus GPS timestamps
#define FIRST_RELEASE_MONTH (12)
#define FIRST_RELEASE_DAY   (19)

// ----- load/save configuration using SDRAM
// #define EXTERNAL_FLASH_USE_QSPI     // 2020-02-11 added by BarryH, since it seems to be missing from
// c:\Users\barry\AppData\Local\Arduino15\packages\adafruit\hardware\samd\1.5.7\variants\feather_m4\variant.h
#define CONFIG_FOLDER "/Griduino"

// ----- alias names for SCREEN_ROTATION
enum Rotation {
  PORTRAIT          = 0,   //   0 degrees = portrait
  LANDSCAPE         = 1,   //  90 degrees = landscape
  FLIPPED_PORTRAIT  = 2,   // 180 degrees = portrait flipped 180-degrees
  FLIPPED_LANDSCAPE = 3,   // 270 degrees = landscape flipped 180-degrees
};

// ----- alias names for fGetDataSource()
enum {
  eGPSRECEIVER = 1,   // use the GPS receiver hardware
  eGPSSIMULATOR,      // use a GPS simulator
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
const int MAXBRIGHT = 255;   // = 100% brightness = maximum allowed on individual LED
const int BRIGHT    = 32;    // = tolerably bright indoors
const int HALFBR    = 20;    // = half of tolerably bright
const int OFF       = 0;     // = turned off

// ------- Coin Battery good/bad thresholds ---------
const float GOOD_BATTERY_MINIMUM    = (2.25);   // green, if above this voltage
const float WARNING_BATTERY_MINIMUM = (2.00);   // yellow, if above this voltage
const float BAD_BATTERY_MAXIMUM     = (2.00);   // red, if below this voltage

// ----- Griduino color scheme
// RGB 565 true color: https://chrishewett.com/blog/true-rgb565-colour-picker/
#define BACKGROUND     0x00A            // a little darker than ILI9341_NAVY
#define cBACKGROUND    0x00A            // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cGRIDNAME      ILI9341_GREEN    //
#define cLABEL         ILI9341_GREEN    //
#define cDISTANCE      ILI9341_YELLOW   //
#define cVALUE         ILI9341_YELLOW   // 255, 255, 0
#define cVALUEFAINT    0xbdc0           // darker than cVALUE
#define cDISABLED      0x7bee           // 125, 125, 115 = gray for disabled screen item
#define cHIGHLIGHT     ILI9341_WHITE    //
#define cBUTTONFILL    ILI9341_NAVY     //
#define cBUTTONOUTLINE 0x0514           // was ILI9341_CYAN
#define cTITLE         ILI9341_GREEN    //
#define cTEXTCOLOR     0x67FF           // rgb(102,255,255) = hsl(180,100,70%)
#define cCYAN          ILI9341_CYAN     // rgb(0,255,255) = hsl(180,100,50%)
#define cFAINT         0x0555           // rgb(0,168,168) = hsl(180,100,33%) = blue, between CYAN and DARKCYAN
#define cFAINTER       0x04B2           // rgb(0,128,128) = hsl(180,100,29%) = blue, between CYAN and DARKCYAN
#define cBOXDEGREES    0x0410           // rgb(0,128,128) = hsl(180,100,25%) = blue, between CYAN and DARKCYAN
#define cBUTTONLABEL   ILI9341_YELLOW   //
#define cCOMPASS       ILI9341_BLUE     // a little darker than cBUTTONOUTLINE
#define cSTATUS        0xFC10           // 255, 128, 128 = lavender
#define cWARN          0xF844           // brighter than ILI9341_RED but not pink
#define cTOUCHTARGET   ILI9341_RED      // outline touch-sensitive areas

// plot vehicle and breadcrumb trail
#define cBREADCRUMB ILI9341_CYAN   //
#define cVEHICLE    0xef7d         // light gray (white is too bright)

// barometric pressure graph
#define cSCALECOLOR ILI9341_DARKGREEN   // pressure graph, I tried yellow but it's too bright
#define cGRAPHCOLOR ILI9341_WHITE       // graphed line of baro pressure

#define cDARKPURPLE 0x2809   // debug: 2809 = very dark purple
#define cDARKRED    0x4000   // debug: 4000 = very dark red
#define cDARKBROWN  0x49A0   // debug: 49A0 = dark orange
#define cDARKGREEN  0x01A0   // debug: #0x01A0 = very dark green

// ------------ typedef's
struct Point {
  int x, y;
};
struct PointGPS {
public:
  double lat, lng;
};

#include <TimeLib.h>   // time_t=seconds since Jan 1, 1970, https://github.com/PaulStoffregen/Time
class BaroReading {
public:
  float pressure;   // in millibars, from BMP388 sensor
  time_t time;      // in GMT, from realtime clock
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
    if ((ul.x <= touch.x) && (touch.x <= ul.x + size.x) && (ul.y <= touch.y) && (touch.y <= ul.y + size.y)) {
      return true;
    } else {
      return false;
    }
  }
};

struct Route {   // screen coordinates
  uint16_t x, y;
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
  char text[26];       // one line of text, centered
  int x, y;            // fillRoundRect ul corner
  int w, h;            // fillRoundRect size
  Rect hitTarget;      // touch-sensitive area
  int radius;          // radio button size
  uint16_t color;      // text color
  int functionIndex;   // button identifier
};

// Breadcrumb record types
#define rGPS                 "GPS"
#define rPOWERUP             "PUP"
#define rPOWERDOWN           "PDN"
#define rFIRSTVALIDTIME      "TIM"
#define rLOSSOFSIGNAL        "LOS"
#define rACQUISITIONOFSIGNAL "AOS"
#define rCOINBATTERYVOLTAGE  "BAT"
#define rRESET               "\0\0\0"
#define rVALIDATE            rGPS rPOWERUP rPOWERDOWN rFIRSTVALIDTIME rLOSSOFSIGNAL rACQUISITIONOFSIGNAL rCOINBATTERYVOLTAGE

// Breadcrumb data definition for circular buffer
class Location {
public:
  char recordType[4];      // GPS, power-up, first valid time, etc
  PointGPS loc;            // has-a lat/long, degrees
  time_t timestamp;        // has-a GMT time
  uint8_t numSatellites;   // number of satellites in use (not the same as in view)
  float speed;             // current speed over ground in MPH (or coin battery voltage)
  float direction;         // direction of travel, degrees from true north
  float altitude;          // altitude, meters above MSL
public:
  void reset() {
    recordType[0] = 0;
    loc.lat = loc.lng = 0.0;
    timestamp         = 0;
    numSatellites     = 0;
    speed             = 0.0;
    direction = altitude = 0.0;
  }
  bool isEmpty() const {
    // we take advantage of the fact that all unused records
    // will have reset their recordType field to zeroes, ie, rRESET
    return (strlen(recordType) == 0);
  }

  static bool isValidRecordType(const char *rec) {
    // check for "should not happen" situations
    if (strstr(rVALIDATE, rec)) {
      return true;
    } else {
      return false;
    }
  }

  bool isGPS() const {
    return (strncmp(recordType, rGPS, sizeof(recordType)) == 0);
  }

  bool isPUP() const {
    return (strncmp(recordType, rPOWERUP, sizeof(recordType)) == 0);
  }

  bool isFirstValidTime() const {
    return (strncmp(recordType, rFIRSTVALIDTIME, sizeof(recordType)) == 0);
  }

  bool isLossOfSignal() const {
    return (strncmp(recordType, rLOSSOFSIGNAL, sizeof(recordType)) == 0);
  }

  bool isAcquisitionOfSignal() const {
    return (strncmp(recordType, rACQUISITIONOFSIGNAL, sizeof(recordType)) == 0);
  }

  bool isCoinBatteryVoltage() const {
    return (strncmp(recordType, rCOINBATTERYVOLTAGE, sizeof(recordType)) == 0);
  }

  // print ourself - a sanity check
  void printLocation(const char *comment = NULL) {   // debug
    // note: must use Serial.print (not logger) because the logger.h cannot be included at this level
    Serial.println(". Rec, ___Date___ __Time__, (__Lat__, __Long__), Alt, Spd, Dir, Sats");

    char out[128];
    snprintf(out, sizeof(out), ". %s , ", recordType);
    Serial.print(out);

    // timestamp
    TimeElements time;   // https://github.com/PaulStoffregen/Time
    breakTime(timestamp, time);
    snprintf(out, sizeof(out), "%02d-%02d-%02d %02d:%02d:%02d, ",
             time.Year + 1970, time.Month, time.Day, time.Hour, time.Minute, time.Second);
    Serial.print(out);

    // lat/long
    Serial.print("(");
    Serial.print(loc.lat, 4);
    Serial.print(", ");
    Serial.print(loc.lng, 4);
    Serial.print("), ");

    // alt, speed dir sats
    Serial.print(altitude, 1);   // meters
    Serial.print(", ");
    Serial.print(speed, 1);   // mph
    Serial.print(", ");
    Serial.print(direction, 1);   // degrees
    Serial.print(", ");
    Serial.print(numSatellites);
    Serial.println();

    if (comment) {
      Serial.println(comment);
    }
  }
};
