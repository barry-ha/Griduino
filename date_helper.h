#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     date_helper.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This contains date and time conversion utilities:
            date to string, elapsed time between dates, next minute, etc.

            The goal is to collect these helpers together in one place,
            because these techniques are the most likely to be reused or
            rewritten into other programming languages.
*/

#include "constants.h"   // Griduino constants and colors
#include "logger.h"      // conditional printing to Serial port

// ========== class Grids =========================================
class Dates {
public:
  // ======== date time helpers =================================
  char *dateToString(char *msg, int len, time_t datetime) {
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
             year(datetime), month(datetime), day(datetime),
             hour(datetime), minute(datetime), second(datetime));
    return msg;
  }

  // Does the GPS real-time clock contain a valid date?
  bool isDateValid(int yy, int mm, int dd) {
    bool valid = true;
    if (yy < 20) {
      valid = false;
    }
    if (mm < 1 || mm > 12) {
      valid = false;
    }
    if (dd < 1 || dd > 31) {
      valid = false;
    }
    if (!valid) {
      // debug - issue message to console to help track down timing problem in Baroduino view
      // char msg[120];
      // snprintf(msg, sizeof(msg), "Date ymd not valid: %d-%d-%d");
      // Serial.println(msg);
    }
    return valid;
  }

  time_t nextOneSecondMark(time_t timestamp) {
    return timestamp + 1;
  }
  time_t nextOneMinuteMark(time_t timestamp) {
    return ((timestamp + 1 + SECS_PER_MIN) / SECS_PER_MIN) * SECS_PER_MIN;
  }
  time_t nextFiveMinuteMark(time_t timestamp) {
    return ((timestamp + 1 + SECS_PER_5MIN) / SECS_PER_5MIN) * SECS_PER_5MIN;
  }
  time_t nextFifteenMinuteMark(time_t timestamp) {
    return ((timestamp + 1 + SECS_PER_15MIN) / SECS_PER_15MIN) * SECS_PER_15MIN;
  }

};   // end class Dates
