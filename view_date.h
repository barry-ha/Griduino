/*
  File:     view_date.cpp

  Special Event Calendar Count - "How many days since Groundhog Day 2020"

  Date:     2020-10-15 refactored from .cpp to .h
            2020-10-02 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is a frivolous calendar display showing the number 
            of days in a row that we've celebrated some special event. 
            I happen to be amused by wishing people "Happy Groundhog Day" 
            during the pandemic and self-isolation. This is a reference to 
            the film "Groundhog Day," a 1993 comedy starring Bill Murray who 
            is caught in a time loop and relives February 2 repeatedly. 
            The first cases of covid-19 were reported near the end of January
            in Washington State near where I live. So Feb 2 is a reasonable
            stand-in for the start of the pandemic.

            This is "total days spanned" and not "days since the event".
            The difference is whether or not the first day is included in the count.
            I want Feb 2 as "Groundhog Day #1" and Feb 3 as "Groundhog Day #2." 
            In contrast, calculating "days since" would show Feb 3 as Day #1.

  Inspired by: https://days.to/groundhog-day/2020

            +-----------------------------------------+
            | *        How many days since          > |
            |       Sunday, February 2, 2020          |
            |            Groundhog Day                |
            |                                         |
            |           NNNNN NNNNN NNNNN             |
            |           NNNNN NNNNN NNNNN             |
            |           NNNNN NNNNN NNNNN             |
            |           NNNNN NNNNN NNNNN             |
            |                                         |
            |            HH    MM    SS               |
            |           hours  min   sec              |
            |                                         |
            |               Today is                  |
            | -7h          Oct 2, 2020             6# |
            +-----------------------------------------+
   
  Units of Time:
         This relies on "TimeLib.h" which uses "time_t" to represent time.
         The basic unit of time (time_t) is the number of seconds since Jan 1, 1970, 
         a compact 4-byte integer.
         https://github.com/PaulStoffregen/Time

*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>         // TFT color display library
#include <TimeLib.h>                  // BorisNeubert / Time (who forked it from PaulStoffregen / Time)
#include "constants.h"                // Griduino constants and colors
#include "model_gps.h"                // Model of a GPS for model-view-controller
#include "Adafruit_BMP3XX.h"          // Precision barometric and temperature sensor
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "view.h"                     // Base class for all views

// ======= customize this for any count up/down display ========
// Example 1: Number of "Groundhog Days"
//      Encode "Sunday, Feb 2, 2020" as a time_t
//      But use the day before Feb 2, so the counter includes "Groundhog Day #1" on 2/2/2020
/* 
#define HIDE_ELAPSED_HMS
#define DISPLAY_LINE_1  "Total days including"
#define DISPLAY_LINE_2  "Sunday, Feb 2, 2020"
#define DISPLAY_LINE_3  "Groundhog Day"
//                        s,m,h, dow, d,m,y
TimeElements targetGMT  { 0,0,7,  1,  1,2,2020-1970};    // GMT Groundhog Day (add 7 hours for when Pacific time starts their new day)
/* */

// Example 2: Time until Halloween Trick'r Treaters
/* 
#define DISPLAY_LINE_1  "Countdown to"
#define DISPLAY_LINE_2  "Halloween 6pm"
#define DISPLAY_LINE_3  "Days til Trick'n Treaters"
//                        s,m,h,   dow,   d, m, y
TimeElements targetGMT  { 0,0,7+18,  1,  31,10,2020-1970}; // 6pm Halloween in Pacific time (encoded in GMT by adding 7 hours)
/* */

// Example 3: Time until Christmas Eve
/*
#define DISPLAY_LINE_1  "Countdown to"
#define DISPLAY_LINE_2  "Santa's Arrival"
#define DISPLAY_LINE_3  "Christmas Eve"
//                        s,m,h,   dow,   d, m, y
TimeElements targetGMT  { 0,0,7+0,  1,  25,12,2020-1970}; // Midnight in Pacific time (encoded in GMT by adding 7 hours)
/* */

