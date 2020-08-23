/*
  File: view_time.cpp

  GMT_clock - bright colorful Greenwich Mean Time based on GPS

  Date:     2020-04-22 created
            2020-04-29 added touch adjustment of local time zone
            2020-05-01 added save/restore to nonvolatile RAM
            2020-05-12 updated TouchScreen code
            2020-05-13 proportional fonts
            2020-06-02 merged from standalone example into Griduino view

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is a bright colorful GMT clock for your shack
            or dashboard. After all, once a rover arrives at a 
            destination they no longer need grid square navigation.

            +-----------------------------------------+
            |            Griduino GMT Clock           |
            |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
            |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
            |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
            |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
            |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
            |                                         |
            |         GMT Date: Apr 29, 2020          |
            |                                         |
            |         Internal Temp:  101.7 F         |
            |                                         |
            +-------+                         +-------+
            | +1 hr |    Local: HH:MM:SS      | -1 hr |
            +-------+-------------------------+-------+
   
*/

#include <Arduino.h>
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors
#include "model.cpp"                // "Model" portion of model-view-controller
#include "Adafruit_BMP3XX.h"        // Precision barometric and temperature sensor
#include "save_restore.h"           // Save configuration in non-volatile RAM
#include "TextField.h"              // Optimize TFT display text for proportional fonts

// ========== extern ===========================================
extern Adafruit_ILI9341 tft;        // Griduino.ino
extern Model* model;                // "model" portion of model-view-controller
extern Adafruit_BMP3XX baro;        // Griduino.ino
extern void getDate(char* result, int maxlen);  // model.cpp

void setFontSize(int font);         // Griduino.ino
int getOffsetToCenterTextOnButton(String text, int leftEdge, int width ); // Griduino.ino
void drawAllIcons();                // draw gear (settings) and arrow (next screen) // Griduino.ino
void showScreenBorder();            // optionally outline visible area

// ========== forward reference ================================
void timePlus();
void timeMinus();
void updateTimeScreen();

// ============== constants ====================================

// ========== globals ==========================================
int gTimeZone = -7;                     // default local time Pacific (-7 hours), saved in nonvolatile memory
//int gSatellites = 0;                    // number of satellites

// ======== barometer and temperature helpers ==================
float getTemperature() {
  // returns temperature in Farenheight
  if (!baro.performReading()) {
    Serial.println("Error, failed to read temperature sensor, re-initializeing");

    baro.begin();   // attempt to initialize
  }
  // continue anyway
  float tempF = baro.temperature * 9 / 5 + 32;
  return tempF;
}

// ========== load/save config setting =========================

// ========== main clock view helpers ==========================
// these are names for the array indexes, must be named in same order as array below
enum txtIndex {
  TITLE=0, 
  HOURS, COLON1, MINUTES, COLON2, SECONDS,
  GMTDATE, DEGREES, LOCALTIME, TIMEZONE, NUMSATS,
};

TextField txtClock[] = {
  // text            x,y    color             index
  {"Griduino GMT",  -1, 10, cTEXTCOLOR, ALIGNCENTER}, // [TITLE] program title, centered
  {"hh",            12, 90, cVALUE},                  // [HOURS]     giant clock hours
  {":",             94, 90, cVALUE},                  // [COLON1]    :
  {"mm",           120, 90, cVALUE},                  // [MINUTES]   giant clock minutes
  {":",            206, 90, cVALUE},                  // [COLON2]    :
  {"ss",           230, 90, cVALUE},                  // [SECONDS]   giant clock seconds
  {"MMM dd, yyyy",  94,130, cVALUE},                  // [GMTDATE]   GMT date
  {"12.3 F",       132,164, cVALUE},                  // [DEGREES]   Temperature
  {"hh:mm:ss",     118,226, cTEXTCOLOR},              // [LOCALTIME] Local time
  {"-7h",            8,226, cTEXTFAINT},              // [TIMEZONE]  addHours time zone
  {"6#",           308,226, cTEXTFAINT, ALIGNRIGHT},  // [NUMSATS]   numSats
};
const int numClockFields = sizeof(txtClock)/sizeof(TextField);

TimeButton timeButtons[] = {
  // For "GMT_clock" we have rather small modest +/- buttons, meant to visually
  // fade a little into the background. However, we want larger touch-targets to 
  // make them easy to press.
  //
  // 3.2" display is 320 x 240 pixels, landscape, (y=239 reserved for activity bar)
  //
  // label  origin   size      touch-target     
  // text    x,y      w,h      x,y      w,h   radius  color     function
  {  "+",   66,204,  36,30, { 30,180, 110,59},  4,  cTEXTCOLOR, timePlus  },  // Up
  {  "-",  226,204,  36,30, {190,180, 110,59},  4,  cTEXTCOLOR, timeMinus },  // Down
};
const int nTimeButtons = sizeof(timeButtons)/sizeof(Button);

//=========== time helpers =====================================
#define TIME_FOLDER  "/GMTclock"     // 8.3 names
#define TIME_FILE    TIME_FOLDER "/AddHours.cfg"
#define TIME_VERSION "v01"

