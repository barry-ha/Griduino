/*
  Baroduino - a graphing 3-day barometer
            A standalone example program to demonstrate BMP388 or BMP390 barometric sensor

  Version history:
            2021-02-02 this example program was merged into the main Griduino.ino program
            2021-01-30 added support for BMP390 and latest Adafruit_BMP3XX library, v0.31
            2020-12-19 v0.30 published to the GitHub downloads folder (no functional change)
            2020-10-02 v0.24 published to the GitHub downloads folder
            2020-09-18 this is really two independent functions: (1)data logger, (2)visualizer
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

            +---------------------------------------+
            | date          Baroduino         hh:mm |
            | #sat         29.97 inHg            ss |
            |  30.5+-----------------------------+  | <- yTop
            |      |         |         |         |  |
            |      |         |         |         |  |
            |      |         |         |         |  |
            | 30.0 +  -  -  -  -  -  -  -  -  -  +  | <- yMid
            |      |         |         |         |  |
            |      |         |         |         |  |
            |      |         |         |  Today  |  |
            | 29.5 +-----------------------------+  | <- yBot
            |         10/18     10/19     10/20     |
            +------:---------:---------:---------:--+
                   xDay1     xDay2     xDay3     xRight

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
         about +/- 0.5 meter of altitude.
         
  Real Time Clock:
         The real time clock in the Adafruit Ultimate GPS is not directly readable nor 
         accessible from the Arduino. It's definitely not writeable. It's only internal 
         to the GPS. Once the battery is installed, and the GPS gets its first data 
         reception from satellites it will set the internal RTC. Then as long as the 
         battery is installed, this program can read the time from the GPS as normal. 
         Even without a current "gps fix" the time will be correct.
         The RTC timezone cannot be changed, it is always UTC.

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)     https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341    https://www.adafruit.com/product/1743

         3. Adafruit BMP388 - Precision Barometric Pressure https://www.adafruit.com/product/3966
            Adafruit BMP390                                 https://www.adafruit.com/product/4816

         4. Adafruit Ultimate GPS                           https://www.adafruit.com/product/746

*/

#include <Adafruit_GFX.h>             // Core graphics display library
#include <Adafruit_ILI9341.h>         // TFT color display library
#include <TouchScreen.h>              // Touchscreen built in to 3.2" Adafruit TFT display
#include <Adafruit_GPS.h>             // Ultimate GPS library
#include <TimeLib.h>                  // time_t=seconds since Jan 1, 1970, https://github.com/PaulStoffregen/Time
#include "Adafruit_BMP3XX.h"          // Precision barometric and temperature sensor
#include <Adafruit_NeoPixel.h>        // On-board color addressable LED
#include "model_baro.h"               // Model of a barometer that stores 3-day history
#include "save_restore.h"             // Save configuration in non-volatile RAM
#include "constants.h"                // Griduino constants, colors, typedefs
#include "hardware.h"                 // Griduino pin definitions
#include "TextField.h"                // Optimize TFT display text for proportional fonts

// ------- Identity for splash screen and console --------
#define BAROGRAPH_TITLE "Baroduino"

//--------------CONFIG--------------
//float elevCorr = 4241;  // elevation correction in Pa, 
// use difference between altimeter setting and station pressure: https://www.weather.gov/epz/wxcalc_altimetersetting

float elevCorr = 0;

float fMaxPa = 102000;                // top bar of graph: highest pressure rounded UP to nearest multiple of 2000 Pa
float fMinPa = 98000;                 // bot bar of graph: lowest pressure rounded DOWN to nearest multiple of 2000 Pa

float fMaxHg = 30.6;                  // top axis of graph: upper bound of graph, inHg
float fMinHg = 29.4;                  // bot axis of graph: bound of graph, inHg

// Use the following to control how much the initial graph fills the display.
// Smaller numbers give bigger, more dynamic graph.
// Larger numbers make the first graph look smaller and smoother. 
const float PA_RES = 400.0;           // metric y-axis resolution, ie, nearest 2000 Pa (20 hPa)
const float HG_RES = 0.2;             // english y-axis resolution, ie, nearest 0.2 inHg