// Example 4: Time until Valentines Day
/* 
#define DISPLAY_LINE_1  "Countdown to"
#define DISPLAY_LINE_2  "Sunday, Feb 14, 2021"
#define DISPLAY_LINE_3  "Valentines's Day"
//                        s,m,h,   dow,  d, m, y
TimeElements targetGMT  { 0,0,8+0,  1,  14,02,2021-1970}; // Midnight in Pacific time (encoded in GMT by adding 8 hours DST)
/* */

// Example 5: Time until June VHF Contest
/* */
#define DISPLAY_LINE_1  "Countdown to"
#define DISPLAY_LINE_2  "June 12, 2021 at 1800z"
#define DISPLAY_LINE_3  "ARRL June VHF Contest"
//                        s,m,h,   dow,  d, m, y
TimeElements targetGMT  { 0,0,18,   1,  12,06,2021-1970};
/* */

// ========== extern ===========================================
extern Model* model;                  // "model" portion of model-view-controller
extern void showDefaultTouchTargets();// Griduino.ino

// ========== class ViewDate ===================================
class ViewDate : public View {
  public:
    // ---------- public interface ----------
    // This derived class must implement the public interface:
    ViewDate(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    {
      background = cBACKGROUND;       // every view can have its own background color
    }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);

  protected:
    // ---------- local data for this derived class ----------
    // color scheme: see constants.h

    // ----- main clock view helpers -----
    // these are names for the array indexes, must be named in same order as array below
    enum txtIndex {
      TITLE1=0, TITLE2, TITLE3,
      COUNTDAYS, COUNTTIME,
      LOCALDATE, LOCALTIME, TIMEZONE, NUMSATS,
    };

    #define numDateFields 9
    TextField txtDate[numDateFields] = {
      // text            x,y    color       alignment    size
      {DISPLAY_LINE_1,  -1, 20, cTEXTCOLOR, ALIGNCENTER, eFONTSMALLEST }, // [TITLE1] program title, centered
      {DISPLAY_LINE_2,  -1, 44, cTEXTCOLOR, ALIGNCENTER, eFONTSMALLEST }, // [TITLE2]
      {DISPLAY_LINE_3,  -1, 86, cVALUE,     ALIGNCENTER, eFONTSMALL    }, // [TITLE3]
      {"nnn",           -1,152, cVALUE,     ALIGNCENTER, eFONTGIANT    }, // [COUNTDAYS] counted number of days
      {"01:02:03",      -1,182, cVALUEFAINT,ALIGNCENTER, eFONTSMALLEST }, // [COUNTTIME] counted number of hms
      {"MMM dd, yyyy",  66,226, cFAINT,     ALIGNLEFT,   eFONTSMALLEST }, // [LOCALDATE] Local date
      {"hh:mm:ss",     178,226, cFAINT,     ALIGNLEFT,   eFONTSMALLEST }, // [LOCALTIME] Local time
      {"-7h",            8,226, cFAINT,     ALIGNLEFT,   eFONTSMALLEST }, // [TIMEZONE]  addHours time zone
      {"6#",           308,226, cFAINT,     ALIGNRIGHT,  eFONTSMALLEST }, // [NUMSATS]   numSats
    };

    // ----- helpers -----
    #define TIME_FOLDER  "/GMTclock"     // 8.3 names
    #define TIME_FILE    TIME_FOLDER "/AddHours.cfg"
    #define TIME_VERSION "v01"

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

