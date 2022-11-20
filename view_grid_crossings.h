#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     view_grid_crossing.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  List the time that you spend in each grid.
            Note the 'exit' time of one grid typically equals
            the 'enter' time of the next grid.

            +-------------------------------------------+
            |  *         Grid Crossing Log            > |...yRow1
            |                                           |...yRow2 (unused)
            | Grid   Enter         Exit                 |...yRow3
            | CN77   7/25 1234z    -                    |...yRow4
            | CN88   7/24 0813z    7/25 1234z  0d 13.7h |...yRow5
            | CN98   7/20 1942z    7/24 1015z  4d 19.1h |...yRow6
            | CN99   -             7/01 1234z           |...yRow7
            |                                           |
            |      Aug 22, 2022  08:23:17 GMT           |...yRowBot
            +-------------------------------------------+
              :      :             :           :
              xGrid  xEnter        xExit       xDuration
*/

#include <Arduino.h>            //
#include <Adafruit_ILI9341.h>   // TFT color display library
#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "grid_helper.h"        // lat/long conversion routines
#include "date_helper.h"        // date/time conversions
#include "model_gps.h"          // Model of a GPS for model-view-controller
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
extern Location history[];               // Griduino.ino, GPS breadcrumb trail
extern const int numHistory;             // Griduino.ino, number of elements in history[]
extern void showDefaultTouchTargets();   // Griduino.ino
extern Logger logger;                    // Griduino.ino
extern Grids grid;                       // grid_helper.h
extern Adafruit_ILI9341 tft;             // Griduino.ino
extern Model *model;                     // "model" portion of model-view-controller

// ========== class ViewGridCrossing ===========================
class ViewGridCrossings : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewGridCrossings(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background = cBACKGROUND;   // every view can have its own background color
  }
  void updateScreen() {
    // called on every pass through main()
    // our only dynamic item is "time in current grid" on the top row

    // ----- [GRID1] data is our current location
    // find the time that we entered this grid
    // - examine the breadcrumb history file
    // - start from most recent entry, working our way backwards in time
    // - if the data is still in this grid, then continue
    // - if the data is a different grid, then update [GRID1]

    int currentIndex = previousItem(model->nextHistoryItem);
    Location item    = history[currentIndex];

    char currentGrid4[5];
    grid.calcLocator(currentGrid4, item.loc.lat, item.loc.lng, 4);
    logger.info("Current grid = ", currentGrid4);   // debug

    for (int ii = 0; ii < numHistory; ii++) {
      item = history[currentIndex];
      if (!item.isEmpty()) {
        char grid4[5];
        grid.calcLocator(grid4, item.loc.lat, item.loc.lng, 4);

        char temp[128];   // debug
        snprintf(temp, sizeof(temp), "Comparing slot %d grid %s to current grid %s", currentIndex, grid4, currentGrid4);
        logger.debug(temp);   // debug

        if (strcmp(grid4, currentGrid4) == 0) {
          // same grid, ignore, keep looking
        } else {
          // the next older slot is a different grid, so write this grid-crossing info to the screen
          // todo: this is just [GRID1] so extrapolate for the next row
          time_t enterTimestamp = item.timestamp;
          time_t exitTimestamp  = now();   // todo: this works only for [GRID1]
          showGridCrossing(GRID1, currentGrid4, enterTimestamp, exitTimestamp);
          break;   // debug
        }
      }
      currentIndex = previousItem(currentIndex);
    }

    // ----- GMT date & time
    char sDate[15];   // strlen("Aug 26, 2022") = 13
    char sTime[10];   // strlen("19:54:14") = 8
    model->getDate(sDate, sizeof(sDate));
    model->getTime(sTime);
    txtFields[GMT_DATE].print(sDate);
    txtFields[GMT_TIME].print(sTime);
    txtFields[GMT].print();
  }
  void startScreen() {
    // called once each time this view becomes active

    this->clearScreen(this->background);            // clear screen
    txtFields[0].setBackground(this->background);   // set background for all TextFields in this view
#define nCrossingsFields 38
    TextField::setTextDirty(txtFields, nCrossingsFields);   // make sure all fields get re-printed on screen change

    // ----- draw text fields
    for (int ii = 0; ii < nCrossingsFields; ii++) {
      txtFields[ii].print();
    }

    drawAllIcons();              // draw gear (settings) and arrow (next screen)
    showDefaultTouchTargets();   // optionally draw boxes around button-touch area
    // showMyTouchTargets(Buttons, nButtons);   // no buttons on this view
    showScreenBorder();       // optionally outline visible area
    showScreenCenterline();   // optionally draw visual alignment bar
    updateScreen();           // fill in values immediately, don't wait for the main loop to eventually get around to it
  }
  bool onTouch(Point touch) {
    return false;   // this view has no buttons, false=controller uses default action
  }

  // human friendly "elapsed time" helper
  // from time1 to time2 (where time1 <= time2)
  // this method is 'public' so it is callable from unit tests
  void calcTimeDiff(char *msg, int sizeMsg, time_t time1, time_t time2) {
    int timeDiff = time2 - time1;   // seconds
    if (timeDiff < SECS_PER_MIN) {
      // ------------------------------------------
      // show seconds, 0s ... 59s
      // ------------------------------------------
      snprintf(msg, sizeMsg, "%ds", timeDiff);

    } else if (timeDiff < (SECS_PER_MIN * 91)) {
      // ------------------------------------------
      // show minutes, 1m ... 90m
      // ------------------------------------------
      int elapsed = (unsigned long)timeDiff / SECS_PER_MIN;   // typecast integer for printing
      snprintf(msg, sizeMsg, "%dm", elapsed);

    } else if (timeDiff < (SECS_PER_DAY * 2)) {
      // ------------------------------------------
      // show fractional hours, 1.5h ... 47.9h
      // ------------------------------------------
      int eHours10    = timeDiff * 10 / SECS_PER_HOUR;
      float fHours    = eHours10 / 10.0;
      char sHours[8];
      floatToCharArray(sHours, sizeof(sHours), fHours, 1);
      snprintf(msg, sizeMsg, "%sh", sHours);

    } else if (timeDiff < (SECS_PER_DAY * 100)) {
      // ------------------------------------------
      // show fractional days, 2.0d ... 99.9d
      // ------------------------------------------
      int eDays = timeDiff * 10 / SECS_PER_DAY;
      float fDays = eDays / 10.0;
      char sDays[8];
      floatToCharArray(sDays, sizeof(sDays), fDays, 1);
      snprintf(msg, sizeMsg, "%sd", sDays);

    } else {
      // ------------------------------------------
      // show whole days, 100d ... 9999d
      // ------------------------------------------
      int elapsed = (unsigned long)timeDiff / SECS_PER_DAY;   // typecast integer for printing
      snprintf(msg, sizeMsg, "%dd", elapsed);
    }
  }