enum units { eMetric, eEnglish };
int gUnits = eEnglish;                // units on startup: 0=english=inches mercury, 1=metric=millibars

// ---------- extern
extern bool newScreenTap(Point* pPoint, int orientation); // Touch.cpp
extern uint16_t myPressure(void);                         // Touch.cpp
//extern bool TouchScreen::isTouching(void);              // Touch.cpp
extern void mapTouchToScreen(TSPoint touch, Point* screen, int orientation);
extern void setFontSize(int font);                        // TextField.cpp

// ========== forward reference ================================
int loadConfigUnits();
void saveConfigUnits();

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- NeoPixel
#define NUMPIXELS 1                   // Feather M4 has one NeoPixel on board
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

const uint32_t colorRed    = pixel.Color(HALFBR, OFF,    OFF);
const uint32_t colorGreen  = pixel.Color(OFF,    HALFBR, OFF);
const uint32_t colorBlue   = pixel.Color(OFF,    OFF,    BRIGHT);
const uint32_t colorPurple = pixel.Color(HALFBR, OFF,    HALFBR);

// ---------- Barometric and Temperature Sensor
Adafruit_BMP3XX baro;                 // hardware SPI

// ---------- GPS ----------
// Hardware serial port for GPS
Adafruit_GPS GPS(&Serial1);         // https://github.com/adafruit/Adafruit_GPS

// ------------ definitions
const int howLongToWait = 10;         // max number of seconds at startup waiting for Serial port to console

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cICON           ILI9341_CYAN
#define cTITLE          ILI9341_GREEN
#define cWARN           0xF844        // brighter than ILI9341_RED but not pink

