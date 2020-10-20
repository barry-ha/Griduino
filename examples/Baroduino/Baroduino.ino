/*
  Baroduino -- demonstrate BMP388 barometric sensor

  Version history: 
            2020-10-02 v0.24 published to the GitHub downloads folder
            2020-09-18 this is really two independent functions: 1.data logging, 2. visualization
            2020-08-27 save unit setting (english/metric) in nonvolatile RAM
            2020-08-24 start rewriting user interface
            2020-05-18 added NeoPixel control to illustrate technique
            2020-05-12 updated TouchScreen code
            2020-03-05 replaced physical button design with touchscreen
            2019-12-18 created from example by John KM7O

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  What can you do with a barometer that knows the time of day? Graph it!
            This program timestamps each reading and stores a history.
            It graphs the most recent 3 days. 
            It saves the readings in non-volatile memory and re-displays them on power-up.
            Its RTC (realtime clock) is updated from the GPS satellite network.

            +-----------------------------------+
            | date        Griduino        hh:mm |
            | #sat       29.97 inHg          ss |
            | 30.5 +--------------------------+ | <- yTop
            |      |        |        |        | |
            |      |        |        |        | |
            |      |        |        |        | |
            | 30.0 +  -  -  -  -  -  -  -  -  + | <- yMid
            |      |        |        |        | |
            |      |        |        |        | |
            |      |        |        |  Today | |
            | 29.5 +--------------------------+ | <- yBot
            |         9/18     9/19     9/20    |
            +------:--------:--------:--------:-+
                   xDay1    xDay2    xDay3    xRight

  Units of Time:
         This relies on "TimeLib.h" which uses "time_t" to represent time.
         The basic unit of time (time_t) is the number of seconds since Jan 1, 1970, 
         a compact 4-byte integer.
         https://github.com/PaulStoffregen/Time

  Units of Pressure:
         hPa is the abbreviated name for hectopascal (100 x 1 pascal) pressure 
         units which are exactly equal to millibar pressure unit (mb or mbar):

         100 Pascals = 1 hPa = 1 millibar. 
         
         The hectopascal or millibar is the preferred unit for reporting barometric 
         or atmospheric pressure in European and many other countries.
         The Adafruit BMP388 Precision Barometric Pressure sensor reports pressure 
         in 'float' values of Pascals.

         In the USA and other backward countries that failed to adopt SI units, 
         barometric pressure is reported as inches-mercury (inHg). 
         
         1 pascal = 0.000295333727 inches of mercury, or 
         1 inch Hg = 3386.39 Pascal
         So if you take the Pascal value of say 100734 and divide by 3386.39 you'll get 29.72 inHg.
         
         The BMP388 sensor has a relative accuracy of 8 Pascals, which translates to 
         about ± 0.5 meter of altitude. 
         
  Real Time Clock:
         The real time clock in the Adafruit Ultimate GPS is not directly readable or 
         accessible from the Arduino. It's definitely not writeable. It's only internal to the GPS. 
         Once the battery is installed, and the GPS gets its first data reception from satellites 
         it will set the internal RTC. Then as long as the battery is installed, you can read the 
         time from the GPS as normal. Even without a current "gps fix" the time will be correct.
         The RTC timezone cannot be changed, it is always UTC.

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)     https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341    https://www.adafruit.com/product/1743

         3. Adafruit Ultimate GPS                           https://www.adafruit.com/product/746

         4. Adafruit BMP388 Barometric Pressure             https://www.adafruit.com/product/3966
*/

#include "Adafruit_GFX.h"             // Core graphics display library
#include "Adafruit_ILI9341.h"         // TFT color display library
#include "TouchScreen.h"              // Touchscreen built in to 3.2" Adafruit TFT display
#include "Adafruit_GPS.h"             // Ultimate GPS library
#include "Adafruit_BMP3XX.h"          // Precision barometric and temperature sensor
#include "Adafruit_NeoPixel.h"        // On-board color addressable LED
#include "save_restore.h"             // save/restore configuration data to SDRAM
#include "hardware.h"                 // Griduino pin definitions 
#include "constants.h"                // Griduino constants, colors, typedefs
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "TimeLib.h"                  // BorisNeubert / Time (who forked it from PaulStoffregen / Time)