protected:
  // ---------- local data for this derived class ----------
  // vertical placement of text rows
  const int space = 27;
  const int half  = space / 2;

  const int yRow1   = 18;
  const int yRow2   = yRow1 + space;
  const int yRow3   = yRow2 + half;
  const int yRow4   = yRow3 + space;
  const int yRow5   = yRow4 + space;
  const int yRow6   = yRow5 + space;
  const int yRow7   = yRow6 + space;
  const int yRow8   = yRow7 + space;
  const int yRowBot = 226;   // GMT date on bottom row, "y=226" will match other views

  const int xGrid      = 6;
  const int xEnterCL   = 110;   // center line for "enter grid"
  const int xEnterDate = xEnterCL - 7;
  const int xEnterTime = xEnterCL + 6;
  const int xExitCL    = 214;   // center line for "exit grid"
  const int xExitDate  = xExitCL - 7;
  const int xExitTime  = xExitCL + 6;
  const int xDuration  = gScreenWidth - 6;

  // ----- screen text
  // names for the array indexes, must be named in same order as array below
  enum txtIndex {
    TITLE = 0,
    GRID, ENTER, EXIT, DURATION,
    GRID1, DATE1IN, TIME1IN, DATE1OUT, TIME1OUT, ET1,
    GRID2, DATE2IN, TIME2IN, DATE2OUT, TIME2OUT, ET2,
    GRID3, DATE3IN, TIME3IN, DATE3OUT, TIME3OUT, ET3,
    GRID4, DATE4IN, TIME4IN, DATE4OUT, TIME4OUT, ET4,
    GRID5, DATE5IN, TIME5IN, DATE5OUT, TIME5OUT, ET5,
    GMT_DATE, GMT_TIME, GMT
  };

  // ----- static + dynamic screen text
  TextField txtFields[nCrossingsFields] = {
      {"Grid Crossing Log", -1, yRow1, cTITLE, ALIGNCENTER, eFONTSMALLEST},    // [TITLE] view title, centered
      {"Grid", xGrid, yRow3, cTEXTCOLOR, ALIGNLEFT, eFONTSMALLEST},            // [GRID]
      {"Enter", xEnterCL - 32, yRow3, cTEXTCOLOR, ALIGNLEFT, eFONTSMALLEST},   // [ENTER]
      {"Exit", xExitCL - 30, yRow3, cTEXTCOLOR, ALIGNLEFT, eFONTSMALLEST},     // [EXIT]
      {"Elapsed", xDuration, yRow3, cTEXTCOLOR, ALIGNRIGHT, eFONTSMALLEST},    // [DURATION]

      {"CN86", xGrid, yRow4, cVALUE, ALIGNLEFT, eFONTSMALLEST},        // [GRID1]
      {"2/1", xEnterDate, yRow4, cVALUE, ALIGNRIGHT, eFONTSMALLEST},   // dummy data
      {"0822", xEnterTime, yRow4, cVALUE, ALIGNLEFT, eFONTSMALLEST},
      {"-", xExitDate, yRow4, cVALUE, ALIGNRIGHT, eFONTSMALLEST},
      {"", xExitTime, yRow4, cVALUE, ALIGNLEFT, eFONTSMALLEST},
      {"30s", xDuration, yRow4, cVALUE, ALIGNRIGHT, eFONTSMALLEST},

      {"CN85", xGrid, yRow5, cVALUE, ALIGNLEFT, eFONTSMALLEST},        // [GRID2]
      {"2/1", xEnterDate, yRow5, cVALUE, ALIGNRIGHT, eFONTSMALLEST},   // dummy data
      {"0723", xEnterTime, yRow5, cVALUE, ALIGNLEFT, eFONTSMALLEST},
      {"2/1", xExitDate, yRow5, cVALUE, ALIGNRIGHT, eFONTSMALLEST},
      {"0822", xExitTime, yRow5, cVALUE, ALIGNLEFT, eFONTSMALLEST},
      {"59m", xDuration, yRow5, cVALUE, ALIGNRIGHT, eFONTSMALLEST},

      {"CN84", xGrid, yRow6, cVALUE, ALIGNLEFT, eFONTSMALLEST},         // [GRID3]
      {"1/31", xEnterDate, yRow6, cVALUE, ALIGNRIGHT, eFONTSMALLEST},   // dummy data
      {"0823", xEnterTime, yRow6, cVALUE, ALIGNLEFT, eFONTSMALLEST},
      {"2/1", xExitDate, yRow6, cVALUE, ALIGNRIGHT, eFONTSMALLEST},
      {"0723", xExitTime, yRow6, cVALUE, ALIGNLEFT, eFONTSMALLEST},
      {"47.9h", xDuration, yRow6, cVALUE, ALIGNRIGHT, eFONTSMALLEST},

      {"CN83", xGrid, yRow7, cVALUE, ALIGNLEFT, eFONTSMALLEST},        // [GRID4]
      {"2/1", xEnterDate, yRow7, cVALUE, ALIGNRIGHT, eFONTSMALLEST},   // dummy data
      {"0823", xEnterTime, yRow7, cVALUE, ALIGNLEFT, eFONTSMALLEST},
      {"1/31", xExitDate, yRow7, cVALUE, ALIGNRIGHT, eFONTSMALLEST},
      {"0823", xExitTime, yRow7, cVALUE, ALIGNLEFT, eFONTSMALLEST},
      {"99.9d", xDuration, yRow7, cVALUE, ALIGNRIGHT, eFONTSMALLEST},

      {"CN82", xGrid, yRow8, cVALUE, ALIGNLEFT, eFONTSMALLEST},          // [GRID5]
      {"11/21", xEnterDate, yRow8, cVALUE, ALIGNRIGHT, eFONTSMALLEST},   // dummy data
      {"0111", xEnterTime, yRow8, cVALUE, ALIGNLEFT, eFONTSMALLEST},
      {"12/22", xExitDate, yRow8, cVALUE, ALIGNRIGHT, eFONTSMALLEST},
      {"0823", xExitTime, yRow8, cVALUE, ALIGNLEFT, eFONTSMALLEST},
      {"999d", xDuration, yRow8, cVALUE, ALIGNRIGHT, eFONTSMALLEST},

      {"Jan 01, 2001", 130, yRowBot, cFAINT, ALIGNRIGHT, eFONTSMALLEST},   // [GMT_DATE]
      {"02:34:56", 148, yRowBot, cFAINT, ALIGNLEFT, eFONTSMALLEST},        // [GMT_TIME]
      {"GMT", 232, yRowBot, cFAINT, ALIGNLEFT, eFONTSMALLEST},             // [GMT]
  };

  // Iterator helper
  int previousItem(int ii) {
    return (ii > 0) ? (ii - 1) : (numHistory - 1);
  }

  // helper to show one row on screen display
  void showGridCrossing(int row, char *grid4, time_t enterTime, time_t exitTime) {
    // [GRID]
    txtFields[row + 0].print(grid4);
    char msg[32];
    // date enter
    snprintf(msg, sizeof(msg), "%d/%d", month(enterTime), day(enterTime));
    txtFields[row + 1].print(msg);
    // time enter
    snprintf(msg, sizeof(msg), "%02d%02d", hour(enterTime), minute(enterTime));
    txtFields[row + 2].print(msg);
    // date exit
    snprintf(msg, sizeof(msg), "%d/%d", month(exitTime), day(exitTime));
    txtFields[row + 3].print(msg);
    // time exit
    snprintf(msg, sizeof(msg), "%02d%02d", hour(exitTime), minute(exitTime));
    txtFields[row + 4].print(msg);
    // elapsed time
    calcTimeDiff(msg, sizeof(msg), enterTime, exitTime);
    txtFields[row + 5].print(msg);
  }

};   // end class ViewGridCrossings

