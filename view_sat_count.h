#pragma once   // Please format this file with clang before check-in to GitHub
/*
   File:    view_sat_count.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Graph the number of satellites recently acquired.
            We want to compare several Griduino side-by-side to
            evaluate how sensitive and consistent any given GPS
            sensor is reporting satellites.

            +-----------------------------------+
            | *      GPS Satellite Count      > |...yRow1
            |    +---------------------------+  |...yTop
            |10+ |               X           |  |...yRow10  green
            | 8  |        X      X      X    |  |...yRow8   green
            | 6  | X      X      X      X    |  |...yRow6   green
            | 4  | X      X      X      X    |  |...yRow4   yellow
            | 2  | X      X      X      X    |  |...yRow2   yellow
            | 0  | X      X      X      X    |  |...yRow0   red
            |    +---------------------------+  |...yBot
            |   19:10  19:15  19:20  19:25      |
            +-:-::---------------------------:--+
              : :xLeft                       xRight
         labelX valueX
*/

#include <Adafruit_ILI9341.h>            // TFT color display library
#include "constants.h"                   // Griduino constants and colors
#include <elapsedMillis.h>               // Scheduling intervals in main loop
#include "logger.h"                      // conditional printing to Serial port
#include "date_helper.h"                 // date/time conversions
#include "view.h"                        // Base class for all views
#include "Embedded_Template_Library.h"   // Required for any etl import using Arduino IDE
#include "etl/circular_buffer.h"         // Embedded Template Library

// ========== extern ===========================================
extern Dates date;   // for "datetimeToString()", Griduino.ino

// ========== struct for each item in the circular buffer
struct satCountItem {
  time_t tm;   // seconds since Jan 1, 1970
  int numSats;
};

// ========== class ViewSatCount =================================
class ViewSatCount : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewSatCount(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background = cBACKGROUND;   // every view can have its own background color
  }
  void updateScreen();
  void startScreen();
  bool onTouch(Point touch);

  etl::circular_buffer<satCountItem, 19> cbSats;
  etl::circular_buffer<satCountItem, 19>::iterator cbIter;

  // pushes a value to the back of the circular buffer
  void push(time_t tm, int nSats) {
    // nSats = random(0, 19);   // unit test: this replaces measurements with something to scroll across the screen
    satCountItem item = {tm, nSats};
    cbSats.push(item);
    graphRefreshRequested = true;
  }

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h
  bool graphRefreshRequested;   // true = new data arrived in circular buffer, refresh the bar graph

  // ========== text screen layout ===================================

  // vertical placement of text rows
  const int space = 30;
  const int half  = space / 2;

  const int yRow1  = 18;
  const int yRow10 = yRow1 + space + 8;
  const int yRow8  = yRow10 + 25;
  const int yRow6  = yRow8 + 25;
  const int yRow4  = yRow6 + 25;
  const int yRow2  = yRow4 + 25;
  const int yRow0  = yRow2 + 25;

  const int labelX = 4;    // legend for vertical axis
  const int valueX = 52;   // left-align values

  const int label2 = 100;
  const int value2 = 118;

  // canvas for bar graph, in screen coordinates
  const int xLeft  = 40;    // pixels
  const int xRight = 306;   // = (#bars) * (px/bar) + xLeft
                            // = (19) * (14) + 40
  const int yTop = 42;
  const int yBot = 184;

  // ----- screen text
  // names for the array indexes, must be named in same order as array below
  enum txtIndex {
    TITLE = 0,
    eDate,
    eNumSat,
    eTimeHHMM,
    eTimeSS,
    TEN,
    EIGHT,
    SIX,
    FOUR,
    TWO,
    ZERO,
  };

  // ----- static + dynamic screen text
  // clang-format off
