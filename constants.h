#ifndef _GRIDUINO_CONSTANTS_H
#define _GRIDUINO_CONSTANTS_H

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "Griduino"
#define PROGRAM_VERSION "v0.9.4"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"


#define gScreenWidth 320    // pixels wide
#define gScreenHeight 240   // pixels hight
                            // we use #define here instead of reading it from "tft.width()" because this
                            // screen layout is specifically designed for 3.2" ILI9341 in landscape orientation

#define feetPerMeters 3.28084
#define mphPerKnots   1.15078

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