void timePlus() {
  gTimeZone++;
  if (gTimeZone > 12) {
    gTimeZone = -11;
  }
  updateTimeScreen();
  Serial.print("Time zone changed to "); Serial.println(gTimeZone);
  //SaveRestore myconfig = SaveRestore(TIME_FOLDER, TIME_FILE, TIME_VERSION, gTimeZone);  // todo
  //myconfig.writeConfig(); // todo
}
void timeMinus() {
  gTimeZone--;
  if (gTimeZone < -12) {
    gTimeZone = 11;
  }
  updateTimeScreen();
  Serial.print("Time zone changed to "); Serial.println(gTimeZone);
  //SaveRestore myconfig = SaveRestore(TIME_FOLDER, TIME_FILE, TIME_VERSION, gTimeZone);  // todo
  //myconfig.writeConfig(); // todo
}

// Formatted Local time
void getTimeLocal(char* result, int len) {
  // @param result = char[10] = string buffer to modify
  // @param len = string buffer length
  int hh = GPS.hour + gTimeZone;      // 24-hour clock (format matches GMT time)
  hh = (hh + 24) % 24;                // ensure positive number of hours
  int mm = GPS.minute;
  int ss = GPS.seconds;
  snprintf(result, len, "%02d:%02d:%02d",
                          hh,  mm,  ss);
}

// ============ time screen view ===============================
void updateTimeScreen() {
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
  
  setFontSize(36);
  txtClock[HOURS].print(sHour);
  txtClock[MINUTES].print(sMinute);
  txtClock[SECONDS].print(sSeconds);
  txtClock[COLON2].dirty = true;
  txtClock[COLON2].print();

  setFontSize(12);

  // GMT Date
  char sDate[16];         // strlen("Jan 12, 2020 ") = 14
  model->getDate(sDate, sizeof(sDate));
  txtClock[GMTDATE].print(sDate);

  // Temperature
  float t = getTemperature();
  double intpart;
  double fractpart= modf(t, &intpart);

  int degr = (int) intpart;
  int frac = (int) fractpart*10;
  
  char sTemp[9];         // strlen("123.4 F") = 7
  snprintf(sTemp, sizeof(sTemp), "%d.%d F", degr, frac);
  txtClock[DEGREES].print(sTemp);

  // Hours to add/subtract from GMT for local time
  char sign[2] = { 0, 0 };              // prepend a plus-sign when >=0
  sign[0] = (gTimeZone>=0) ? '+' : 0;   // (don't need to add a minus-sign bc the print stmt does that for us)
  char sTimeZone[6];      // strlen("-10h") = 4
  snprintf(sTimeZone, sizeof(sTimeZone), "%s%dh", sign, gTimeZone);
  txtClock[TIMEZONE].print(sTimeZone);

  // Local Time
  char sTime[10];         // strlen("01:23:45") = 8
  getTimeLocal(sTime, sizeof(sTime));
  txtClock[LOCALTIME].print(sTime);

  // Satellite Count
  char sBirds[4];         // strlen("5#") = 2
  snprintf(sBirds, sizeof(sBirds), "%d#", model->gSatellites);
  // change colors by number of birds
  txtClock[NUMSATS].color = (model->gSatellites<1) ? cWARN : cTEXTFAINT;
  txtClock[NUMSATS].print(sBirds);
  //txtClock[NUMSATS].dump();   // debug
}
void startTimeScreen() {
  // called once each time this view becomes active
  tft.fillScreen(cBACKGROUND);      // clear screen
  txtClock[0].setBackground(cBACKGROUND);               // set background for all TextFields in this view
  TextField::setTextDirty( txtClock, numClockFields );  // make sure all fields get re-printed on screen change

  drawAllIcons();                   // draw gear (settings) and arrow (next screen)
  showScreenBorder();               // optionally outline visible area

  // ----- draw page title
  setFontSize(eFONTSYSTEM);
  txtClock[TITLE].print();

  // ----- draw giant fields
  setFontSize(36);
  for (int ii=HOURS; ii<=SECONDS; ii++) {
    txtClock[ii].print();
  }

  // ----- draw text fields
  setFontSize(eFONTSMALLEST);
  for (int ii=GMTDATE; ii<numClockFields; ii++) {
    txtClock[ii].print();
  }

  // ----- draw buttons
  setFontSize(12);
  for (int ii=0; ii<nTimeButtons; ii++) {
    TimeButton item = timeButtons[ii];
    tft.fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft.drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);
    tft.setCursor(xx, item.y+item.h/2+5);
    tft.setTextColor(item.color);
    tft.print(item.text);

    #ifdef SHOW_TOUCH_TARGETS
    tft.drawRect(item.hitTarget.ul.x, item.hitTarget.ul.y,  // debug: draw outline around hit target
                 item.hitTarget.size.x, item.hitTarget.size.y, 
                 cWARN);
    #endif
  }

  // debug: show centerline on display
  //tft.drawLine(tft.width()/2,0, tft.width()/2,tft.height(), cWARN); // debug
}
bool onTouchTime(Point touch) {
  Serial.println("->->-> Touched status screen.");
  bool handled = false;             // assume a touch target was not hit
  for (int ii=0; ii<nTimeButtons; ii++) {
    TimeButton item = timeButtons[ii];
    if (touch.x >= item.x && touch.x <= item.x+item.w
     && touch.y >= item.y && touch.y <= item.y+item.h) {
        handled = true;             // hit!
        item.function();            // do the thing

        //tft.setCursor(touch.x, touch.y);  // debug: show where hit
        //setFontSize(12);
        //tft.print("x");
     }
  }
  return handled;                   // true=handled, false=controller uses default action
}