#define nSatCountValues 11
  TextField txtValues[nSatCountValues] = {
      {"Satellites", -1, yRow1,cTITLE,         ALIGNCENTER, eFONTSMALLEST},   // [TITLE] view title, centered
      {"01-02",    48, 18,     cWARN,          ALIGNLEFT,  eFONTSMALLEST},    // [eDate]
      {"0#",       48, 36,     cWARN,          ALIGNLEFT,  eFONTSMALLEST},    // [eNumSat]
      {"12:34",   276, 18,     cWARN,          ALIGNRIGHT, eFONTSMALLEST},    // [eTimeHHMM]
      {"56",      276, 36,     cWARN,          ALIGNRIGHT, eFONTSMALLEST},    // [eTimeSS]
      {"10+",  labelX, yRow10, ILI9341_GREEN,  ALIGNLEFT,  eFONTSMALLEST},    // [TEN]
      {" 8",   labelX, yRow8,  ILI9341_GREEN,  ALIGNLEFT,  eFONTSMALLEST},    // [EIGHT]
      {" 6",   labelX, yRow6,  ILI9341_GREEN,  ALIGNLEFT,  eFONTSMALLEST},    // [SIX]
      {" 4",   labelX, yRow4,  ILI9341_GREEN,  ALIGNLEFT,  eFONTSMALLEST},    // [FOUR]
      {" 2",   labelX, yRow2,  ILI9341_YELLOW, ALIGNLEFT,  eFONTSMALLEST},    // [TWO]
      {" 0",   labelX, yRow0,  ILI9341_RED,    ALIGNLEFT,  eFONTSMALLEST},    // [ZERO]
  };
  // clang-format on

  void showBar(int position, satCountItem item) {
    int value = item.numSats;

#define barWidth  14   // total width including gutter
#define barGutter 2    // divider between bars
    // map(value, fromLow, fromHigh, toLow, toHigh)
    int tt = map(value, 0, 10, yBot, yTop);   // top of bar, screen coords
    tt     = max(tt, yTop);                   // top of bar must remain on canvas
    int hh = yBot - tt;                       // height of bar, pixels
    int xx = xLeft + position * barWidth;     // left edge of bar

    int color;
    if (value <= 1) {
      color = ILI9341_RED;
    } else if (value <= 3) {
      color = ILI9341_YELLOW;
    } else {
      color = ILI9341_GREEN;
    }
    tft->fillRect(xx, tt, (barWidth - barGutter), hh, color);

    // make sure text is always drawn inside canvas
    if (value < 10) {
      // small values are drawn above bar in white
      tft->setCursor(xx + 2, tt - 2);
      tft->setTextColor(ILI9341_WHITE);
      tft->print(value);
    } else {
      // large values are drawn on top of bar in contrasting color
      tft->setTextColor(ILI9341_BLACK);
      int valueTens = value / 10;
      int valueOnes = value % 10;

      tft->setCursor(xx + 1, tt + 13);
      tft->print(valueTens);
      tft->setCursor(xx + 1, tt + 29);
      tft->print(valueOnes);
    }
  }

  void showTimeOfDay() {
    // fetch RTC and display it on screen
    char msg[12];   // strlen("12:34:56") = 8
    int mo, dd, hh, mm, ss;
    if (timeStatus() == timeNotSet) {
      mo = dd = hh = mm = ss = 0;
    } else {
      time_t tt = now();
      mo        = month(tt);
      dd        = day(tt);
      hh        = hour(tt);
      mm        = minute(tt);
      ss        = second(tt);
    }

    snprintf(msg, sizeof(msg), "%d-%02d", mo, dd);
    txtValues[eDate].print(msg);

    snprintf(msg, sizeof(msg), "%d#", GPS.satellites);
    txtValues[eNumSat].print(msg);   // show number of satellites

    snprintf(msg, sizeof(msg), "%02d:%02d", hh, mm);
    txtValues[eTimeHHMM].print(msg);   // show time, help identify when RTC stops

    snprintf(msg, sizeof(msg), "%02d", ss);
    txtValues[eTimeSS].print(msg);
  }

};   // end class ViewSatCount

// ============== implement public interface ================
void ViewSatCount::updateScreen() {
  // update display
  showTimeOfDay();

  // called on every pass through main(), but only update bar graph once/2second
  if (graphRefreshRequested) {
    graphRefreshRequested = false;

    // erase the canvas
    tft->fillRect(xLeft, yTop, (xRight - xLeft), (yBot - yTop), this->background);

    // update entire bar graph
    int bar = 0;
    for (cbIter = cbSats.begin(); cbIter < cbSats.end(); ++cbIter) {
      showBar(bar, *cbIter);
      bar++;
    }

    // update all x-axis labels
    tft->fillRect(xLeft, yBot + 2, (xRight - xLeft), (gScreenHeight - (yBot + 2) - 1), this->background);
    int savedRotation = tft->getRotation();
    int newRotation   = (savedRotation + 3) % 4;
    tft->setRotation(newRotation);   // set portrait mode

    tft->setTextColor(cHIGHLIGHT, this->background);
    setFontSize(eFONTSMALLEST);

    bar = 0;
    for (cbIter = cbSats.begin(); cbIter < cbSats.end(); ++cbIter) {

      if ((bar + 1) % 2) {   // draw label every even-numbered bar, or text is too crowded
        tft->setCursor(2, valueX + bar * barWidth);
        // convert timestamp to string
        char msg[6];   // strlen("hh:mm") = 5;
        satCountItem item = *cbIter;
        date.timeToString(msg, sizeof(msg), item.tm);
        tft->print(msg);
      }

      bar++;
    }
    tft->setRotation(savedRotation);   // restore screen orientation for horiz text
  }

}   // end updateScreen

void ViewSatCount::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                   // clear screen
  txtValues[0].setBackground(this->background);          // set background for all TextFields in this view
  TextField::setTextDirty(txtValues, nSatCountValues);   // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALL);

  drawAllIcons();              // draw gear (settings) and arrow (next screen)
  //showDefaultTouchTargets();   // optionally draw box around default button-touch areas
  //showMyTouchTargets(0, 0);    // no real buttons on this view
  //showScreenBorder();          // optionally outline visible area
  //showScreenCenterline();      // optionally draw visual alignment bar

  // ----- draw border for canvas of bar graph
  tft->drawRect(xLeft - 1, yTop - 1, (xRight - xLeft) + 2, (yBot - yTop) + 2, ILI9341_CYAN);

  // ----- draw fields that have static text
  for (int ii = 0; ii < nSatCountValues; ii++) {
    txtValues[ii].print();
  }

  graphRefreshRequested = true;
  //logger.fencepost("--- startScreen()", __LINE__);
  updateScreen();   // update UI immediately, don't wait for the main loop to eventually get around to it
}

bool ViewSatCount::onTouch(Point touch) {
  return false;   // true=handled, false=controller will run default action
}   // end onTouch()