    // Formatted elapsed time
    //    this is similar to getTimeLocal (view_date.cpp)
    void getTimeElapsed(char* result, int len, time_t elapsed) {
      // @param result = char[10] = string buffer to modify
      // @param len = string buffer length

      int dd = elapsedDays(elapsed);
      int hh = numberOfHours(elapsed);
      int mm = numberOfMinutes(elapsed);
      int ss = numberOfSeconds(elapsed);
      snprintf(result, len, "%02d:%02d:%02d",
                             hh, mm,  ss);
    }

};  // end class ViewDate
// ============== implement public interface ================
void ViewDate::updateScreen() {
  // called on every pass through main()

  // GMT Time
  char sHour[8], sMinute[8], sSeconds[8];
  snprintf(sHour,   sizeof(sHour), "%02d", GPS.hour);
  snprintf(sMinute, sizeof(sMinute), "%02d", GPS.minute);
  snprintf(sSeconds,sizeof(sSeconds), "%02d", GPS.seconds);

  if (GPS.seconds == 0) {
    // report GMT to console, but not too often
    Serial.print(sHour);    Serial.print(":");
    Serial.print(sMinute);  Serial.print(":");
    Serial.print(sSeconds); Serial.println(" GMT");
  }

  //                       s,m,h, dow, d,m,y 
  //TimeElements targetGMT{ 1,1,1,  1,  2,2,2020-1970};    // GMT Groundhog Day
  //meElements todaysDate{ 1,1,1,  1,  2,2,2020-1970};    // debug: verify the first Groundhog Day is "day #1"
  //meElements todaysDate{ 1,1,1,  1,  2,10,2020-1970};   // debug: verify Oct 2nd is "day #244" i.e. one more than shown on https://days.to/groundhog-day/2020
  TimeElements todaysDate{ GPS.seconds,GPS.minute,GPS.hour,
                           1,GPS.day,GPS.month,(byte)(2000-1970+GPS.year)}; // GMT current date/time 
  
  time_t adjustment = model->gTimeZone * SECS_PER_HOUR;

  time_t date1local = makeTime(targetGMT);
  time_t date2local = makeTime(todaysDate);

  time_t elapsed = abs(date2local - date1local);      // absolute value ensures result is always +ve
  int elapsedDays = elapsed / SECS_PER_DAY;
  char sTime[24];                     // strlen("01:23:45") = 8
  getTimeElapsed(sTime, sizeof(sTime), elapsed);
  
  txtDate[COUNTDAYS].print(elapsedDays);
  txtDate[COUNTTIME].print(sTime);

  // Local Date
  char sDate[16];                       // strlen("Jan 12, 2020 ") = 14
  model->getDate(sDate, sizeof(sDate)); // todo - make this local not GMT date
  txtDate[LOCALDATE].print(sDate);

  // Hours to add/subtract from GMT for local time
  char sign[2] = { 0, 0 };            // prepend a plus-sign when >=0
  sign[0] = (model->gTimeZone >= 0) ? '+' : 0;   // (don't need to add a minus-sign bc the print stmt does that for us)
  char sTimeZone[6];                  // strlen("-10h") = 4
  snprintf(sTimeZone, sizeof(sTimeZone), "%s%dh", sign, model->gTimeZone);
  txtDate[TIMEZONE].print(sTimeZone);

  // Local Time
  model->getTimeLocal(sTime, sizeof(sTime));
  txtDate[LOCALTIME].print(sTime);

  // Satellite Count
  char sBirds[4];                     // strlen("5#") = 2
  snprintf(sBirds, sizeof(sBirds), "%d#", model->gSatellites);
  // change colors by number of birds
  txtDate[NUMSATS].color = (model->gSatellites<1) ? cWARN : cFAINT;
  txtDate[NUMSATS].print(sBirds);
  //txtDate[NUMSATS].dump();          // debug
}

void ViewDate::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                  // clear screen
  txtDate[0].setBackground(this->background);           // set background for all TextFields in this view
  TextField::setTextDirty( txtDate, numDateFields );    // make sure all fields get re-printed on screen change

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();          // optionally draw boxes around button-touch area
  //showMyTouchTargets(timeButtons, nTimeButtons);
  showScreenBorder();                 // optionally outline visible area
  showScreenCenterline();             // optionally draw visual alignment bar

  // ----- draw page title
  txtDate[TITLE1].print();
  txtDate[TITLE2].print();
  txtDate[TITLE3].print();

  // ----- draw giant fields
  txtDate[COUNTDAYS].print();
  #ifdef HIDE_ELAPSED_HMS
    txtDate[COUNTTIME].setColor(cBACKGROUND); // if requested, make invisible the elapsed h:m:s
  #endif

  // ----- draw text fields
  for (int ii=LOCALDATE; ii<numDateFields; ii++) {
    txtDate[ii].print();
  }

  updateScreen();                     // update UI immediately, don't wait for laggy mainline loop
} // end startScreen()


bool ViewDate::onTouch(Point touch) {
  Serial.println("->->-> Touched date screen.");

  bool handled = false;               // assume a touch target was not hit
  return handled;                     // true=handled, false=controller uses default action
} // end onTouch()
