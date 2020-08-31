/*
  Baroduino -- demonstrate BMP388 barometric sensor

  Date:     2019-20-18 created from example by John KM7O
            2020-03-05 replaced physical button design with touchscreen
            2020-05-12 updated TouchScreen code
            2020-05-18 added NeoPixel control to illustrate technique
            2020-08-24 start rewriting user interface
            2020-08-27 save units (english/metric) in nonvolatile RAM

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  What can you do with a graphing barometer that knows the exact time of day?
            This program timestamps each reading and stores it for later.
            It graphs the most recent 3 days. 
            (Todo) It saves the readings in non-volatile memory and re-displays them on power-up.
            The RTC (realtime clock) is updated from the GPS satellite network.

            +-----------------------------------+
            |           29.97 inHg              |
            | 30.5 +--------------------------+ | <- yTop
            |      |        |        |        | |
            |      |        |        |        | |
            |      |        |        |        | |
            | 30.0 +  -  -  -  -  -  -  -  -  + | <- yMid
            |      |        |        |        | |
            |      |        |        |        | |
            |      |        |        |        | |
            | 29.5 +--------------------------+ | <- yBot
            |         8/22     8/23     Today   |
            +------:--------:--------:--------:-+
                   xDay1    xDay2    xDay3    xRight

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)     https://www.adafruit.com/product/3857
         2. Adafruit 3.2" TFT color LCD display ILI-9341    https://www.adafruit.com/product/1743
         3. Adafruit Ultimate GPS                           https://www.adafruit.com/product/746
         4. Adafruit BMP388 Barometric Pressure             https://www.adafruit.com/product/3966
*/

#include <Wire.h>
#include "SPI.h"                    // Serial Peripheral Interface
#include "Adafruit_GFX.h"           // Core graphics display library
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "Adafruit_GPS.h"           // Ultimate GPS library
#include "TouchScreen.h"            // Touchscreen built in to 3.2" Adafruit TFT display
#include "Adafruit_BMP3XX.h"        // Precision barometric and temperature sensor
#include "constants.h"              // Griduino constants, colors, typedefs
#include "save_restore.h"           // save/restore configuration data to SDRAM
#include "TextField.h"              // Optimize TFT display text for proportional fonts
#include "Adafruit_NeoPixel.h"
#include "TimeLib.h"                // BorisNeubert / Time (forked from PaulStoffregen / Time)

// ------- Identity for splash screen and console --------
#define BAROGRAPH_TITLE "Barograph"

//--------------CONFIG--------------
//float elevCorr = 4241;  // elevation correction in Pa, 
// use difference between altimeter setting and station pressure: https://www.weather.gov/epz/wxcalc_altimetersetting

float elevCorr = 0;
float maxP = 104000;          // in Pa
float minP = 98000;           // in Pa

enum units { eMetric, eEnglish };
int gUnits = eEnglish;         // units on startup: 0=english=inches mercury, 1=metric=millibars

// ========== forward reference ================================
int loadConfigUnits();
void saveConfigUnits();

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see constants.h

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- neopixel
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

const uint32_t colorRed    = pixel.Color(HALFBR, OFF,    OFF);
const uint32_t colorGreen  = pixel.Color(OFF,    HALFBR, OFF);
const uint32_t colorBlue   = pixel.Color(OFF,    OFF,    BRIGHT);
const uint32_t colorPurple = pixel.Color(HALFBR, OFF,    HALFBR);

// ---------- extern
bool newScreenTap(Point* pPoint, int orientation);  // Touch.cpp
uint16_t myPressure(void);                          // Touch.cpp
//bool TouchScreen::isTouching(void);               // Touch.cpp
void mapTouchToScreen(TSPoint touch, Point* screen, int orientation);

// ---------- Barometric and Temperature Sensor
Adafruit_BMP3XX baro(BMP_CS); // hardware SPI

// ---------- GPS ----------
/* "Ultimate GPS" pin wiring is connected to a dedicated hardware serial port
    available on an Arduino Mega, Arduino Feather and others.
*/

// Hardware serial port for GPS
Adafruit_GPS GPS(&Serial1);

