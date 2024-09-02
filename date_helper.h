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
  char *datetimeToString(char *msg, int len, time_t tm) {
    // utility function to format date:  "2020-9-27 17:18:19"
    // Representation is general-to-specific (year first) for sorting
    // Similar to ISO-8601 format but without "T" in front of time
    // Example 1:
    //      char sDate[24];
    //      datetimeToString( sDate, sizeof(sDate), now(), " GMT" );
    //      logger.println( sDate );
    //      --> "2022-12-3 08:07 GMT"
    // Example 2:
    //      char sDate[24];
    //      logger.print("The current date is ");
    //      logger.print( datetimeToString(sDate, sizeof(sDate), now()) );
    //      logger.println(" GMT");
    snprintf(msg, len, "%04d-%d-%d %02d:%02d:%02d",
             year(tm), month(tm), day(tm),
             hour(tm), minute(tm), second(tm));
    return msg;
  }
  char *datetimeToString(char *msg, int len, time_t tm, const char *suffix) {
    datetimeToString(msg, len, tm);
    strncat(msg, suffix, len - 1);
    return msg;
  }

  char *dateToString(char *msg, int len, time_t tm) {
    snprintf(msg, len, "%04d-%02d-%02d", year(tm), month(tm), day(tm));
    return msg;
  }
  char *dateToString(char *msg, int len, time_t tm, const char *suffix) {
    dateToString(msg, len, tm);
    strncat(msg, suffix, len - 1);
    return msg;
  }

  char *timeToString(char *msg, int len, time_t tm) {
    snprintf(msg, len, "%02d:%02d:%02d", hour(tm), minute(tm), second(tm));
    return msg;
  }
  char *timeToString(char *msg, int len, time_t tm, const char *suffix) {
    timeToString(msg, len, tm);
    strncat(msg, suffix, len - 1);
    return msg;
  }

  // Did the GPS real-time clock report a valid date?
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