// ------- Identity for splash screen and console --------
#define BAROGRAPH_TITLE "Griduino"

//--------------CONFIG--------------
//float elevCorr = 4241;  // elevation correction in Pa, 
// use difference between altimeter setting and station pressure: https://www.weather.gov/epz/wxcalc_altimetersetting

float elevCorr = 0;

float fMaxPa = 102000;        // upper bound of graph, Pa
float fMinPa = 98000;         // lower bound of graph, Pa

float fMaxHg = 30.6;          // upper gound of graph, inHg
float fMinHg = 29.4;          // lower bound of graph, inHg

enum units { eMetric, eEnglish };
int gUnits = eEnglish;         // units on startup: 0=english=inches mercury, 1=metric=millibars

// ---------- extern
extern bool newScreenTap(Point* pPoint, int orientation);  // Touch.cpp
extern uint16_t myPressure(void);                          // Touch.cpp
//extern bool TouchScreen::isTouching(void);               // Touch.cpp
extern void mapTouchToScreen(TSPoint touch, Point* screen, int orientation);

// ========== forward reference ================================
int loadConfigUnits();
void saveConfigUnits();

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- neopixel
#define NUMPIXELS 1         // Feather M4 has one NeoPixel on board
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

const uint32_t colorRed    = pixel.Color(HALFBR, OFF,    OFF);
const uint32_t colorGreen  = pixel.Color(OFF,    HALFBR, OFF);
const uint32_t colorBlue   = pixel.Color(OFF,    OFF,    BRIGHT);
const uint32_t colorPurple = pixel.Color(HALFBR, OFF,    HALFBR);

// ---------- Barometric and Temperature Sensor
Adafruit_BMP3XX baro(BMP_CS);         // hardware SPI

// ---------- GPS ----------
// Hardware serial port for GPS
Adafruit_GPS GPS(&Serial1);

// ------------ typedef's
class Reading {
  public:
    float pressure;             // in millibars, from BMP388 sensor
    time_t time;                // in GMT, from realtime clock 
};

// ------------ definitions
const int howLongToWait = 8;    // max number of seconds at startup waiting for Serial port to console

#define MILLIBARS_PER_INCHES_MERCURY (0.02953)
#define BARS_PER_INCHES_MERCURY      (0.0338639)
#define PASCALS_PER_INCHES_MERCURY   (3386.39)

#define SECS_PER_5MIN  ((time_t)(300UL))
#define SECS_PER_15MIN ((time_t)(900UL))

// ----- color scheme -----
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cSCALECOLOR     ILI9341_DARKGREEN // tried yellow but it's too bright
#define cGRAPHCOLOR     ILI9341_WHITE     // graphed line of baro pressure
#define cICON           ILI9341_CYAN
#define cTITLE          ILI9341_GREEN     
#define cWARN           0xF844            // brighter than ILI9341_RED but not pink
//efine cSINKING        0xF882            // highlight rapidly sinking barometric pressure

// ------------ global barometric data
float inchesHg;
float gPressure;
float hPa;
float feet;

// Old:
//    144 steps at 20 minute refresh time is a 2880 minute (48 hr) graph with 20 minute resolution.
//    with 2px per step, we get 10 minutes/px, and 288px per 48 hrs, leaving some space for graph labels
// New:
//    288px wide graph ==> allows 96 px/day ==> 4px/hour
//    
const int maxReadings = 144;
Reading pressureStack[maxReadings] = {};    // array to hold pressure data, fill with zeros
const int lastIndex = maxReadings - 1;      // index to the last element in pressure array
bool redrawGraph = true;                    // true=request graph be drawn
bool waitingForRTC = true;                  // true=waiting for GPS hardware to give us the first valid date/time