// ------------ typedef's
class Reading {
  public:
    float pressure;           // in millibars, from BMP388 sensor
    int hh, mm, ss;           // in GMT, from realtime clock
};

// ------------ definitions
const int howLongToWait = 8;  // max number of seconds at startup waiting for Serial port to console

#define FEET_PER_METER 3.28084
#define SEA_LEVEL_PRESSURE_HPA (1013.25)

#define MILLIBARS_PER_INCHES_MERCURY (0.02953)
#define BARS_PER_INCHES_MERCURY      (0.033864)
#define PASCALS_PER_INCHES_MERCURY   (3386.4)

// ----- color scheme -----
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cSCALECOLOR     ILI9341_DARKGREEN // tried ILI9341_YELLOW but it's too bright
#define cGRAPHCOLOR     ILI9341_WHITE     // graphed line of baro pressure
#define cICON           ILI9341_CYAN
#define cTITLE          ILI9341_GREEN     
#define cWARN           0xF844            // brighter than ILI9341_RED but not pink
#define cSINKING        0xF882            // highlight rapidly sinking barometric pressure

// ------------ global barometric data
float inchesHg;
float gPressure;
float hPa;
float feet;
float tempF;

// Old:
//    144 steps at 20 minute refresh time is a 2880 minute (48 hr) graph with 20 minute resolution.
//    with 2px per step, we get 10 minutes/px, and 288px per 48 hrs, leaving some space for graph labels
// New:
//    288px wide graph ==> 96 px/day ==> 4px/hour
//    
const int maxReadings = 144;
Reading pressureStack[maxReadings] = {};    // array to hold pressure data, fill with zeros
const int lastIndex = maxReadings - 1;      // index to the last element in pressure array

