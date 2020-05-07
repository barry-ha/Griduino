#ifndef _GRIDUINO_CONSTANTS_H
#define _GRIDUINO_CONSTANTS_H

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "Griduino"
#define PROGRAM_VERSION "v0.12"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"


#define gScreenWidth 320    // pixels wide
#define gScreenHeight 240   // pixels high
                            // we use #define here instead of reading it from "tft.width()" because this
                            // screen layout is specifically designed for landscape orientation on 3.2" ILI9341

#define feetPerMeters 3.28084
#define mphPerKnots   1.15078

// ------- Select features ---------
//#define RUN_UNIT_TESTS            // comment out to save boot-up time
//#define ECHO_GPS                  // use this to resend GPS sentences to IDE console for debug

// ----- load/save configuration using SDRAM
//#define EXTERNAL_FLASH_USE_QSPI   // 2020-02-11 added by BarryH, since it seems to be missing from 
                                    // c:\Users\barry\AppData\Local\Arduino15\packages\adafruit\hardware\samd\1.5.7\variants\feather_m4\variant.h
#define CONFIG_FOLDER  "/Griduino"

// ----- color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define BACKGROUND      0x00A             // a little darker than ILI9341_NAVY
#define cBACKGROUND     0x00A             // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cLABEL          ILI9341_GREEN
#define cVALUE          ILI9341_YELLOW
#define cHIGHLIGHT      ILI9341_WHITE
#define cBUTTONFILL     ILI9341_NAVY
#define cBUTTONOUTLINE  ILI9341_CYAN
#define cBUTTONLABEL    ILI9341_YELLOW
#define cCOMPASS        ILI9341_BLUE      // a little darker than cBUTTONOUTLINE
#define cWARN           0xF844            // brighter than ILI9341_RED but not pink

// ------------ typedef's
typedef struct {
  int x, y;
} Point;

typedef struct {
  int left, top;
  int width, height;
  char label[12];
} Rectangle;

typedef struct {
  char text[26];
  int x, y;
  uint16_t color;
} Label;

typedef void (*simpleFunction)();
typedef struct {
  char text[26];
  int x, y;
  int w, h;
  int radius;
  uint16_t color;
  simpleFunction function;
} Button;

#endif // _GRIDUINO_CONSTANTS_H
