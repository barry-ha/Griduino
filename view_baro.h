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

            +-----------------------------------------+
            | *  date        Baroduino          hh:mm |
            |    #sat       29.97 inHg            ss  |
            | 30.5 +--------------------------------+ | <- yTop
            |      |          |          |          | |
            |      |          |          |          | |
            |      |          |          |          | |
            | 30.0 +  -  -  - - - -  -  -  -  -  -  + | <- yMid
            |      |          |          |          | |
            |      |          |          |          | |
            |      |          |          |   Today  | |
            | 29.5 +--------------------------------+ | <- yBot
            |         10/18       10/19      10/20    |
            +------:----------:----------:----------:-+
                   xDay1      xDay2      xDay3      xRight

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
extern BarometerModel baroModel;      // singleton instance of the barometer model

extern void showDefaultTouchTargets();// Griduino.ino
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
    void setMetric(bool metric);

  protected:
    // ---------- local data for this derived class ----------
    float fMaxPa = 102000;            // top bar of graph: highest pressure rounded UP to nearest multiple of 2000 Pa
    float fMinPa = 98000;             // bot bar of graph: lowest pressure rounded DOWN to nearest multiple of 2000 Pa

    float fMaxHg = 30.6;              // top axis of graph: upper bound of graph, inHg
    float fMinHg = 29.4;              // bot axis of graph: bound of graph, inHg

    // Use the following to control how much the initial graph fills the display.
    // Smaller numbers give bigger, more dynamic graph.
    // Larger numbers make the first graph look smaller and smoother. 
    const float PA_RES = 400.0;       // metric y-axis resolution, ie, nearest 2000 Pa (20 hPa)
    const float HG_RES = 0.2;         // english y-axis resolution, ie, nearest 0.2 inHg

    bool redrawGraph = true;          // true=request graph be drawn
    bool waitingForRTC = true;        // true=waiting for GPS hardware to give us the first valid date/time

    time_t nextShowPressure = 0;      // timer to update displayed value (5 min), init to take a reading soon after startup
    time_t nextSavePressure = 0;      // timer to log pressure reading (15 min)

    // ========== graph screen layout ==============================
    const int graphHeight = 160;          // in pixels
    const int pixelsPerHour = 4;          // 4 px/hr graph
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
    // these are names for the array indexes, must be named in same order as array below
    enum txtIndex {
      eTitle=0,
      eDate, eNumSat, eTimeHHMM, eTimeSS,
      valPressure, unitPressure,
    };

    #define numBaroFields 7
    TextField txtBaro[numBaroFields] = {
      // text            x,y    color       align       font
      {"Baroduino",     -1, 18, cTITLE,     ALIGNCENTER,eFONTSMALLEST}, // [eTitle] screen title, centered
      {"01-02",         48, 18, cWARN,      ALIGNLEFT,  eFONTSMALLEST}, // [eDate]
      {"0#",            48, 36, cWARN,      ALIGNLEFT,  eFONTSMALLEST}, // [eNumSat]
      {"12:34",        276, 18, cWARN,      ALIGNRIGHT, eFONTSMALLEST}, // [eTimeHHMM]
      {"56",           276, 36, cWARN,      ALIGNRIGHT, eFONTSMALLEST}, // [eTimeSS]
      {"30.000",       162, 46, ILI9341_WHITE, ALIGNRIGHT,eFONTSMALL},  // [valPressure]
      {"inHg",         180, 46, ILI9341_WHITE, ALIGNLEFT, eFONTSMALL},  // [unitPressure]
    };

    void showReadings() {
      clearScreen(yTop, graphHeight);     // erase only the graph area, not the whole screen, to reduce blinking

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
        TextField wait = TextField{ "Waiting for real-time clock", 12,100, cTITLE, ALIGNCENTER };
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
      txtBaro[eDate].print(msg);
    
      snprintf(msg, sizeof(msg), "%d#", GPS.satellites);
      txtBaro[eNumSat].print(msg);    // show number of satellites, help give sense of positional accuracy
    
      snprintf(msg, sizeof(msg), "%02d:%02d", hh,mm);
      txtBaro[eTimeHHMM].print(msg);  // show time, help identify when RTC stops
    
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
      if (model->gMetric) {
        fPressure = pascals / 100;
        sUnits = hPa;
        decimals = 1;
      } else {
        fPressure = pascals / PASCALS_PER_INCHES_MERCURY;
        sUnits = inHg;
        decimals = 3;
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
      Serial.print(": Minimum and maximum vertical scale = ");    printTwoFloats(fMinPa, fMaxPa);      Serial.println(" Pa");   // debug
      Serial.print(": Minimum and maximum reported pressure = "); printTwoFloats(lowestHg, highestHg); Serial.println(" inHg"); // debug
      Serial.print(": Minimum and maximum vertical scale = ");    printTwoFloats(fMinHg, fMaxHg);      Serial.println(" inHg"); // debug
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
        tft->drawLine(xDay1, y,  xDay1 + len,  y, cSCALECOLOR);  // mark left edge
        tft->drawLine(xRight,y,  xRight - len, y, cSCALECOLOR);  // mark right edge
      }
    }

    void superimposeLabel(int x, int y, double value, int precision) {
      // to save screen space, the scale numbers are written right on top of the vertical axis
      // x,y = lower left corner of text (baseline) to write
      Point ll = { x, y };

      // measure size of text that will be written
      int16_t x1, y1;
      uint16_t w, h;
      tft->getTextBounds("1234",  ll.x,ll.y,  &x1,&y1,  &w,&h);

      // erase area for text, plus a few extra pixels for border
      tft->fillRect(x1-2,y1-1,  w+4,h+2,  cBACKGROUND);
      //tft->drawRect(x1-2,y1-1,  w+4,h+2,  ILI9341_RED);  // debug

      tft->setCursor(ll.x, ll.y);
      tft->print(value, precision);
    }

    enum { eTODAY, eDATETODAY, eYESTERDAY, eDAYBEFORE };
    #define numDates 4
    TextField txtDate[numDates] = {
      TextField{"Today", xDay3+20,yBot-TEXTHEIGHT+2,  ILI9341_CYAN, 9}, // [eTODAY]
      TextField{"8/25",  xDay3+34,yBot+TEXTHEIGHT+2,  ILI9341_CYAN, 9}, // [eDATETODAY]
      TextField{"8/24",  xDay2+34,yBot+TEXTHEIGHT+2,  ILI9341_CYAN, 9}, // [eYESTERDAY]
      TextField{"8/23",  xDay1+34,yBot+TEXTHEIGHT+2,  ILI9341_CYAN, 9}, // [eDAYBEFORE]
    };

    void drawScale() {
      // draw horizontal lines
      const int yLine1 = yBot - graphHeight;
      const int yLine2 = yBot;
      const int yMid   = (yLine1 + yLine2)/2;
      tft->drawLine(xDay1, yLine1, xRight, yLine1, cSCALECOLOR);
      tft->drawLine(xDay1, yLine2, xRight, yLine2, cSCALECOLOR);
      for (int ii=xDay1; ii<xRight; ii+=10) {   // dotted line
        tft->drawPixel(ii, yMid, cSCALECOLOR);
        tft->drawPixel(ii+1, yMid, cSCALECOLOR);
      }
    
      // draw vertical lines
      tft->drawLine( xDay1,yBot - graphHeight,   xDay1,yBot, cSCALECOLOR);
      tft->drawLine( xDay2,yBot - graphHeight,   xDay2,yBot, cSCALECOLOR);
      tft->drawLine( xDay3,yBot - graphHeight,   xDay3,yBot, cSCALECOLOR);
      tft->drawLine(xRight,yBot - graphHeight,  xRight,yBot, cSCALECOLOR);
    
      // write limits of pressure scale in consistent units
      setFontSize(9);
      tft->setTextColor(ILI9341_CYAN);
      if (model->gMetric) {
        // metric: hecto-Pascal (hPa)
        superimposeLabel( MARGIN, yBot + TEXTHEIGHT/3,                 (fMinPa/100), 0);
        superimposeLabel( MARGIN, yBot - graphHeight + TEXTHEIGHT/3,   (fMaxPa/100), 0);
        superimposeLabel( MARGIN, yBot - graphHeight/2 + TEXTHEIGHT/3, (fMinPa + (fMaxPa - fMinPa)/2)/100, 0);
      } else {
        // english: inches mercury (inHg)
        superimposeLabel( MARGIN, yBot - TEXTHEIGHT/3,                 fMinHg, 1);
        superimposeLabel( MARGIN, yBot - graphHeight + TEXTHEIGHT/3,   fMaxHg, 1);
        superimposeLabel( MARGIN, yBot - graphHeight/2 + TEXTHEIGHT/3, (fMinHg + (fMaxHg - fMinHg)/2), 1);
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
      dateToString(sDate, sizeof(sDate), today);
    
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
    
      float yTopPa = (model->gMetric) ? fMaxPa : (fMaxHg*PASCALS_PER_INCHES_MERCURY);
      float yBotPa = (model->gMetric) ? fMinPa : (fMinHg*PASCALS_PER_INCHES_MERCURY);
    
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
          if (model->gMetric) {
            // todo
          }
          int y1 = map(baroModel.pressureStack[ii].pressure,  yBotPa,yTopPa,  yBot,yTop);
      
          // X-axis:
          //    Scale from timestamps onto x-axis
          time_t t1 = baroModel.pressureStack[ii].time;
          //       map(value, fromLow,fromHigh, toLow,toHigh)
          int x1 = map( t1,   minTime,maxTime,  xDay1,xRight);
    
          if (x1 < xDay1) {
            dateToString(sDate, sizeof(sDate), t1);
            snprintf(msg, sizeof(msg), "%d. Ignored: Date x1 (%s = %d) is off left edge of (%d).", 
                                        ii,                  sDate,x1,                   xDay1); 
            Serial.println(msg);          // debug
            continue;
          }
          if (x1 > xRight) {
            dateToString(sDate, sizeof(sDate), t1);
            snprintf(msg, sizeof(msg), "%d. Ignored: Date x1 (%s = %d) is off right edge of (%d).", 
                                        ii,                 sDate, x1,                   xRight); 
            Serial.println(msg);          // debug
            continue;
          }
    
          tft->drawPixel(x1,y1, cGRAPHCOLOR);
          int approxPa = (int)baroModel.pressureStack[ii].pressure;
          //snprintf(msg, sizeof(msg), "%d. Plot %d at pixel (%d,%d)", ii, approxPa, x1,y1);
          //Serial.println(msg);          // debug
        }
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
    nextShowPressure = nextOneMinuteMark( rightnow );
    //nextShowPressure = nextOneSecondMark( rightnow );  // debug
  
    float pascals = baroModel.getBaroPressure();  // get pressure
    printPressure( pascals );         // redraw text pressure reading
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

} // end updateScreen


void ViewBaro::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                // clear screen
  txtBaro[0].setBackground(this->background);         // set background for all TextFields in this view
  TextField::setTextDirty( txtBaro, numBaroFields );  // make sure all fields get re-printed on screen change

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();          // optionally draw boxes around button-touch area
  showScreenBorder();                 // optionally outline visible area
  showScreenCenterline();             // optionally draw visual alignment bar

  // ----- draw page title
  txtBaro[eTitle].print();

  redrawGraph = true;                 // make sure graph is drawn on entry
  updateScreen();                     // update UI immediately, don't wait for laggy mainline loop
} // end startScreen()


bool ViewBaro::onTouch(Point touch) {
  Serial.println("->->-> Touched baro screen.");

  bool handled = false;               // assume a touch target was not hit
  return handled;                     // true=handled, false=controller uses default action
} // end onTouch()
