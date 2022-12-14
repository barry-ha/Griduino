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

  struct CrossingInfo {
    char grid4[5];           // e.g. "CN87"
    time_t enterTimestamp;   // seconds
    time_t exitTimestamp;    // seconds
    bool isSet;            // true=real data, false=uninitialized
  };
  CrossingInfo timeInGrid[5] = {
      {"A", 0, 0, false},   // [GRID1]
      {"B", 0, 0, false},   // [GRID2]
      {"C", 0, 0, false},   // [GRID3]
      {"D", 0, 0, false},   // [GRID4]
      {"E", 0, 0, false},   // [GRID5]
  };

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
    // logger.info("Current grid = ", currentGrid4);   // debug

    extractGridCrossings(timeInGrid);   // read GPS history array
    showGridCrossings(timeInGrid);      // show result on screen

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
      int eHours10 = timeDiff * 10 / SECS_PER_HOUR;
      float fHours = eHours10 / 10.0;
      char sHours[8];
      floatToCharArray(sHours, sizeof(sHours), fHours, 1);
      snprintf(msg, sizeMsg, "%sh", sHours);

    } else if (timeDiff < (SECS_PER_DAY * 100)) {
      // ------------------------------------------
      // show fractional days, 2.0d ... 99.9d
      // ------------------------------------------
      int eDays   = timeDiff * 10 / SECS_PER_DAY;
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
  const int xEnterCL   = 158;   // center line for "enter grid"
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
    {"Grid Crossing Log", -1, yRow1, cTITLE, ALIGNCENTER, eFONTSMALLEST},            // [TITLE] view title, centered
    {"Grid", xGrid, yRow3, cTEXTCOLOR, ALIGNLEFT, eFONTSMALLEST},                    // [GRID]
    {"Entered Grid", xEnterCL - 70, yRow3, cTEXTCOLOR, ALIGNLEFT, eFONTSMALLEST},    // [ENTER]
    {"", xExitCL - 28, yRow3, cTEXTCOLOR, ALIGNLEFT, eFONTSMALLEST},                 // [EXIT] unused
    {"Time in Grid", xDuration + 2, yRow3, cTEXTCOLOR, ALIGNRIGHT, eFONTSMALLEST},   // [DURATION]

    {"CN86", xGrid, yRow4, cVALUE, ALIGNLEFT, eFONTSMALLEST},             // [GRID1]
    {"2-1-2022", xEnterDate, yRow4, cVALUE, ALIGNRIGHT, eFONTSMALLEST},   // [DATE1IN]
    {"0822", xEnterTime, yRow4, cVALUE, ALIGNLEFT, eFONTSMALLEST},        // [TIME1IN]
    {"", xExitDate, yRow4, cVALUE, ALIGNRIGHT, eFONTSMALLEST},            // [DATE1OUT] unused
    {"", xExitTime, yRow4, cVALUE, ALIGNLEFT, eFONTSMALLEST},             // [TIME1OUT] unused
    {"30s", xDuration, yRow4, cVALUE, ALIGNRIGHT, eFONTSMALLEST},         // [DURATION]

    {"CN85", xGrid, yRow5, cVALUE, ALIGNLEFT, eFONTSMALLEST},              // [GRID2]
    {"1-31-2022", xEnterDate, yRow5, cVALUE, ALIGNRIGHT, eFONTSMALLEST},   // dummy data
    {"0723", xEnterTime, yRow5, cVALUE, ALIGNLEFT, eFONTSMALLEST},
    {"", xExitDate, yRow5, cVALUE, ALIGNRIGHT, eFONTSMALLEST},
    {"", xExitTime, yRow5, cVALUE, ALIGNLEFT, eFONTSMALLEST},
    {"59m", xDuration, yRow5, cVALUE, ALIGNRIGHT, eFONTSMALLEST},

    {"CN84", xGrid, yRow6, cVALUE, ALIGNLEFT, eFONTSMALLEST},               // [GRID3]
    {"12-11-2011", xEnterDate, yRow6, cVALUE, ALIGNRIGHT, eFONTSMALLEST},   // dummy data
    {"0823", xEnterTime, yRow6, cVALUE, ALIGNLEFT, eFONTSMALLEST},
    {"", xExitDate, yRow6, cVALUE, ALIGNRIGHT, eFONTSMALLEST},
    {"", xExitTime, yRow6, cVALUE, ALIGNLEFT, eFONTSMALLEST},
    {"47.9h", xDuration, yRow6, cVALUE, ALIGNRIGHT, eFONTSMALLEST},

    {"CN83", xGrid, yRow7, cVALUE, ALIGNLEFT, eFONTSMALLEST},               // [GRID4]
    {"12-55-2055", xEnterDate, yRow7, cVALUE, ALIGNRIGHT, eFONTSMALLEST},   // dummy data
    {"0823", xEnterTime, yRow7, cVALUE, ALIGNLEFT, eFONTSMALLEST},
    {"", xExitDate, yRow7, cVALUE, ALIGNRIGHT, eFONTSMALLEST},
    {"", xExitTime, yRow7, cVALUE, ALIGNLEFT, eFONTSMALLEST},
    {"99.9d", xDuration, yRow7, cVALUE, ALIGNRIGHT, eFONTSMALLEST},

    {"CN82", xGrid, yRow8, cVALUE, ALIGNLEFT, eFONTSMALLEST},             // [GRID5]
    {"1-1-1111", xEnterDate, yRow8, cVALUE, ALIGNRIGHT, eFONTSMALLEST},   // dummy data
    {"0111", xEnterTime, yRow8, cVALUE, ALIGNLEFT, eFONTSMALLEST},
    {"", xExitDate, yRow8, cVALUE, ALIGNRIGHT, eFONTSMALLEST},
    {"", xExitTime, yRow8, cVALUE, ALIGNLEFT, eFONTSMALLEST},
    {"999d", xDuration, yRow8, cVALUE, ALIGNRIGHT, eFONTSMALLEST},

    {"Jan 01, 2001", 130, yRowBot, cFAINT, ALIGNRIGHT, eFONTSMALLEST},   // [GMT_DATE]
    {"02:34:56", 148, yRowBot, cFAINT, ALIGNLEFT, eFONTSMALLEST},        // [GMT_TIME]
    {"GMT", 232, yRowBot, cFAINT, ALIGNLEFT, eFONTSMALLEST},             // [GMT]
};
  // Iterator helper
  int previousItem(int ii) {
    return (ii > 0) ? (ii - 1) : (numHistory - 1);
  }

  // helper that actually does all the work
  void extractGridCrossings(CrossingInfo *timeInGrid) {
    // loop through ALL entries in the GPS history array
    // assume the entries are in chronological order, most recent first
    // this only finds the most recent 5 grids crossed, it doesn't do anything about showing results
    int historyIndex = previousItem(model->nextHistoryItem);
    // Serial.print("First item examined is index "); Serial.println(historyIndex);   // debug
    int maxResults  = 5;   // number of rows displayed on screen
    int resultIndex = 0;   //

    char currentGrid4[5] = "none";
    // grid.calcLocator(currentGrid4, model->gLatitude, model->gLongitude, 4);

    // clear any previous results
    for (int jj = 0; jj < 5; jj++) {
      strncpy(timeInGrid[jj].grid4, "un", 4);
      timeInGrid[jj].enterTimestamp = (time_t)0;
      timeInGrid[jj].exitTimestamp  = (time_t)0;
      timeInGrid[jj].isSet          = false;
    }
    // Serial.print("At entry: "); dumpCrossingInfo(timeInGrid, 0);   // debug

    // walk the entire GPS breadcrumb array
    for (int ii = 0; ii < numHistory; ii++) {
      Location item = history[historyIndex];
      if (!item.isEmpty()) {
        char thisGrid[5];
        grid.calcLocator(thisGrid, item.loc.lat, item.loc.lng, 4);

        // compare this slot's grid to previous slot's grid
        if (strcmp(thisGrid, currentGrid4) == 0) {
          // same grid, ignore, keep looking
        } else {
          // we encountered a different grid, so write this grid-crossing info into result
          strcpy(timeInGrid[resultIndex].grid4, thisGrid);
          timeInGrid[resultIndex].enterTimestamp = item.timestamp;
          // timeInGrid[resultIndex].exitTimestamp  = item[prevResultIndex].enterTimestamp?; // exit time is unused
          timeInGrid[resultIndex].isSet = true;

          // save this grid for next loop comparison
          strncpy(currentGrid4, thisGrid, sizeof(currentGrid4));
        }
      }
      historyIndex = previousItem(historyIndex);
    }
    dumpCrossingInfo(&timeInGrid[0], 0);   // debug
    /* ... 
    dumpCrossingInfo(&timeInGrid[1], 1);   // debug
    dumpCrossingInfo(&timeInGrid[2], 2);   // debug
    dumpCrossingInfo(&timeInGrid[3], 3);   // debug
    dumpCrossingInfo(&timeInGrid[4], 4);   // debug
    /* ... */
  }

  // debug helper to show internal status of Grid Crossing
  void dumpCrossingInfo(const CrossingInfo *timeInGrid, int index) {
    char dump[128];  // debug
    snprintf(dump, sizeof(dump), "timeInGrid[%d] = '%s', %lu, %lu, %lu seconds",  // "%lu" is unsigned long
        index,
        timeInGrid->grid4,
        timeInGrid->enterTimestamp,   // enter
        timeInGrid->exitTimestamp,    // exit
        (timeInGrid->exitTimestamp - timeInGrid->enterTimestamp)); // elapsed seconds
    Serial.print(dump); // debug
    if (timeInGrid->isSet) {
      Serial.println(", valid");
    } else {
      Serial.println(", un");
    }
  }

  // helper to re-display entire list of crossings
  void showGridCrossings(CrossingInfo *timeInGrid) {
    for (int row = 0; row < 5; row++) {
      CrossingInfo item = timeInGrid[row];
      if (row == 0) {
        // first row's elapsed time uses "now" because it has no "exit time" since we're still in the same grid
        showGridCrossing(GRID1, item.grid4, item.enterTimestamp, now(), item.isSet);
      } else {
        // subsequent rows always have an "exit time"
        int field = GRID1 + row * 6;  // 6 = count of (GRID1, DATE1IN, TIME1IN, DATE1OUT, TIME1OUT, ET1)
        showGridCrossing(field, item.grid4, item.enterTimestamp, item.exitTimestamp, item.isSet);
      }
    }
  }

  // helper to display one row on screen
  void showGridCrossing(int field, const char *grid4, time_t enterTime, time_t exitTime, bool isSet) {
    // first, sanity check the recorded time
    //                             s, m, h, dow, dd, mm, yy
    TimeElements Jan_1_2020     = {0, 0, 0, 0,    1,  1, 2020 - 1970};
    time_t earliestPossibleTime = makeTime(Jan_1_2020);
    bool timeTooEarly           = (enterTime < earliestPossibleTime) ? true : false;

    TimeElements Jan_1_2072     = {0, 0, 0, 0,    1,  1, 2072 - 1970};
    time_t latestAllowedTime    = makeTime(Jan_1_2072);   // fifty years from current 2022
    bool timeTooLate            = (enterTime > latestAllowedTime) ? true : false;

    // if the grid was recorded before RTC set, the "enter time" can be 1970, before Griduino was built
    char suffix[8]       = "";
    if (isSet && timeTooEarly) {
      strncpy(suffix, " ?", sizeof(suffix));

      char msg[24];
      date.datetimeToString(msg, sizeof(msg), enterTime, " ?");
      logger.error("Internal Error, entered grid before 2020: ", msg);
    }

    // if the "enter time" is after now() then we don't want to show negative elapsed time
    // but we can't trust "now()" if the RTC is not set yet, so use an arbitrarily large milepost
    if (isSet && timeTooLate) {
      strncpy(suffix, " ?", sizeof(suffix));

      char msg[24];
      date.datetimeToString(msg, sizeof(msg), enterTime, suffix);
      logger.error("Internal Error, entered grid more than fifty years after 2022: ", msg);
    }

    // [GRID]
    if (isSet) {
      txtFields[field + 0].print(grid4);
      txtFields[field + 0].setColor(cVALUE);
    } else {
      txtFields[field + 0].print("  - -");
      txtFields[field + 0].setColor(cFAINT);
    }

    // entered grid
    char msg[32];
    if (isSet) {
      date.dateToString(msg, sizeof(msg), enterTime);
      txtFields[field + 1].print(msg);
      date.timeToString(msg, sizeof(msg), enterTime, suffix);
      txtFields[field + 2].print(msg);
    } else {
      txtFields[field + 1].setColor(cFAINT);
      txtFields[field + 2].setColor(cFAINT);
      txtFields[field + 1].print("-");
      txtFields[field + 2].print("-");
    }

    /* ...
    // exited grid (unused)
    if (field == GRID1) {
      // top row on screen is a special case, since we're still in the grid there's no 'exit' time
      txtFields[DATE1OUT].setColor(cFAINT);
      txtFields[TIME1OUT].setColor(cFAINT);
      // txtFields[DATE1OUT].print("");   // unused
      // txtFields[TIME1OUT].print("");
    } else {
      if (isSet) {
        txtFields[field + 1].setColor(cVALUE);
        txtFields[field + 2].setColor(cVALUE);
        date.dateToString(msg, sizeof(msg), exitTime);
        // txtFields[field + 3].print(msg);   // unused
        date.timeToString(msg, sizeof(msg), exitTime);
        // txtFields[field + 4].print(msg);   // unused
      } else {
        txtFields[field + 3].setColor(cFAINT);
        txtFields[field + 4].setColor(cFAINT);
        // txtFields[field + 3].print("-");   // unused
        // txtFields[field + 4].print("-");   // unused
      }
    }
    ... */

    // elapsed time in grid
    if (isSet) {
      calcTimeDiff(msg, sizeof(msg), enterTime, exitTime);
      txtFields[field + 5].print(msg);
    } else {
      txtFields[field + 5].setColor(cFAINT);
      txtFields[field + 5].print("-");
    }
  }
};   // end class ViewGridCrossings