// ======== barometer and temperature helpers ==================
void getBaroData() {
  // returns: gPressure (global var)
  //          hPa       (global var)
  //          inchesHg  (global var)
  if (!baro.performReading()) {
    Serial.println("Error, failed to read barometer");
  }
  // continue anyway, for demo
  gPressure = baro.pressure + elevCorr;   // Pressure is returned in SI units of Pascals. 100 Pascals = 1 hPa = 1 millibar
  hPa = gPressure / 100;
  inchesHg = 0.0002953 * gPressure;
  Serial.print("Barometer ");
  Serial.print(gPressure);
  Serial.print(" Pa [");
  Serial.print(__LINE__);
  Serial.println("]");
}

void rememberPressure( float pressure, time_t time ) {
  // push the barometer reading onto the stack 
  // shift existing stack to the left
  for (int ii = 0; ii < lastIndex; ii++) {
    pressureStack[ii] = pressureStack[ii + 1];
  }
  
  // put the latest pressure onto the stack
  pressureStack[lastIndex].pressure = pressure;
  pressureStack[lastIndex].time = time;
}

void dumpPressureHistory() {            // debug
  Serial.print("Pressure history stack, non-zero values [line "); Serial.print(__LINE__); Serial.println("]");
  for (int ii=0; ii<maxReadings; ii++) {
    Reading item = pressureStack[ii];
    if (item.pressure > 0) {
      Serial.print("Stack["); Serial.print(ii); Serial.print("] = ");
      Serial.print(item.pressure);
      Serial.print("  ");
      char msg[24];
      Serial.println( dateToString(msg, sizeof(msg), item.time) );                          // debug
    }
  }
  return;
}

