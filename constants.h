#ifndef _GRIDUINO_CONSTANTS_H
#define _GRIDUINO_CONSTANTS_H

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "Griduino"
#define PROGRAM_VERSION "v0.9.8"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"


#define gScreenWidth 320    // pixels wide
#define gScreenHeight 240   // pixels high
                            // we use #define here instead of reading it from "tft.width()" because this
                            // screen layout is specifically designed for landscape orientation on 3.2" ILI9341

#define feetPerMeters 3.28084
#define mphPerKnots   1.15078

// ------- Select features ---------
//#define RUN_UNIT_TESTS              // comment out to save boot-up time

// ----- load/save configuration using SDRAM
//#define EXTERNAL_FLASH_USE_QSPI   // 2020-02-11 added by BarryH, since it seems to be missing from 
                                    // c:\Users\barry\AppData\Local\Arduino15\packages\adafruit\hardware\samd\1.5.7\variants\feather_m4\variant.h
#define CONFIG_FOLDER  "/Griduino"

// ----- color scheme
#define BACKGROUND     0x00A            // a little darker than ILI9341_NAVY
#define cBACKGROUND    0x00A            // a little darker than ILI9341_NAVY
#define cLABEL         ILI9341_GREEN
#define cVALUE         ILI9341_YELLOW
#define cHIGHLIGHT     ILI9341_WHITE
#define cBUTTONFILL    ILI9341_NAVY
#define cBUTTONOUTLINE ILI9341_CYAN
#define cBUTTONLABEL   ILI9341_YELLOW
#define cWARN          0xF844           // a little brighter than ILI9341_RED

#endif // _GRIDUINO_CONSTANTS_H