// ======== barometer and temperature helpers ==================
void getBaroData() {
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

void rememberPressure( float pressure ) {
  
  // shift existing stack to the left
  for (int ii = 0; ii < lastIndex; ii++) {
    pressureStack[ii] = pressureStack[ii + 1];
  }
  
  // and put the latest pressure onto the stack
  pressureStack[lastIndex].pressure = pressure;
  //pressureStack[lastIndex].hh = GPS.hour;     // todo
  //pressureStack[lastIndex].mm = GPS.minute;   // todo
  //pressureStack[lastIndex].ss = GPS.seconds;  // todo
}

void dumpPressureHistory() {            // debug
  Serial.print("Pressure history stack, non-zero values [line "); Serial.print(__LINE__); Serial.println("]");
  for (int ii=0; ii<maxReadings; ii++) {
    Reading item = pressureStack[ii];
    if (item.pressure > 0) {
      Serial.print("Stack[");
      Serial.print(ii);
      Serial.print("] = ");
      Serial.print(pressureStack[ii].pressure);
      Serial.println();
    }
  }
  return;
}

#ifdef RUN_UNIT_TESTS
void initTestPressureHistory() {
  // add test data:
  //      sine wave from 990 to 1030 hPa, with period of one day
  //      y = 1010.0hPa + 20*sin(w)   where 'w' goes from 0..2pi in the desired period
  int numTestData = lastIndex/5;      // add this many entries of test data
  float offsetPa = 1010*100;          // readings are saved in Pa (not hPa)
  float amplitude = 20*100;           // readings are saved in Pa (not hPa)
  for (int ii=0; ii<numTestData; ii++) {
    float w = 2.0 * PI * ii / numTestData;
    float fakePressure = offsetPa + amplitude * sin( w );
    rememberPressure( fakePressure );

    Serial.print(ii);                 // debug
    Serial.print(". pressure(");
    Serial.print(fakePressure,1);
    Serial.println(")");
  }
}
#endif // RUN_UNIT_TESTS

// ========== font management helpers ==========================
/* Using fonts: https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts

  "Fonts" folder is inside \Documents\User\Arduino\libraries\Adafruit_GFX_Library\fonts
*/
#include "Fonts/FreeSans18pt7b.h"       // eFONTGIANT    36 pt (see constants.h)
#include "Fonts/FreeSansBold24pt7b.h"   // eFONTBIG      24 pt
#include "Fonts/FreeSans12pt7b.h"       // eFONTSMALL    12 pt
#include "Fonts/FreeSans9pt7b.h"        // eFONTSMALLEST  9 pt
// (built-in)                           // eFONTSYSTEM    8 pt

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
#define yRow1   54                // program title: "Barograph"
#define yRow2   yRow1 + 28        // program version
#define yRow3   yRow2 + 48        // author line 1
#define yRow4   yRow3 + 32        // author line 2

TextField txtSplash[] = {
  //        text               x,y       color  
  {BAROGRAPH_TITLE,  -1,yRow1,  cTEXTCOLOR, ALIGNCENTER}, // [0] program title, centered
  {PROGRAM_VERSION,  -1,yRow2,  cLABEL,     ALIGNCENTER}, // [1] normal size text, centered
  {PROGRAM_LINE1,    -1,yRow3,  cLABEL,     ALIGNCENTER}, // [2] credits line 1, centered
  {PROGRAM_LINE2,    -1,yRow4,  cLABEL,     ALIGNCENTER}, // [3] credits line 2, centered
  {"Compiled " PROGRAM_COMPILED,       
                     -1,228,    cTEXTCOLOR, ALIGNCENTER}, // [4] "Compiled", bottom row
};
const int numSplashFields = sizeof(txtSplash)/sizeof(TextField);

void startSplashScreen() {
  clearScreen();                                    // clear screen
  txtSplash[0].setBackground(cBACKGROUND);          // set background for all TextFields
  TextField::setTextDirty( txtSplash, numSplashFields ); // make sure all fields are updated

  setFontSize(12);
  for (int ii=0; ii<4; ii++) {
    txtSplash[ii].print();
  }

  setFontSize(9);
  for (int ii=4; ii<numSplashFields; ii++) {
    txtSplash[ii].print();
  }
}

// ========== graph screen layout ==============================
const int graphHeight = 160;    // in pixels
const int pixelsPerHour = 4;    // 4 px/hr graph
//nst int pixelsPerDay = 72;    // 3 px/hr * 24 hr/day = 72 px/day
const int pixelsPerDay = pixelsPerHour * 24;  // 4 px/hr * 24 hr/day = 96 px/day

const int MARGIN = 6;           // reserve an outer blank margin on all sides

// to center the graph: xDay1 = [(total screen width) - (reserved margins) - (graph width)]/2 + margin
//                      xDay1 = [(320 px) - (2*6 px) - (3*96 px)]/2 + 6
//                      xDay1 = 16
const int xDay1 = MARGIN+10;    // pixels
const int xDay2 = xDay1 + pixelsPerDay;
const int xDay3 = xDay2 + pixelsPerDay;

const int xRight = xDay3 + pixelsPerDay;

const int TEXTHEIGHT = 16;      // text line spacing, pixels
const int DESCENDERS = 6;       // proportional font descenders may go 6 pixels below baseline
const int yBot = gScreenHeight - MARGIN - DESCENDERS - TEXTHEIGHT;

// ========== screen helpers ===================================
void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

enum { eTitle, valPressure, unitPressure };
const int xIndent = 12;         // in pixels, text on main screen
const int yText1 = MARGIN+12;   // in pixels, top row, main screen
const int yText2 = yText1 + 28;
TextField txtReading[] = {
  TextField{ BAROGRAPH_TITLE, xIndent,yText1, cTITLE, ALIGNCENTER }, // [eTitle]
  TextField{ "30.00",  xIndent+150,yText2,  ILI9341_WHITE, ALIGNRIGHT },  // [valPressure]
  TextField{ "inHg",   xIndent+168,yText2,  ILI9341_WHITE, ALIGNLEFT  },  // [unitPressure]
};
const int numReadings = sizeof(txtReading) / sizeof(txtReading[0]);

void showReadings(int units) {
  // todo: is argument 'units' unused? if so, eliminate
  clearScreen();
  txtSplash[0].setBackground(cBACKGROUND);          // set background for all TextFields
  TextField::setTextDirty( txtReading, numReadings ); // make sure all fields are updated

  printPressure();
  tickMarks(3, 5);      // draw 8 short ticks every day (24hr/8ticks = 3-hour intervals, 5 px high)
  tickMarks(12, 10);    // draw 2 long ticks every day (24hr/2ticks = 12-hour intervals, 10 px high)
  scaleMarks(500, 5);   // args: (Pa, length)
  scaleMarks(1000, 10);
  drawScale();
  drawGraph();
}

// ----- print current value of pressure reading
void printPressure() {
  float fPressure;
  char* sUnits;
  char inHg[] = "inHg";
  char hPa[] = "hPa";
  switch (gUnits) {
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

  setFontSize(9);
  txtReading[eTitle].print();

  setFontSize(12);
  txtReading[valPressure].print( fPressure, 2 );
  txtReading[unitPressure].print( sUnits );
  Serial.print(fPressure, 2); Serial.print(" "); Serial.println(sUnits);
}

void tickMarks(int t, int h) {
  // draw tick marks for the horizontal axis
  // input: t = hours
  //        h = height of tick mark, pixels
  int deltax = t * pixelsPerHour;
  for (int x=xRight; x>xDay1; x = x - deltax) {
    tft.drawLine(x,yBot,  x,yBot - h, cSCALECOLOR);
  }
}

void scaleMarks(int p, int len) {
  // draw scale marks for the vertical axis
  // input: p = pascal intervals
  //        len = length of mark, pixels
  int y = yBot;
  int deltay = map(p, 0, maxP - minP, 0, graphHeight);
  for (y = yBot; y > yBot - graphHeight + 5; y = y - deltay) {
    tft.drawLine(xDay1, y,  xDay1 + len,  y, cSCALECOLOR);  // mark left edge
    tft.drawLine(xRight,y,  xRight - len, y, cSCALECOLOR);  // mark right edge
  }
}

void superimposeLabel(int x, int y, double value, int precision) {
  // to save screen space, the scale numbers are written right on top of the vertical axis
  // x,y = lower left corner of text (baseline) to write
  Point ll = { x, y };

  // measure size of text that will be written
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds("1234",  ll.x,ll.y,  &x1,&y1,  &w,&h);

  // erase area for text, plus a few extra pixels for border
  tft.fillRect(x1-2,y1-1,  w+4,h+2,  cBACKGROUND);
  //tft.drawRect(x1-2,y1-1,  w+4,h+2,  ILI9341_RED);  // debug

  tft.setCursor(ll.x, ll.y);
  tft.print(value, precision);
}

enum { eTODAY, eDATETODAY, eYESTERDAY, eDAYBEFORE };
TextField txtDate[] = {
  TextField{"Today", xDay3+20,yBot-TEXTHEIGHT+2,  ILI9341_CYAN },  // [eTODAY]
  TextField{"8/25",  xDay3+34,yBot+TEXTHEIGHT+1,  ILI9341_CYAN },  // [eDATETODAY]
  TextField{"8/24",  xDay2+34,yBot+TEXTHEIGHT+1,  ILI9341_CYAN },  // [eYESTERDAY]
  TextField{"8/23",  xDay1+20,yBot+TEXTHEIGHT+1,  ILI9341_CYAN },  // [eDAYBEFORE]
};
const int numDates = sizeof(txtDate) / sizeof(txtDate[0]);

void drawScale() {
  // draw horizontal lines
  const int yLine1 = yBot - graphHeight;
  const int yLine2 = yBot;
  const int yMid   = (yLine1 + yLine2)/2;
  tft.drawLine(xDay1, yLine1, xRight, yLine1, cSCALECOLOR);
  tft.drawLine(xDay1, yLine2, xRight, yLine2, cSCALECOLOR);
  for (int ii=xDay1; ii<xRight; ii+=10) {   // dotted line
    tft.drawPixel(ii, yMid, cSCALECOLOR);
    tft.drawPixel(ii+1, yMid, cSCALECOLOR);
  }

  // draw vertical lines
  tft.drawLine( xDay1,yBot - graphHeight,   xDay1,yBot, cSCALECOLOR);
  tft.drawLine( xDay2,yBot - graphHeight,   xDay2,yBot, cSCALECOLOR);
  tft.drawLine( xDay3,yBot - graphHeight,   xDay3,yBot, cSCALECOLOR);
  tft.drawLine(xRight,yBot - graphHeight,  xRight,yBot, cSCALECOLOR);

  // write limits of pressure scale in consistent units
  setFontSize(9);
  tft.setTextColor(ILI9341_CYAN);
  if (gUnits == eEnglish) {
    // english: inches mercury (inHg)
    superimposeLabel( MARGIN, yBot - TEXTHEIGHT/3,                 (minP/3386.4), 1);
    superimposeLabel( MARGIN, yBot - graphHeight + TEXTHEIGHT/3,   (maxP/3386.4), 1);
    superimposeLabel( MARGIN, yBot - graphHeight/2 + TEXTHEIGHT/3, (minP + (maxP - minP)/2)/3386.4, 1);
  } else {
    // metric: hecto-Pascal (hPa)
    superimposeLabel( MARGIN, yBot + TEXTHEIGHT/3,                 (minP/100), 0);
    superimposeLabel( MARGIN, yBot - graphHeight + TEXTHEIGHT/3,   (maxP/100), 0);
    superimposeLabel( MARGIN, yBot - graphHeight/2 + TEXTHEIGHT/3, (minP + (maxP - minP)/2)/100, 0);
  }

  // labels along horizontal axis
  setFontSize(9);
  //Serial.print("eTODAY x,y = "); Serial.print(txtDate[eTODAY].x); Serial.print(","); Serial.println(txtDate[eTODAY].y);
  //Serial.print("eYESTERDAY x,y = "); Serial.print(txtDate[eYESTERDAY].x); Serial.print(","); Serial.println(txtDate[eYESTERDAY].y);
  //Serial.print("eDAYBEFORE x,y = "); Serial.print(txtDate[eDAYBEFORE].x); Serial.print(","); Serial.println(txtDate[eDAYBEFORE].y);

  // get today's date from the RTC (real time clock)
  // Note: The real time clock in the Adafruit Ultimate GPS is not directly readable or 
  //       accessible from the Arduino. It's definitely not writeable. It's only internal to the GPS. 
  //       Once the battery is installed, and the GPS gets its first data reception from satellites 
  //       it will set the internal RTC. Then as long as the battery is installed, you can read the 
  //       time from the GPS as normal. Even without a current "gps fix" the time will be correct.
  //       The RTC timezone cannot be changed, it is always UTC.
  int mo = GPS.month;
  int dd = GPS.day;
  int hh = GPS.hour;
  int mm = GPS.minute;
  int ss = GPS.seconds;

  char msg[128];
  snprintf(msg, sizeof(msg), "GPS time mo(%d) dd(%d) hh(%d) mm(%d) ss(%d)",
                                             mo,    dd,    hh,    mm,    ss);
  Serial.println(msg);      // debug

  char sDateToday[12];      // strlen("12/34") = 5
  snprintf(sDateToday, sizeof(sDateToday), "%d/%d", mo, dd);
  
  TextField::setTextDirty(txtDate, numDates);
  txtDate[eTODAY].print();
  txtDate[eDATETODAY].print(sDateToday);    // "8/25"
  txtDate[eYESTERDAY].print("8/29");
  txtDate[eDAYBEFORE].print("8/28");
}

void drawGraph() {
  int deltax = 2;
  int x = xRight;
  for (int ii = lastIndex; ii > 0 ; ii--) {
    int p1 =  map(pressureStack[ii-0].pressure, minP, maxP, 0, graphHeight);
    int p2 =  map(pressureStack[ii-1].pressure, minP, maxP, 0, graphHeight);
    if (pressureStack[ii - 1].pressure != 0) {
      tft.drawLine(x, yBot - p1, x - deltax, yBot - p2, cGRAPHCOLOR);
    }
    x = x - deltax;
  }
}

void adjustUnits() {
  if (gUnits == eMetric) {
    gUnits = eEnglish;
  } else {
    gUnits = eMetric;
  }
  saveConfigUnits();     // non-volatile storage

  char sInchesHg[] = "inHg";
  char sHectoPa[] = "hPa";
  switch (gUnits) {
    case eEnglish:
      Serial.print("Units changed to English: "); Serial.println(gUnits);
      txtReading[unitPressure].print( sInchesHg );
      break;
    case eMetric:
      Serial.print("Units changed to Metric: "); Serial.println(gUnits);
      txtReading[unitPressure].print( sHectoPa );
      break;
    default:
      Serial.println("Internal Error! Unknown units");
      break;
  }

  Serial.print(". unitPressure = "); Serial.println( txtReading[unitPressure].text );
  getBaroData();
  showReadings(gUnits);
}

// ========== load/save config setting =========================
#define UNITS_CONFIG_FILE    CONFIG_FOLDER "/barogrph.cfg"
#define UNITS_CONFIG_VERSION "Barograph v01"

int loadConfigUnits() {
  SaveRestore config(UNITS_CONFIG_FILE, UNITS_CONFIG_VERSION);
  int tempUnitSetting;
  int result = config.readConfig( (byte*) &tempUnitSetting, sizeof(tempUnitSetting) );
  if (result) {
    gUnits = tempUnitSetting;                // set english/metric units
    Serial.print(". Loaded units setting: "); Serial.println(tempUnitSetting);
  }
  return result;
}

void saveConfigUnits() {
  SaveRestore config(UNITS_CONFIG_FILE, UNITS_CONFIG_VERSION);
  config.writeConfig( (byte*) &gUnits, sizeof(gUnits) );
}

// ========== load/save barometer pressure readings ============
#define PRESSURE_HISTORY_FILE     CONFIG_FOLDER "/barometr.dat"
#define PRESSURE_HISTORY_VERSION  "Pressure v01"

int loadPressureHistory() {
  SaveRestore history(PRESSURE_HISTORY_FILE, PRESSURE_HISTORY_VERSION);
  Reading tempStack[maxReadings] = {};      // array to hold pressure data, fill with zeros
  int result = history.readConfig( (byte*) &tempStack, sizeof(tempStack) );
  if (result) {
    int numNonZero = 0;
    for (int ii=0; ii<maxReadings; ii++) {
      pressureStack[ii] = tempStack[ii];
      if (pressureStack[ii].pressure > 0) {
        numNonZero++;
      }
    }
    
    Serial.print(". Loaded barometric pressure history file, ");
    Serial.print(numNonZero);
    Serial.println(" readings found");
  }
  return result;
}

void savePressureHistory() {
  SaveRestore history(PRESSURE_HISTORY_FILE, PRESSURE_HISTORY_VERSION);
  history.writeConfig( (byte*) &pressureStack, sizeof(pressureStack) );
  Serial.println("Saved the pressure history to non-volatile memory [line "); Serial.print(__LINE__); Serial.println("]");
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
  const int SCALEF = 64;                      // how much to slow it down so it becomes visible

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

  // ----- init TFT display
  tft.begin();                                  // initialize TFT display
  tft.setRotation(SCREEN_ROTATION);             // 1=landscape (default is 0=portrait)
  clearScreen();

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init Feather M4 onboard lights
  pixel.begin();
  pixel.clear();                      // turn off NeoPixel
  digitalWrite(PIN_LED, LOW);         // turn off little red LED
  //Serial.println("NeoPixel initialized and turned off");

  // ----- announce ourselves
  startSplashScreen();

  // ----- init serial monitor
  Serial.begin(115200);                               // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);                       // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(BAROGRAPH_TITLE " " PROGRAM_VERSION);// Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init GPS
  GPS.begin(9600);                              // 9600 NMEA is the default baud rate for Adafruit MTK GPS's
  delay(200);                                   // is delay really needed?
  GPS.sendCommand(PMTK_SET_BAUD_57600);         // set baud rate to 57600
  delay(200);
  GPS.begin(57600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); // turn on RMC (recommended minimum) and GGA (fix data) including altitude

  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    // 1 Hz update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ); // Once every 5 seconds update


  // ----- restore settings
  if (loadConfigUnits()) {
    Serial.println("Successfully loaded settings from non-volatile RAM");
  } else {
    Serial.println("Failed to load settings, re-initializing config file");
    saveConfigUnits();
  }

  // ----- restore pressure history
  if (loadPressureHistory()) {
    Serial.println("Successfully restored barometric pressure history");
    dumpPressureHistory();            // debug
  } else {
    Serial.println("Failed to load barometric pressure history, re-initializing config file");
    savePressureHistory();
  }
  #ifdef RUN_UNIT_TESTS
    initTestPressureHistory();
  #endif

  // ----- init barometer
  if (!baro.begin()) {
    Serial.println("Error, unable to initialize BMP388, check your wiring");
    tft.setCursor(0, yText1);
    tft.setTextColor(cWARN);
    setFontSize(12);
    tft.println("Error!\n Unable to init\n  BMP388 sensor\n   check wiring");
    delay(4000);
  }

  // Set up BMP388 oversampling and filter initialization
  baro.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);
  baro.setPressureOversampling(BMP3_OVERSAMPLING_32X);
  baro.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_127);
  // baro.setOutputDataRate(BMP3_ODR_50_HZ);

  // all done with setup, prepare screen for main program
  delay(3000);         // milliseconds
  clearScreen();

  // Get first data point (done twice because first reading is always bad)
  getBaroData();
  pressureStack[lastIndex].pressure = gPressure + elevCorr;
  showReadings(gUnits);

  getBaroData();
  pressureStack[lastIndex].pressure = gPressure + elevCorr;
  showReadings(gUnits);
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
// Initialized to trigger all processing on first pass in mainline
uint32_t prevTimeGPS = 0;           // timer to process GPS sentence
uint32_t prevTimer1 = 0;            // timer to update displayed value (5 min), UINT32_MIN=0
uint32_t prevTimer2 = 0;            // timer to update displayed graph (20 min)

const int GPS_PROCESS_INTERVAL = 1000;          // Timer GPS = 1 second
const int READ_BAROMETER_INTERVAL = 5*60*1000;  // Timer 1 =  5 minutes
const int LOG_PRESSURE_INTERVAL = 20*60*1000;   // Timer 2 = 20 minutes

void loop() {

  // if our timer or system millis() wrapped around, reset it
  if (prevTimeGPS > millis()) { prevTimeGPS = millis(); }
  if (prevTimer1 > millis()) { prevTimer1 = millis(); }
  if (prevTimer2 > millis()) { prevTimer2 = millis(); }

  GPS.read();   // if you can, read the GPS serial port every millisecond in an interrupt
  if (GPS.newNMEAreceived()) {
    // sentence received -- verify checksum, parse it
    // GPS parsing: https://learn.adafruit.com/adafruit-ultimate-gps/parsed-data-output
    // In the "Baroduino" program, all we need is the RTC date and time, which is sent
    // from the GPS module to the Arduino as NMEA sentences. 
    if (!GPS.parse(GPS.lastNMEA())) {
      // parsing failed -- restart main loop to wait for another sentence
      // this also sets the newNMEAreceived() flag to false
      return;
    }
  }

  // every 5 minutes acquire/print temp and pressure
  if (millis() - prevTimer1 > READ_BAROMETER_INTERVAL) {
    getBaroData();
    showReadings(gUnits);

    // every 20 minutes log, display, and graph pressure/delta pressure
    if (millis() - prevTimer2 > LOG_PRESSURE_INTERVAL) {  // 1200000 for 20 minutes
      prevTimer2 = millis();

      // log this pressure reading
      rememberPressure( gPressure );
      
      // calculate pressure change and reprint all to screen
      //showReadings(gUnits);     // removed - seems redundant with 10 lines above

      // finally save the entire stack in non-volatile RAM
      // which is done after updating display because this can take a visible half-second
      savePressureHistory();
    }
    prevTimer1 = millis();
  }

  // if there's touchscreen input, handle it
  Point touch;
  if (newScreenTap(&touch, SCREEN_ROTATION)) {

    #ifdef SHOW_TOUCH_TARGETS
      const int radius = 3;     // debug
      tft.fillCircle(touch.x, touch.y, radius, cWARN);  // debug - show dot
      touchHandled = true;      // debug - true=stay on same screen
    #endif

    adjustUnits();              // change between "inches mercury" and "millibars" units
  }


  // make a small progress bar crawl along bottom edge
  // this gives a sense of how frequently the main loop is executing
  showActivityBar(tft.height()-1, ILI9341_RED, cBACKGROUND);
}