char* dateToString(char* msg, int len, time_t datetime) {
  // utility function to format date:  "2020-9-27 at 11:22:33"
  // Example 1:
  //      char sDate[24];
  //      dateToString( sDate, sizeof(sDate), now() );
  //      Serial.println( sDate );
  // Example 2:
  //      char sDate[24];
  //      Serial.print("The current time is ");
  //      Serial.println( dateToString(sDate, sizeof(sDate), now()) );
  snprintf(msg, len, "%d-%d-%d at %02d:%02d:%02d",
                     year(datetime),month(datetime),day(datetime), 
                     hour(datetime),minute(datetime),second(datetime));
  return msg;
}

  // Does the GPS real-time clock contain a valid date?
  bool isDateValid(int yy, int mm, int dd) {
    if (yy < 20) {
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

#ifdef RUN_UNIT_TESTS
void initTestStepValues() {
  // inject a straight line constant value and visually verify graph scaling
  // add test pressure values:
  //      y = { 1000, 1020, 1040 }
  int numTestData = 12;                 // add this many entries of test data
  float fakePressure = 1000.1*100;      // first value
  //                              ss,mm,hh, dow, dd,mm,yy
  TimeElements tm = TimeElements{ 0, 0, 0,   1,  26, 9,2020-1970 };   // 27 Sept 2020 on Sun at 12:00:00 am
  time_t fakeTime = makeTime(tm);

  char sDate[24];                       // strlen("1999-12-31 at 00:11:22") = 22
  dateToString(sDate, sizeof(sDate), fakeTime);
  Serial.print("initTestStepValue() starting date: ");    // debug
  Serial.println(sDate);                                  // debug

  // ---test: 1000 hPa---
  for (int ii=0; ii<numTestData; ii++) {
    fakeTime += 15*SECS_PER_MIN;
    rememberPressure( fakePressure, fakeTime );   // inject several samples of the same value
  }

  // ---test: 1020 hPa---
  fakePressure = 1020.0*100;
  for (int ii=0; ii<numTestData; ii++) {
    fakeTime += 15*SECS_PER_MIN;
    rememberPressure( fakePressure, fakeTime );   // inject several samples of the same value
  }

  // ---test: 1040 hPa---
  fakePressure = 1039.9*100;
  for (int ii=0; ii<numTestData; ii++) {
    fakeTime += 15*SECS_PER_MIN;
    rememberPressure( fakePressure, fakeTime );   // inject several samples of the same value
  }
}

void initTestSineWave() {
  // add test data:
  //      sine wave from 990 to 1030 hPa, with period of 12 hours
  //      y = 1010.0hPa + 20*sin(w)   where 'w' goes from 0..2pi in the desired period
  //      timestamps start today at midnight, every 15 minutes
  int numTestData = lastIndex/5;      // add this many entries of test data
  float offsetPa = 1020.0*100;          // readings are saved in Pa (not hPa)
  float amplitude = 10.1*100;           // readings are saved in Pa (not hPa)

  //                              ss,mm,hh, dow, dd,mm,yy
  //meElements tm = TimeElements{  1, 2, 3,  4,   5, 6, 7 };   // 1977-06-05 on Wed at 03:02:01
  TimeElements tm = TimeElements{  0, 0, 0,  1,  26, 9,20 };   // 20 Sept 2020 on Sun at 12:00:00 am
  time_t todayMidnight = makeTime(tm);

  for (int ii=0; ii<numTestData; ii++) {
    float w = 2.0 * PI * ii / numTestData;
    float fakePressure = offsetPa + amplitude * sin( w );
    time_t fakeTime = todayMidnight + ii*15*SECS_PER_MIN;
    rememberPressure( fakePressure, fakeTime );

    Serial.print(ii);                 // debug
    Serial.print(". pressure(");
    Serial.print(fakePressure,1);
    Serial.print(") at ");
    Serial.print( year(fakeTime) ); Serial.print("-");
    Serial.print( month(fakeTime) ); Serial.print("-");
    Serial.print( day(fakeTime) ); Serial.print("  ");
    Serial.print( hour(fakeTime) ); Serial.print(":");
    Serial.print( minute(fakeTime) ); Serial.print(":");
    Serial.print( second(fakeTime) ); Serial.print(" ");
    Serial.println("");
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
const int yTop = yBot - graphHeight;

// ========== screen helpers ===================================
void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

enum { eTitle, eDate, eNumSat, eTimeHHMM, eTimeSS, valPressure, unitPressure };
const int xIndent = 12;         // in pixels, text on main screen
const int yText1 = MARGIN+12;   // in pixels, top row, main screen
const int yText2 = yText1 + 28;
TextField txtReading[] = {
  TextField{ BAROGRAPH_TITLE, xIndent,yText1, cTITLE,      ALIGNCENTER},    // [eTitle]
  TextField{ "09-22",    xIndent+2,yText1,    cWARN,       ALIGNLEFT  },    // [eDate]
  TextField{ "0#",       xIndent+2,yText2-10, cWARN,       ALIGNLEFT  },    // [eNumSat]
  TextField{ "12:34",    gScreenWidth-20,yText1, cWARN,    ALIGNRIGHT },    // [eTimeHHMM]
  TextField{ "56",       gScreenWidth-20,yText2-10, cWARN, ALIGNRIGHT },    // [eTimeSS]
  TextField{ "30.00",    xIndent+150,yText2,  ILI9341_WHITE, ALIGNRIGHT },  // [valPressure]
  TextField{ "inHg",     xIndent+168,yText2,  ILI9341_WHITE, ALIGNLEFT  },  // [unitPressure]
};
const int numReadings = sizeof(txtReading) / sizeof(txtReading[0]);

void showReadings() {
  clearScreen();
  txtSplash[0].setBackground(cBACKGROUND);          // set background for all TextFields
  TextField::setTextDirty( txtReading, numReadings ); // make sure all fields are updated

  printPressure();
  tickMarks(3, 5);      // draw 8 short ticks every day (24hr/8ticks = 3-hour intervals, 5 px high)
  tickMarks(12, 10);    // draw 2 long ticks every day (24hr/2ticks = 12-hour intervals, 10 px high)
  autoScaleGraph();     // update fMinPa/fMaxPa limits on vertical scale
  scaleMarks(500, 5);   // args: (pressure in Pa, length in pixels)
  scaleMarks(1000, 10);
  drawScale();

  if (timeStatus() == timeSet) {
    drawGraph();
  } else {
    TextField wait = TextField{ "Waiting for real-time clock", xIndent,100, cTITLE, ALIGNCENTER };
    wait.print();
  }
}

void showTimeOfDay() {
  // fetch RTC and display it on screen
  char msg[12];               // strlen("12:34:56") = 8
  int mo, dd, hh, mm, ss;
  if (timeStatus() == timeNotSet) {
    mo = dd = hh = mm = ss = 0;
  } else {
    time_t tt = now();
    mo = month(tt);
    dd = day(tt);
    hh = hour(tt);
    mm = minute(tt);
    ss = second(tt);
  }

  snprintf(msg, sizeof(msg), "%d-%02d", mo, dd);
  txtReading[eDate].print(msg);

  snprintf(msg, sizeof(msg), "%d#", GPS.satellites);
  txtReading[eNumSat].print(msg);

  snprintf(msg, sizeof(msg), "%02d:%02d", hh,mm);
  txtReading[eTimeHHMM].print(msg);

  snprintf(msg, sizeof(msg), "%02d", ss);
  txtReading[eTimeSS].print(msg);
}

time_t nextFiveMinuteMark(time_t timestamp) {
  return ((timestamp+1+SECS_PER_5MIN)/SECS_PER_5MIN)*SECS_PER_5MIN;
}
time_t nextFifteenMinuteMark(time_t timestamp) {
  return ((timestamp+1+SECS_PER_15MIN)/SECS_PER_15MIN)*SECS_PER_15MIN;
}

// ----- print current value of pressure reading
void printPressure() {
  float fPressure;
  char* sUnits;
  char inHg[] = "inHg";
  char hPa[] = "hPa";
  switch (gUnits) {
    case eEnglish:
      fPressure = gPressure / PASCALS_PER_INCHES_MERCURY;
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
  //txtReading[eDate].print();
  //txtReading[eTimeHHMM].print();
  //txtReading[eTimeSS].print();

  setFontSize(12);
  txtReading[valPressure].print( fPressure, 2 );
  txtReading[unitPressure].print( sUnits );
  Serial.print("Currently "); Serial.print(fPressure, 2); Serial.print(" "); Serial.print(sUnits);
  Serial.print(" ["); Serial.print(__LINE__); Serial.println("]");
}

void tickMarks(int t, int h) {
  // draw tick marks for the horizontal axis
  // input: t = hours
  //        h = height of tick mark, pixels
  int deltaX = t * pixelsPerHour;
  for (int x=xRight; x>xDay1; x = x - deltaX) {
    tft.drawLine(x,yBot,  x,yBot - h, cSCALECOLOR);
  }
}

void printTwoFloats(float one, float two) {
  Serial.print("(");
  Serial.print(one, 2);
  Serial.print(",");
  Serial.print(two, 2);
  Serial.print(")");
}

void autoScaleGraph() {
  // find min/max limits of vertical scale on graph
  // * find lowest and highest values of pressure stored in array
  // * Metric display:
  //   - set 'fMinPa' to lowest pressure rounded DOWN to nearest multiple of 2000 Pa
  //   - set 'fMaxPa' to highest pressure rounded UP to nearest multiple of 2000 Pa
  // * English display:
  //   - set 'fMinHg' to lowest pressure rounded DOWN to nearest multiple of 0.2 inHg
  //   - set 'fMaxHg' to highest pressure rounded UP to nearest multiple of 0.2 inHg

  float lowestPa = 999999.9;
  float highestPa = 0.0;
  bool isEmpty = true;
  for (int ii = 0; ii <= lastIndex ; ii++) {
    // if element has zero pressure, then it is empty
    if (pressureStack[ii].pressure > 0) {
      lowestPa = min(pressureStack[ii].pressure, lowestPa);
      highestPa = max(pressureStack[ii].pressure, highestPa);
    }
  }
  if (isEmpty) {
    // no data in array, set default range so program doesn't divide-by-zero
    lowestPa  =  95000.0 + 0.1;
    highestPa = 105000.0 - 0.1;
  }
 
  const float PA_RES = 2000.0;      // metric y-axis resolution
  const float HG_RES = 0.2;         // english y-axis resolution

  // metric: calculate graph limits in Pa
  fMinPa = (int)(lowestPa/PA_RES) * PA_RES;
  fMaxPa = (int)((highestPa/PA_RES) + 1) * PA_RES;

  // english: calculate graph limits in inHg
  float lowestHg = lowestPa / 3386.39;
  float highestHg = highestPa / 3386.39;
  
  fMinHg = (int)(lowestHg/HG_RES) * HG_RES;
  fMaxHg = (int)((highestHg/HG_RES) + 1) * HG_RES;

  /*
  Serial.print("Minimum and maximum reported pressure = "); printTwoFloats(lowestPa, highestPa); Serial.println(" Pa");   // debug
  Serial.print("Minimum and maximum vertical scale = "); printTwoFloats(fMinPa, fMaxPa); Serial.println(" Pa");           // debug
  Serial.print("Minimum and maximum reported pressure = "); printTwoFloats(lowestHg, highestHg); Serial.println(" inHg"); // debug
  Serial.print("Minimum and maximum vertical scale = "); printTwoFloats(fMinHg, fMaxHg); Serial.println(" inHg");         // debug
  */
}

void scaleMarks(int p, int len) {
  // draw scale marks for the vertical axis
  // input: p = pascal intervals
  //        len = length of mark, pixels
  int y = yBot;
  //           map(val, fromLow,fromHigh,   toLow,toHigh )
  int deltay = map(p,   0,fMaxPa - fMinPa,  0,graphHeight);
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
    superimposeLabel( MARGIN, yBot - TEXTHEIGHT/3,                 fMinHg, 1);
    superimposeLabel( MARGIN, yBot - graphHeight + TEXTHEIGHT/3,   fMaxHg, 1);
    superimposeLabel( MARGIN, yBot - graphHeight/2 + TEXTHEIGHT/3, (fMinHg + (fMaxHg - fMinHg)/2), 1);
  } else {    // metric: hecto-Pascal (hPa)
    superimposeLabel( MARGIN, yBot + TEXTHEIGHT/3,                 (fMinPa/100), 0);
    superimposeLabel( MARGIN, yBot - graphHeight + TEXTHEIGHT/3,   (fMaxPa/100), 0);
    superimposeLabel( MARGIN, yBot - graphHeight/2 + TEXTHEIGHT/3, (fMinPa + (fMaxPa - fMinPa)/2)/100, 0);
  }

  // labels along horizontal axis
  setFontSize(9);

  // get today's date from the RTC (real time clock)
  time_t today = now();
  time_t yesterday = today - SECS_PER_DAY;
  time_t dayBefore = yesterday - SECS_PER_DAY;

  char msg[128];
  snprintf(msg, sizeof(msg), "RTC time %d-%d-%d at %02d:%02d:%02d",
                                       year(today),month(today),day(today), 
                                       hour(today),minute(today),second(today));
  Serial.println(msg);      // debug

  TextField::setTextDirty(txtDate, numDates);
  txtDate[eTODAY].print();

  char sDate[12];      // strlen("12/34") = 5
  snprintf(sDate, sizeof(sDate), "%d/%d", month(today), day(today));
  txtDate[eDATETODAY].print(sDate);  // "8/25"

  snprintf(sDate, sizeof(sDate), "%d/%d", month(yesterday), day(yesterday));
  txtDate[eYESTERDAY].print(sDate);

  snprintf(sDate, sizeof(sDate), "%d/%d", month(dayBefore), day(dayBefore));
  txtDate[eDAYBEFORE].print(sDate);
}

void drawGraph() {
  // check that RTC has been initialized, otherwise we cannot display a sensible graph
  if (timeStatus() == timeNotSet) {
    Serial.println("!! No graph, time has not been started.");
    return;
  }

  // get today's date from the RTC (real time clock)
  time_t today = now();
  time_t maxTime = nextMidnight(today);
  time_t minTime = maxTime - SECS_PER_DAY*3;

  char msg[100], sDate[24];
  dateToString(sDate, sizeof(sDate), today);

  snprintf(msg, sizeof(msg), "Right now is %d-%d-%d at %02d:%02d:%02d",
                                    year(today),month(today),day(today), 
                                    hour(today),minute(today),second(today));
  Serial.println(msg);
  snprintf(msg, sizeof(msg), "Leftmost graph minTime = %d-%02d-%02d at %02d:%02d:%02d (x=%d)",
                                    year(minTime),month(minTime),day(minTime),
                                    hour(minTime),minute(minTime),second(minTime),
                                    xDay1);
  Serial.println(msg);
  snprintf(msg, sizeof(msg), "Rightmost graph maxTime = %d-%02d-%02d at %02d:%02d:%02d (x=%d)",
                                    year(maxTime),month(maxTime),day(maxTime),
                                    hour(maxTime),minute(maxTime),second(maxTime),
                                    xRight);
  Serial.println(msg);

  float yTopPa = (gUnits == eMetric) ? fMaxPa : (fMaxHg*PASCALS_PER_INCHES_MERCURY);
  float yBotPa = (gUnits == eMetric) ? fMinPa : (fMinHg*PASCALS_PER_INCHES_MERCURY);

  Serial.print("Top graph pressure = "); Serial.print(yTopPa,1); Serial.println(" Pa");     // debug
  Serial.print("Bottom graph pressure = "); Serial.print(yBotPa,1); Serial.println(" Pa");  // debug

  int x = xRight;
  for (int ii = lastIndex; ii > 0 ; ii--) {
    if (pressureStack[ii - 1].pressure != 0) {
      // Y-axis:
      //    The data to plot is always 'float Pascals' 
      //    but the graph's y-axis is either Pascals or inches-Hg, each with different scale
      //    so scale the data into the appropriate units on the y-axis
      if (gUnits == eMetric) {
        
      }
      int y1 = map(pressureStack[ii-0].pressure,  yBotPa,yTopPa,  yBot,yTop);
  
      // X-axis:
      //    Scale from timestamps onto x-axis
      int t1 = pressureStack[ii-0].time;
      //       map(value, fromLow,fromHigh, toLow,toHigh)
      int x1 = map( t1,   minTime,maxTime,  xDay1,xRight);

      if (x1 < xDay1) {
        snprintf(msg, sizeof(msg), "%d. Ignored: Date x1 (%d) is off left edge of (%d).", ii, x1, xDay1); Serial.println(msg);
        continue;
      }
      if (x1 > xRight) {
        snprintf(msg, sizeof(msg), "%d. Ignored: Date x1 (%d) is off right edge of (%d).", ii, x1, xRight); Serial.println(msg);
        continue;
      }

      tft.drawPixel(x1,y1, cGRAPHCOLOR);
      int approxPa = (int)pressureStack[ii-0].pressure;
      //snprintf(msg, sizeof(msg), "%d. Plot %d at pixel (%d,%d)", ii, approxPa, x1,y1);
      //Serial.println(msg);    // debug
    }
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
  //getBaroData();              // delete? I think this disturbs the scheduled 5-minute readings
  redrawGraph = true;           // draw graph
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
// To erase and rewrite a new data file, change the version string below.
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
  Serial.print("Saved the pressure history to non-volatile memory [line "); Serial.print(__LINE__); Serial.println("]");
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
  analogWrite(TFT_BL, 255);             // start at full brightness

  // ----- init Feather M4 onboard lights
  pixel.begin();
  pixel.clear();                        // turn off NeoPixel
  digitalWrite(PIN_LED, LOW);           // turn off little red LED
  Serial.println("NeoPixel initialized and turned off");

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

  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    // 1 Hz update rate
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ); // Once every 5 seconds update

  // ----- init RTC
  // Note: See the main() loop. 
  //       The RTC is not available until after receiving a few NMEA sentences.

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
    initTestStepValues();
    //initTestSineWave();
  #endif

  // ----- init barometer
  if (!baro.begin()) {
    Serial.println("Error, unable to initialize BMP388, check the wiring");
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
  delay(3000);              // milliseconds
  clearScreen();

  // Get first data point (repeated because first reading is always bad)
  for (int ii=0; ii<4; ii++) {
    getBaroData();
    delay(100);
  }
  redrawGraph = true;                 // draw graph
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
// Initialized to trigger all processing on first pass in mainline
uint32_t prevTimeGPS = 0;             // timer to process GPS sentence
uint32_t prevShowTime = 0;            // timer to update displayed time-of-day (1 second)

time_t nextShowPressure = 0;          // timer to update displayed value (5 min), init to take a reading soon after startup
time_t nextSavePressure = 0;          // timer to log pressure reading (15 min)

const int RTC_PROCESS_INTERVAL = 1000;          // Timer RTC = 1 second
const int READ_BAROMETER_INTERVAL = 5*60*1000;  // Timer 1 =  5 minutes
const int LOG_PRESSURE_INTERVAL = 15*60*1000;   // Timer 2 = 15 minutes

void loop() {

  // if a timer or system millis() wrapped around, reset it
  if (prevTimeGPS > millis()) { prevTimeGPS = millis(); }
  if (prevShowTime > millis()) { prevShowTime = millis(); }

  GPS.read();   // if you can, read the GPS serial port every millisecond in an interrupt

  if (GPS.newNMEAreceived()) {
    // sentence received -- verify checksum, parse it
    // GPS parsing: https://learn.adafruit.com/adafruit-ultimate-gps/parsed-data-output
    // In this barometer program, all we need is the RTC date and time, which is
    // sent from the GPS module to the Arduino as NMEA sentences.
    if (!GPS.parse(GPS.lastNMEA())) {
      // parsing failed -- wait til the next main loop for another sentence
      // this also sets the newNMEAreceived() flag to false
    }
  }

  // look for the first "setTime()" and request to display the graph
  if (waitingForRTC && isDateValid(GPS.year, GPS.month, GPS.day)) {
    // found a transition from an unknown date -> correct date/time
    // assuming "class Adafruit_GPS" contains 2000-01-01 00:00 until it receives an update via NMEA sentences
    // the next step (1 second timer) will actually set the clock
    redrawGraph = true;
    waitingForRTC = false;

    char msg[128];            // debug
    Serial.println("-----> Found first correct date/time value <-----");  // debug
    snprintf(msg, sizeof(msg), "       GPS time %d-%02d-%02d at %02d:%02d:%02d",
                                       GPS.year,GPS.month,GPS.day, 
                                       GPS.hour,GPS.minute,GPS.seconds);
    Serial.println(msg);      // debug
  }

  // every 1 second update the clock display
  if (millis() - prevShowTime > RTC_PROCESS_INTERVAL) {
    prevShowTime = millis();

    // update RTC from GPS
    if (isDateValid(GPS.year, GPS.month, GPS.day)) {
      
      setTime(GPS.hour, GPS.minute, GPS.seconds, GPS.day, GPS.month, GPS.year);
      //adjustTime(offset * SECS_PER_HOUR);   // todo - adjust to local time zone. for now, we only do GMT
    }

    // update display
    showTimeOfDay();
  }

  // every 5 minutes acquire/print temp and pressure
  // synchronize showReadings() on exactly the 5-minute marks 
  // so the user can easily predict when the next update will occur
  time_t rightnow = now();
  if ( rightnow >= nextShowPressure) {
    nextShowPressure = nextFiveMinuteMark(rightnow);
  
    getBaroData();
    redrawGraph = true;               // draw graph
  }

  // every 15 minutes save pressure in nonvolatile RAM
  if ( rightnow >= nextSavePressure) {
    
    // log this pressure reading ONLY IF the time-of-day is correct and initialized 
    if (timeStatus() == timeSet) {

      nextSavePressure = nextFifteenMinuteMark(rightnow);

      rememberPressure( gPressure, rightnow );
      savePressureHistory();
    }
  }

  // if the barometric pressure graph should be refreshed
  if (redrawGraph) {
    showReadings();
    redrawGraph = false;
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

  // small activity bar crawls along bottom edge to give 
  // a sense of how frequently the main loop is executing
  showActivityBar(tft.height()-1, ILI9341_RED, cBACKGROUND);
}
