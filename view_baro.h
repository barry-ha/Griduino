/*
  File:     view_baro.cpp

  Barometer - a graphing 3-day barometer

  Version history:
            2021-02-01 merged from examples/baroduino into Griduino view
            2021-01-30 added support for BMP390 and latest Adafruit_BMP3XX library, v0.31
            2020-09-18 added datalogger to nonvolatile RAM
            2020-08-27 save unit setting (english/metric) in nonvolatile RAM
            2019-12-18 created from example by John KM7O

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  What can you do with a barometer that knows the time of day? Graph it!
            This program timestamps each reading and stores a history.
            It graphs the most recent 3 days. 
            It saves the readings in non-volatile memory and re-displays them on power-up.
            Its RTC (realtime clock) is updated from the GPS satellite network.

            +-----------------------------------+
            | date        Baroduino       hh:mm |
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
            |        10/18    10/19    10/20    |
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

*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>         // TFT color display library
#include <TimeLib.h>                  // BorisNeubert / Time (who forked it from PaulStoffregen / Time)
#include "constants.h"                // Griduino constants and colors
#include "model_gps.h"                // Model of a GPS for model-view-controller
#include "model_baro.h"               // Model of a barometer that stores 3-day history
#include "save_restore.h"             // Save configuration in non-volatile RAM
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "view.h"                     // Base class for all views

// ========== extern ===========================================
extern Model* model;                  // "model" portion of model-view-controller
extern Adafruit_BMP3XX baro;          // singleton instance to manage barometer hardware
extern BarometerModel baroModel;      // singleton instance of the barometer model

extern void showDefaultTouchTargets();  // Griduino.ino
extern void getDate(char* result, int maxlen);  // model_gps.h
extern bool isDateValid(int yy, int mm, int dd);  // Griduino.ino
extern time_t nextOneSecondMark(time_t timestamp);      // Griduino.ino
extern time_t nextOneMinuteMark(time_t timestamp);      // Griduino.ino
extern time_t nextFiveMinuteMark(time_t timestamp);     // Griduino.ino
extern time_t nextFifteenMinuteMark(time_t timestamp);  // Griduino.ino

// ========== class ViewBaro ===================================
class ViewBaro : public View {
  public:
    // ---------- public interface ----------
    // This derived class must implement the public interface:
    ViewBaro(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    {
      background = cBACKGROUND;       // every view can have its own background color
    }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);

  protected:
    // ---------- local data for this derived class ----------
    bool redrawGraph = true;          // true=request graph be drawn
    bool waitingForRTC = true;        // true=waiting for GPS hardware to give us the first valid date/time

    time_t nextShowPressure = 0;      // timer to update displayed value (5 min), init to take a reading soon after startup
    time_t nextSavePressure = 0;      // timer to log pressure reading (15 min)

    enum eUnits { eMetric, eEnglish };
    int gUnits = eEnglish;            // units on startup: 0=english=inches mercury, 1=metric=millibars

    // ----- graph screen layout
    // todo: are these single-use? can they be moved inside a function?
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

    // ----- text screen layout
    // these are names for the array indexes, must be named in same order as array below
    enum txtIndex {
      eTitle=0, 
      eDate, eNumSat, eTimeHHMM, eTimeSS,
      valPressure, unitPressure,
    };

    /*
    const int MARGIN = 6;             // reserve an outer blank margin on all sides
    const int xIndent = 12;           // in pixels, text on main screen
    const int yText1 = MARGIN+12;     // in pixels, top row, main screen
    const int yText2 = yText1 + 28;
    */
    #define numBaroFields 7
    TextField txtBaro[numBaroFields] = {
      // text            x,y    color             index
      {"Baroduino",     -1, 18, cTITLE,     ALIGNCENTER,eFONTSMALLEST}, // [eTitle] program title, centered
      {"01-02",         48, 18, cWARN,      ALIGNLEFT,  eFONTSMALLEST}, // [eDate]
      {"0#",            48, 36, cWARN,      ALIGNLEFT,  eFONTSMALLEST}, // [eNumSat]
      {"12:34",        276, 18, cWARN,      ALIGNRIGHT, eFONTSMALLEST}, // [eTimeHHMM]
      {"56",           276, 36, cWARN,      ALIGNRIGHT, eFONTSMALLEST}, // [eTimeSS]
      {"30.00",        162, 46, ILI9341_WHITE, ALIGNRIGHT,eFONTSMALL},  // [valPressure]
      {"inHg",         180, 46, ILI9341_WHITE, ALIGNLEFT, eFONTSMALL},  // [unitPressure]
    };

    void showReadings() {
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
      txtBaro[eDate].print(msg);       // 2020-11-12 do show date, help identify when RTC stops
    
      snprintf(msg, sizeof(msg), "%d#", GPS.satellites);
      txtBaro[eNumSat].print(msg);     // 2020-11-12 do show date, help identify when RTC stops
    
      snprintf(msg, sizeof(msg), "%02d:%02d", hh,mm);
      txtBaro[eTimeHHMM].print(msg);
    
      snprintf(msg, sizeof(msg), "%02d", ss);
      txtBaro[eTimeSS].print(msg);
    }

    // ----- baro helpers
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
        decimals = 1;
      }
    
      txtBaro[eTitle].print();
      txtBaro[valPressure].print( fPressure, decimals );
      txtBaro[unitPressure].print( sUnits );
      //Serial.print("Displaying "); Serial.print(fPressure, decimals); Serial.print(" "); Serial.println(sUnits);
    }
    
    void tickMarks(int t, int h) {
      // draw tick marks for the horizontal axis
      // input: t = hours
      //        h = height of tick mark, pixels
      int deltaX = t * pixelsPerHour;
      for (int x=xRight; x>xDay1; x = x - deltaX) {
        tft->drawLine(x,yBot,  x,yBot - h, cSCALECOLOR);
      }
    }


};  // end class ViewBaro

// ============== implement public interface ================
//=========== main work loop ===================================
void ViewBaro::updateScreen() {
  // called on every pass through main()

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

  // update display
  showTimeOfDay();

  // every 5 minutes read current pressure
  // synchronize showReadings() on exactly 5-minute marks 
  // so the user can more easily predict when the next update will occur
  time_t rightnow = now();
  if ( rightnow >= nextShowPressure) {
    //nextShowPressure = nextFiveMinuteMark( rightnow );
    //nextShowPressure = nextOneMinuteMark( rightnow );
    nextShowPressure = nextOneSecondMark( rightnow );
  
    float pascals = baroModel.getBaroData();
    //redrawGraph = true;             // request draw graph
    printPressure( pascals );
  }
}

void ViewBaro::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                // clear screen
  txtBaro[0].setBackground(this->background);         // set background for all TextFields in this view
  TextField::setTextDirty( txtBaro, numBaroFields );  // make sure all fields get re-printed on screen change

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();          // optionally draw boxes around button-touch area
  showScreenBorder();                 // optionally outline visible area

  // ----- draw page title
  txtBaro[eTitle].print();

  updateScreen();                     // fill in values immediately, don't wait for the main loop to eventually get around to it

  #ifdef SHOW_SCREEN_CENTERLINE
    // show centerline at      x1,y1              x2,y2             color
    tft->drawLine( tft->width()/2,0,  tft->width()/2,tft->height(), cWARN); // debug
  #endif
}

bool ViewBaro::onTouch(Point touch) {
  Serial.println("->->-> Touched time screen.");
  bool handled = false;               // assume a touch target was not hit
  return handled;                     // true=handled, false=controller uses default action
}
