#pragma once

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "Griduino"
#define PROGRAM_VERSION "v0.22"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

#define SCREEN_ROTATION 1           // 1=landscape, 3=landscape 180-degrees
#define gScreenWidth 320            // screen pixels wide
#define gScreenHeight 240           // screen pixels high
                                    // we use #define here instead of reading it from "tft.width()" because this
                                    // screen layout is specifically designed for landscape orientation on 3.2" ILI9341

#define gBoxWidth 180               // grid square width as shown on display, pixels
#define gBoxHeight 160              // grid square height as shown on display, pixels

const float gridWidthDegrees = 2.0; // size of one grid square, degrees
const float gridHeightDegrees = 1.0;

#define feetPerMeters 3.28084       // altitude conversion
#define mphPerKnots   1.15078
const double degreesPerRadian = 57.2957795; // conversion factor = (360 degrees)/(2 pi radians)

// ------- Select testing features ---------
#define RUN_UNIT_TESTS            // comment out to save boot-up time
//#define USE_SIMULATED_GPS         // comment out to use real GPS, or else it simulates driving around (see model.cpp)
//#define ECHO_GPS                  // use this to see GPS detailed info on IDE console for debug
//#define ECHO_GPS_SENTENCE         // use this to see once-per-second GPS sentences
//#define SHOW_TOUCH_TARGETS        // use this to outline touchscreen sensitive buttons
//#define SHOW_SCREEN_BORDER        // use this to outline the screen's displayable area

// ----- load/save configuration using SDRAM
//#define EXTERNAL_FLASH_USE_QSPI   // 2020-02-11 added by BarryH, since it seems to be missing from 
                                    // c:\Users\barry\AppData\Local\Arduino15\packages\adafruit\hardware\samd\1.5.7\variants\feather_m4\variant.h
#define CONFIG_FOLDER  "/Griduino"

// ----- alias names for fGetDataSource()
enum {
  eGPSRECEIVER = 1,                 // use the GPS receiver hardware
  eGPSSIMULATOR,                    // use a GPS simulator
};

// ----- alias names for setFontSize()
enum {
  eFONTGIANT    = 36,
  eFONTBIG      = 24,
  eFONTSMALL    = 12,
  eFONTSMALLEST = 9,
  eFONTSYSTEM   = 0
};

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

// ---------- Feather's onboard lights
//efine PIN_NEOPIXEL 8      // already defined in Feather's board variant.h
//efine PIN_LED 13          // already defined in Feather's board variant.h

#define NUMPIXELS 1         // Feather M4 has one NeoPixel on board

const int MAXBRIGHT = 255;    // = 100% brightness = maximum allowed on individual LED
const int BRIGHT = 32;        // = tolerably bright indoors
const int HALFBR = 20;        // = half of tolerably bright
const int OFF = 0;            // = turned off

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
#define cTEXTCOLOR      ILI9341_CYAN      // 0, 255, 255
#define cTEXTFAINT      0x0514            // 0, 160, 160 = blue, between CYAN and DARKCYAN
//define cBOXDEGREES    0x0514            // 0, 160, 160 = blue, between CYAN and DARKCYAN
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
} ;

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
} ;

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