// ======== date time helpers =================================
char* datetimeToString(char* msg, int len, time_t datetime) {
  // utility function to format date:  "2020-9-27 at 11:22:33"
  // Example 1:
  //      char sDate[24];
  //      datetimeToString( sDate, sizeof(sDate), now() );
  //      Serial.println( sDate );
  // Example 2:
  //      char sDate[24];
  //      Serial.print("The current time is ");
  //      Serial.println( datetimeToString(sDate, sizeof(sDate), now()) );
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

time_t nextOneSecondMark(time_t timestamp) {
  return timestamp+1;
}
time_t nextOneMinuteMark(time_t timestamp) {
  return ((timestamp+1+SECS_PER_MIN)/SECS_PER_MIN)*SECS_PER_MIN;
}
time_t nextFiveMinuteMark(time_t timestamp) {
  return ((timestamp+1+SECS_PER_5MIN)/SECS_PER_5MIN)*SECS_PER_5MIN;
}
time_t nextFifteenMinuteMark(time_t timestamp) {
  return ((timestamp+1+SECS_PER_15MIN)/SECS_PER_15MIN)*SECS_PER_15MIN;
}

//==============================================================
//
//      BarometerModel
//      "Class BarometerModel" is intended to be identical 
//      for both Griduino and the Baroduino example
//
//    This model collects data from the BMP388 barometric pressure 
//    and temperature sensor on a schedule determined by the Controller.
//
//    288px wide graph ==> 96 px/day ==> 4px/hour ==> log pressure every 15 minutes
//
//==============================================================

bool redrawGraph = true;              // true=request graph be drawn
bool waitingForRTC = true;            // true=waiting for GPS hardware to give us the first valid date/time

BarometerModel baroModel( &baro, BMP_CS ); // create instance of the model, giving it ptr to hardware and SPI chip select

// ======== unit tests =========================================
#ifdef RUN_UNIT_TESTS
void initTestStepValues() {
  // inject a straight line constant value and visually verify graph scaling
  // add test pressure values:
  //      y = { 1000, 1020, 1040 }
  int numTestData = 12;                 // add this many entries of test data
  float fakePressure = 1000.1*100;      // first value
  //                              ss,mm,hh, dow, dd,mm,yy
  TimeElements tm = TimeElements{ 0, 0, 0,   1,  23,10,2020-1970 };   // 23 Oct 2020 on Sun at 12:00:00 am
  time_t fakeTime = makeTime(tm);

  char sDate[24];                       // strlen("1999-12-31 at 00:11:22") = 22
  datetimeToString(sDate, sizeof(sDate), fakeTime);
  Serial.print("initTestStepValue() starting date: ");    // debug
  Serial.println(sDate);                                  // debug

  // ---test: 1000 hPa---
  for (int ii=0; ii<numTestData; ii++) {
    fakeTime += 15*SECS_PER_MIN;
    baroModel.testRememberPressure( fakePressure, fakeTime );   // inject several samples of the same value
  }

  // ---test: 1020 hPa---
  fakePressure = 1020.0*100;
  for (int ii=0; ii<numTestData; ii++) {
    fakeTime += 15*SECS_PER_MIN;
    baroModel.testRememberPressure( fakePressure, fakeTime );   // inject several samples of the same value
  }

  // ---test: 1040 hPa---
  fakePressure = 1039.9*100;
  for (int ii=0; ii<numTestData; ii++) {
    fakeTime += 15*SECS_PER_MIN;
    baroModel.testRememberPressure( fakePressure, fakeTime );   // inject several samples of the same value
  }
}

void initTestSineWave() {
  // add test data:
  //      sine wave from 990 to 1030 hPa, with period of 12 hours
  //      y = 1010.0hPa + 20*sin(w)   where 'w' goes from 0..2pi in the desired period
  //      timestamps start today at midnight, every 15 minutes
  int numTestData = lastIndex/5;      // add this many entries of test data
  float offsetPa = 1020.0*100;        // readings are saved in Pa (not hPa)
  float amplitude = 10.1*100;         // readings are saved in Pa (not hPa)

  //                              ss,mm,hh, dow, dd,mm,yy
  //meElements tm = TimeElements{  1, 2, 3,  4,   5, 6, 7 };   // 1977-06-05 on Wed at 03:02:01
  TimeElements tm = TimeElements{  0, 0, 0,  1,  26, 9,20 };   // 20 Sept 2020 on Sun at 12:00:00 am
  time_t todayMidnight = makeTime(tm);

  for (int ii=0; ii<numTestData; ii++) {
    float w = 2.0 * PI * ii / numTestData;
    float fakePressure = offsetPa + amplitude * sin( w );
    time_t fakeTime = todayMidnight + ii*15*SECS_PER_MIN;
    baroModel.testRememberPressure( fakePressure, fakeTime );

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

// ========== splash screen helpers ============================
// splash screen layout
#define yRow1   54                    // program title: "Barograph"
#define yRow2   yRow1 + 28            // program version
#define yRow3   yRow2 + 48            // author line 1
#define yRow4   yRow3 + 32            // author line 2

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
const int graphHeight = 160;          // in pixels
const int pixelsPerHour = 4;          // 4 px/hr graph
//nst int pixelsPerDay = 72;          // 3 px/hr * 24 hr/day = 72 px/day
const int pixelsPerDay = pixelsPerHour * 24;  // 4 px/hr * 24 hr/day = 96 px/day

const int MARGIN = 6;                 // reserve an outer blank margin on all sides

// to center the graph: xDay1 = [(total screen width) - (reserved margins) - (graph width)]/2 + margin
//                      xDay1 = [(320 px) - (2*6 px) - (3*96 px)]/2 + 6
//                      xDay1 = 16
const int xDay1 = MARGIN+10;          // pixels
const int xDay2 = xDay1 + pixelsPerDay;
const int xDay3 = xDay2 + pixelsPerDay;

const int xRight = xDay3 + pixelsPerDay;

const int TEXTHEIGHT = 16;            // text line spacing, pixels
const int DESCENDERS = 6;             // proportional font descenders may go 6 pixels below baseline
const int yBot = gScreenHeight - MARGIN - DESCENDERS - TEXTHEIGHT;
const int yTop = yBot - graphHeight;

// ========== text screen layout ===================================
void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

enum { eTitle, eDate, eNumSat, eTimeHHMM, eTimeSS, valPressure, unitPressure };
const int xIndent = 12;               // in pixels, for text on top rows
const int yText1 = MARGIN+12;         // in pixels, for text on top row
const int yText2 = yText1 + 28;
TextField txtReading[] = {
  //          text                  x,y       color        align        font
  TextField{ BAROGRAPH_TITLE, xIndent,yText1, cTITLE,      ALIGNCENTER, 9 }, // [eTitle]
  TextField{ "09-22",    xIndent+2,yText1,    cWARN,       ALIGNLEFT,   9 }, // [eDate]
  TextField{ "0#",       xIndent+2,yText2-10, cWARN,       ALIGNLEFT,   9 }, // [eNumSat]
  TextField{ "12:34",    gScreenWidth-20,yText1, cWARN,    ALIGNRIGHT,  9 }, // [eTimeHHMM]
  TextField{ "56",       gScreenWidth-20,yText2-10, cWARN, ALIGNRIGHT,  9 }, // [eTimeSS]
  TextField{ "30.00",    xIndent+150,yText2,  ILI9341_WHITE, ALIGNRIGHT,12}, // [valPressure]
  TextField{ "inHg",     xIndent+168,yText2,  ILI9341_WHITE, ALIGNLEFT, 12}, // [unitPressure]
};
const int numReadings = sizeof(txtReading) / sizeof(txtReading[0]);

void showReadings() {
  clearScreen();
  txtSplash[0].setBackground(cBACKGROUND);          // set background for all TextFields
  TextField::setTextDirty( txtReading, numReadings ); // make sure all fields are updated

  float pascals = baroModel.getBaroPressure();
  printPressure( pascals );
  tickMarks(3, 5);                    // draw 8 short ticks every day (24hr/8ticks = 3-hour intervals, 5 px high)
  tickMarks(12, 10);                  // draw 2 long ticks every day (24hr/2ticks = 12-hour intervals, 10 px high)
  autoScaleGraph();                   // update fMinPa/fMaxPa limits on vertical scale
  
  // minor scale marks: we think 8 marks looks good along the vertical scale
  int interval = (int)( (fMaxPa - fMinPa)/8 );
  scaleMarks(interval, 6);            // args: (pressure in Pa, length in pixels)
  // major scale marks: we think 4 marks looks good
  scaleMarks(interval*2, 10);
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
  char msg[12];                       // strlen("12:34:56") = 8
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
  txtReading[eDate].print(msg);       // 2020-11-12 do show date, help identify when RTC stops

  snprintf(msg, sizeof(msg), "%d#", GPS.satellites);
  txtReading[eNumSat].print(msg);     // 2020-11-12 do show date, help identify when RTC stops

  snprintf(msg, sizeof(msg), "%02d:%02d", hh,mm);
  txtReading[eTimeHHMM].print(msg);

  snprintf(msg, sizeof(msg), "%02d", ss);
  txtReading[eTimeSS].print(msg);
}

// ----- print current value of pressure reading
// input: pressure in Pascals 
void printPressure(float pascals) {
  char* sUnits;
  char inHg[] = "inHg";
  char hPa[] = "hPa";
  float fPressure;
  int decimals = 1;
  if (gUnits == eEnglish) {
    fPressure = pascals / PASCALS_PER_INCHES_MERCURY;
    sUnits = inHg;
    decimals = 2;
  } else {
    fPressure = pascals / 100;
    sUnits = hPa;
    decimals = 2;
  }

  txtReading[eTitle].print();
  txtReading[valPressure].print( fPressure, decimals );
  txtReading[unitPressure].print( sUnits );
  //Serial.print("Displaying "); Serial.print(fPressure, decimals); Serial.print(" "); Serial.println(sUnits);
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
  //   - set 'fMinPa' to lowest recorded pressure rounded DOWN to nearest multiple of 2000 Pa
  //   - set 'fMaxPa' to highest recorded pressure rounded UP to nearest multiple of 2000 Pa
  // * English display:
  //   - set 'fMinHg' to lowest pressure rounded DOWN to nearest multiple of 0.2 inHg
  //   - set 'fMaxHg' to highest pressure rounded UP to nearest multiple of 0.2 inHg

  float lowestPa = 1E30;              // start at larger than real pressures, to find minimum
  float highestPa = 0.0;              // start at smaller than real pressures, to find maximum
  bool bEmpty = true;                 // assume stored pressure array is full of zeros
  for (int ii = 0; ii <= lastIndex ; ii++) {
    // if element has zero pressure, then it is empty
    if (baroModel.pressureStack[ii].pressure > 0) {
      lowestPa = min(baroModel.pressureStack[ii].pressure, lowestPa);
      highestPa = max(baroModel.pressureStack[ii].pressure, highestPa);
      bEmpty = false;
    }
  }
  if (bEmpty) {
    // no data in array, set default range so program doesn't divide-by-zero
    lowestPa  =  95000.0 + 0.1;
    highestPa = 105000.0 - 0.1;
  }
 
  // metric: round up/down the extremes to calculate graph limits in Pa
  fMinPa = (int)(lowestPa/PA_RES) * PA_RES;
  fMaxPa = (int)((highestPa/PA_RES) + 1) * PA_RES;

  // english: calculate graph limits in inHg
  float lowestHg = lowestPa * INCHES_MERCURY_PER_PASCAL;
  float highestHg = highestPa * INCHES_MERCURY_PER_PASCAL;
  
  fMinHg = (int)(lowestHg/HG_RES) * HG_RES;
  fMaxHg = (int)((highestHg/HG_RES) + 1) * HG_RES;

  /* */
  Serial.print(": Minimum and maximum reported pressure = "); printTwoFloats(lowestPa, highestPa); Serial.println(" Pa");   // debug
  Serial.print(": Minimum and maximum vertical scale = "); printTwoFloats(fMinPa, fMaxPa); Serial.println(" Pa");           // debug
  Serial.print(": Minimum and maximum reported pressure = "); printTwoFloats(lowestHg, highestHg); Serial.println(" inHg"); // debug
  Serial.print(": Minimum and maximum vertical scale = "); printTwoFloats(fMinHg, fMaxHg); Serial.println(" inHg");         // debug
  /* */
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
  TextField{"Today", xDay3+20,yBot-TEXTHEIGHT+2,  ILI9341_CYAN, 9}, // [eTODAY]
  TextField{"8/25",  xDay3+34,yBot+TEXTHEIGHT+1,  ILI9341_CYAN, 9}, // [eDATETODAY]
  TextField{"8/24",  xDay2+34,yBot+TEXTHEIGHT+1,  ILI9341_CYAN, 9}, // [eYESTERDAY]
  TextField{"8/23",  xDay1+20,yBot+TEXTHEIGHT+1,  ILI9341_CYAN, 9}, // [eDAYBEFORE]
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
  } else {
    // metric: hecto-Pascal (hPa)
    superimposeLabel( MARGIN, yBot + TEXTHEIGHT/3,                 (fMinPa/100), 0);
    superimposeLabel( MARGIN, yBot - graphHeight + TEXTHEIGHT/3,   (fMaxPa/100), 0);
    superimposeLabel( MARGIN, yBot - graphHeight/2 + TEXTHEIGHT/3, (fMinPa + (fMaxPa - fMinPa)/2)/100, 0);
  }

  // labels along horizontal axis

  // get today's date from the RTC (real time clock)
  time_t today = now();
  time_t yesterday = today - SECS_PER_DAY;
  time_t dayBefore = yesterday - SECS_PER_DAY;

  char msg[128];
  snprintf(msg, sizeof(msg), "RTC time %d-%d-%d at %02d:%02d:%02d",
                                       year(today),month(today),day(today), 
                                       hour(today),minute(today),second(today));
  Serial.println(msg);                // debug

  TextField::setTextDirty(txtDate, numDates);
  txtDate[eTODAY].print();

  char sDate[12];                     // strlen("12/34") = 5
  snprintf(sDate, sizeof(sDate), "%d/%d", month(today), day(today));
  txtDate[eDATETODAY].print(sDate);   // "8/25"

  snprintf(sDate, sizeof(sDate), "%d/%d", month(yesterday), day(yesterday));
  txtDate[eYESTERDAY].print(sDate);

  snprintf(sDate, sizeof(sDate), "%d/%d", month(dayBefore), day(dayBefore));
  txtDate[eDAYBEFORE].print(sDate);
}

void drawGraph() {
  // check that RTC has been initialized, otherwise we cannot display a sensible graph
  if (timeStatus() == timeNotSet) {
    Serial.println("!! No graph, real-time clock has not been set.");
    return;
  }

  // get today's date from the RTC (real time clock)
  time_t today = now();
  time_t maxTime = nextMidnight(today);
  time_t minTime = maxTime - SECS_PER_DAY*3;

  char msg[100], sDate[24];
  datetimeToString(sDate, sizeof(sDate), today);

  snprintf(msg, sizeof(msg), ". Right now is %d-%d-%d at %02d:%02d:%02d",
                                    year(today),month(today),day(today), 
                                    hour(today),minute(today),second(today));
  Serial.println(msg);                // debug
  snprintf(msg, sizeof(msg), ". Leftmost graph minTime = %d-%02d-%02d at %02d:%02d:%02d (x=%d)",
                                    year(minTime),month(minTime),day(minTime),
                                    hour(minTime),minute(minTime),second(minTime),
                                    xDay1);
  Serial.println(msg);                // debug
  snprintf(msg, sizeof(msg), ". Rightmost graph maxTime = %d-%02d-%02d at %02d:%02d:%02d (x=%d)",
                                    year(maxTime),month(maxTime),day(maxTime),
                                    hour(maxTime),minute(maxTime),second(maxTime),
                                    xRight);
  Serial.println(msg);                // debug

  float yTopPa = (gUnits == eMetric) ? fMaxPa : (fMaxHg*PASCALS_PER_INCHES_MERCURY);
  float yBotPa = (gUnits == eMetric) ? fMinPa : (fMinHg*PASCALS_PER_INCHES_MERCURY);

  Serial.print(". Top graph pressure = "); Serial.print(yTopPa,1); Serial.println(" Pa");     // debug
  Serial.print(". Bottom graph pressure = "); Serial.print(yBotPa,1); Serial.println(" Pa");  // debug
  Serial.print(". Saving "); Serial.print(sizeof(baroModel.pressureStack)); Serial.print(" bytes, ");  // debug
  Serial.print(sizeof(baroModel.pressureStack)/sizeof(baroModel.pressureStack[0])); Serial.println(" readings");  // debug

  // loop through entire saved array of pressure readings
  // each reading is one point, i.e., one pixel (we don't draw lines connecting the dots)
  for (int ii = lastIndex; ii >= 0 ; ii--) {
    if (baroModel.pressureStack[ii].pressure != 0) {
      // Y-axis:
      //    The data to plot is always 'float Pascals' 
      //    but the graph's y-axis is either Pascals or inches-Hg, each with different scale
      //    so scale the data into the appropriate units on the y-axis
      int y1 = map(baroModel.pressureStack[ii].pressure,  yBotPa,yTopPa,  yBot,yTop);
  
      // X-axis:
      //    Scale from timestamps onto x-axis
      time_t t1 = baroModel.pressureStack[ii].time;
      //       map(value, fromLow,fromHigh, toLow,toHigh)
      int x1 = map( t1,   minTime,maxTime,  xDay1,xRight);

      if (x1 < xDay1) {
        datetimeToString(sDate, sizeof(sDate), t1);
        snprintf(msg, sizeof(msg), "%d. Ignored: Date x1 (%s = %d) is off left edge of (%d).", 
                                    ii,                  sDate,x1,                   xDay1); 
        Serial.println(msg);          // debug
        continue;
      }
      if (x1 > xRight) {
        datetimeToString(sDate, sizeof(sDate), t1);
        snprintf(msg, sizeof(msg), "%d. Ignored: Date x1 (%s = %d) is off right edge of (%d).", 
                                    ii,                 sDate, x1,                   xRight); 
        Serial.println(msg);          // debug
        continue;
      }

      tft.drawPixel(x1,y1, cGRAPHCOLOR);
      int approxPa = (int)baroModel.pressureStack[ii].pressure;
      //snprintf(msg, sizeof(msg), "%d. Plot %d at pixel (%d,%d)", ii, approxPa, x1,y1);
      //Serial.println(msg);          // debug
    }
  }
}

void adjustUnits() {
  if (gUnits == eMetric) {
    gUnits = eEnglish;
  } else {
    gUnits = eMetric;
  }
  saveConfigUnits();                  // non-volatile storage

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
  redrawGraph = true;                 // draw graph
}

// ========== load/save config setting =========================
#define UNITS_CONFIG_FILE    CONFIG_FOLDER "/barogrph.cfg"
#define UNITS_CONFIG_VERSION "Barograph v01"

int loadConfigUnits() {
  SaveRestore config(UNITS_CONFIG_FILE, UNITS_CONFIG_VERSION);
  int tempUnitSetting;
  int result = config.readConfig( (byte*) &tempUnitSetting, sizeof(tempUnitSetting) );
  if (result) {
    gUnits = tempUnitSetting;         // set english/metric units
    Serial.print(". Loaded units setting: "); Serial.println(tempUnitSetting);
  }
  return result;
}

void saveConfigUnits() {
  SaveRestore config(UNITS_CONFIG_FILE, UNITS_CONFIG_VERSION);
  config.writeConfig( (byte*) &gUnits, sizeof(gUnits) );
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connection to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong*1000;
  while (millis() < targetTime) {
    if (Serial) break;
  }
}

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  static int addDotX = 10;            // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count = 0;
  const int SCALEF = 64;              // how much to slow it down so it becomes visible

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
  tft.begin();                        // initialize TFT display
  tft.setRotation(SCREEN_ROTATION);   // 1=landscape (default is 0=portrait)
  clearScreen();

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init Feather M4 onboard lights
  pixel.begin();
  pixel.clear();                      // turn off NeoPixel
  digitalWrite(PIN_LED, LOW);         // turn off little red LED
  Serial.println("NeoPixel initialized and turned off");

  // ----- announce ourselves
  startSplashScreen();

  // ----- init serial monitor
  Serial.begin(115200);               // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);       // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(BAROGRAPH_TITLE " " PROGRAM_VERSION);// Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init GPS
  GPS.begin(9600);                              // 9600 NMEA is the default baud rate for Adafruit MTK GPS's
  delay(50);                                    // is delay really needed?
  GPS.sendCommand(PMTK_SET_BAUD_57600);         // set baud rate to 57600
  delay(50);
  GPS.begin(57600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); // turn on RMC (recommended minimum) and GGA (fix data) including altitude

  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    // 1 Hz update rate
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ); // Once every 5 seconds update

  // ----- report on our memory hogs
  char temp[200];
  Serial.println("Large resources:");
  snprintf(temp, sizeof(temp),
          ". baroModel.pressureStack[%d] uses %d bytes/entry = %d bytes total",
             maxReadings, sizeof(BaroReading), sizeof(baroModel.pressureStack));
  Serial.println(temp);

  // ----- init RTC
  // Note: See the main() loop. 
  //       The realtime clock is not available until after receiving a few NMEA sentences.

  // ----- restore barograph english/metric settings
  if (loadConfigUnits()) {
    Serial.println("Successfully loaded settings from non-volatile RAM");
  } else {
    Serial.println("Failed to load settings, re-initializing config file");
    saveConfigUnits();
  }

  // ----- restore barometric pressure history
  if (baroModel.loadHistory()) {
    Serial.println("Successfully restored barometric pressure history");
  } else {
    Serial.println("Failed to load barometric pressure history, re-initializing config file");
    baroModel.saveHistory();
  }

  // ----- init barometer
  if (baroModel.begin()) {
    // success
  } else {
    // failed to initialize hardware
    tft.setCursor(0, yText1);
    tft.setTextColor(cWARN);
    setFontSize(12);
    tft.println("Error!\n Unable to init\n  BMP388/390 sensor\n   check wiring");
    delay(4000);
  }

  // ----- run unit tests, if allowed by "#define RUN_UNIT_TESTS"
  #ifdef RUN_UNIT_TESTS
    initTestStepValues();
    //initTestSineWave();
  #endif

  // ----- all done with setup, show opening view screen
  delay(3000);                        // milliseconds
  clearScreen();

  redrawGraph = true;                 // draw graph
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
// Initialized to trigger all processing on first pass in mainline
uint32_t prevTimeGPS = 0;             // timer to process GPS sentence
uint32_t prevShowTime = 0;            // timer to update time-of-day (1 second)

time_t nextShowPressure = 0;          // timer to update displayed value (5 min), init to take a reading soon after startup
time_t nextSavePressure = 0;          // timer to log pressure reading (15 min)

const int RTC_PROCESS_INTERVAL = 1000;          // Timer RTC = 1 second
const int READ_BAROMETER_INTERVAL = 5*60*1000;  // Timer 1 =  5 minutes, in milliseconds
const int LOG_PRESSURE_INTERVAL = 15*60*1000;   // 15 minutes, in milliseconds

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

  // look for the first "setTime()" to begin the datalogger
  if (waitingForRTC && isDateValid(GPS.year, GPS.month, GPS.day)) {
    // found a transition from an unknown date -> correct date/time
    // assuming "class Adafruit_GPS" contains 2000-01-01 00:00 until 
    // it receives an update via NMEA sentences
    // the next step (1 second timer) will actually set the clock
    //redrawGraph = true;
    waitingForRTC = false;

    char msg[128];                    // debug
    Serial.println("Received first correct date/time from GPS");  // debug
    snprintf(msg, sizeof(msg), ". GPS time %d-%02d-%02d at %02d:%02d:%02d",
                                  GPS.year,GPS.month,GPS.day, 
                                  GPS.hour,GPS.minute,GPS.seconds);
    Serial.println(msg);              // debug
  }

  // every 1 second update the realtime clock
  uint32_t tempMillis = millis();   // only read RTC once, to avoid race conditions
  if (tempMillis - prevShowTime > RTC_PROCESS_INTERVAL) {
    prevShowTime = tempMillis;

    // update RTC from GPS
    if (isDateValid(GPS.year, GPS.month, GPS.day)) {
      
      setTime(GPS.hour, GPS.minute, GPS.seconds, GPS.day, GPS.month, GPS.year);
      //adjustTime(offset * SECS_PER_HOUR);  // todo - adjust to local time zone. for now, we only do GMT
    }

    // update display
    showTimeOfDay();
  }

  // every 5 minutes read current pressure
  // synchronize showReadings() on exactly 5-minute marks 
  // so the user can more easily predict when the next update will occur
  time_t rightnow = now();
  if ( rightnow >= nextShowPressure) {
    //nextShowPressure = nextFiveMinuteMark( rightnow );
    //nextShowPressure = nextOneMinuteMark( rightnow );
    nextShowPressure = nextOneSecondMark( rightnow );
  
    float pascals = baroModel.getBaroPressure();
    //redrawGraph = true;             // request draw graph
    printPressure( pascals );
  }

  // every 15 minutes read barometric pressure and save it in nonvolatile RAM
  if (rightnow >= nextSavePressure) {
    
    // log this pressure reading only if the time-of-day is correct and initialized 
    if (timeStatus() == timeSet) {
      baroModel.logPressure( rightnow );
      redrawGraph = true;             // request draw graph
      nextSavePressure = nextFifteenMinuteMark( rightnow );
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
      const int radius = 3;           // debug
      tft.fillCircle(touch.x, touch.y, radius, cTOUCHTARGET);  // debug - show dot
    #endif

    adjustUnits();                    // change between "inches mercury" and "millibars" units
  }

  // small activity bar crawls along bottom edge to give 
  // a sense of how frequently the main loop is executing
  showActivityBar(tft.height()-1, ILI9341_RED, cBACKGROUND);
}
