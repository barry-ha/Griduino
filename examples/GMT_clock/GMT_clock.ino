// Please format this file with clang before check-in to GitHub
/*
  GMT_clock - bright colorful Greenwich Mean Time based on GPS

  Version history:
            2024-01-17 replaced Adafruit_Touchscreen with my own Resistive_Touch_Screen library
            2023-12-26 simplified program with common constants.h and touch.cpp
            2023-12-24 improved debounce by adding hysteresis
            2021-01-30 added support for BMP390 and latest Adafruit_BMP3XX library
            2020-06-03 merged this clock into the main Griduino program as another view
            2020-05-13 proportional fonts
            2020-05-12 updated TouchScreen code
            2020-05-01 added save/restore to nonvolatile RAM
            2020-04-29 added touch adjustment of local time zone
            2020-04-22 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This program offers a bright colorful GMT clock for your shack
            or dashboard. After all, once a rover arrives at a destination
            they no longer need grid square navigation.

            This sketch has only two touch areas to make it easy for operator
            to select "left half" and "right half" without looking.
            Touch target precision is not essential.

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)     https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341    https://www.adafruit.com/product/1743

         3. Adafruit BMP388 - Precision Barometric Pressure https://www.adafruit.com/product/3966
            Adafruit BMP390                                 https://www.adafruit.com/product/4816

         4. Adafruit Ultimate GPS                           https://www.adafruit.com/product/746

*/

#include <Adafruit_ILI9341.h>         // TFT color display library
#include <Resistive_Touch_Screen.h>   // my library replaces the lame Adafruit/Adafruit_TouchScreen
#include "Adafruit_GPS.h"             // Ultimate GPS library
#include "Adafruit_BMP3XX.h"          // Precision barometric and temperature sensor
#include "save_restore.h"             // Save configuration in non-volatile RAM
#include "constants.h"                // Griduino constants, colors, typedefs
#include "hardware.h"                 // Griduino pin definitions
#include "TextField.h"                // Optimize TFT display text for proportional fonts

// ------- Identity for splash screen and console --------
#define PROGRAM_NAME "Griduino GMT Clock"

// #define ECHO_GPS_SENTENCE           // comment out to quiet down the serial monitor
// #define SHOW_TOUCH_TARGET           // comment out for production

// ========== forward reference ================================
void timeZonePlus();
void timeZoneMinus();

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Barometric and Temperature Sensor
Adafruit_BMP3XX baro;   // hardware SPI

// ---------- GPS ----------
// Hardware serial port for GPS
Adafruit_GPS GPS(&Serial1);

// ---------- Touch Screen
Resistive_Touch_Screen tsn(PIN_XP, PIN_YP, PIN_XM, PIN_YM, XP_XM_OHMS);

// ------------ definitions
const int howLongToWait = 4;   // max number of seconds at startup waiting for Serial port to console

// ------------ global scope
int gTimeZone   = -7;   // default local time Pacific (-7 hours), saved in nonvolatile memory
int gSatellites = 0;    // number of satellites

// ============== GPS helpers ==================================
void processGPS() {
  // keep track of number of GPS satellites in view as a confidence indicator
  gSatellites = GPS.satellites;

  // this sketch doesn't use GPS lat/long, so we ignore NMEA sentences (except #satellites)
  // IF the GPS has a battery, then its RTC (realtime clock) remembers the time
  // and for all practical purposes the GMT reading is always right.
}

// Formatted Local time
void getTimeLocal(char *result, int len) {
  // @param result = char[10] = string buffer to modify
  // @param len = string buffer length
  int hh = GPS.hour + gTimeZone;   // 24-hour clock (format matches GMT time)
  hh     = (hh + 24) % 24;         // ensure positive number of hours
  int mm = GPS.minute;
  int ss = GPS.seconds;
  snprintf(result, len, "%02d:%02d:%02d",
           hh, mm, ss);
}

// Did the GPS report a valid date?
bool isDateValid(int yy, int mm, int dd) {
  if (yy < 19) {
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

// Formatted GMT date "Jan 12, 2020"
void getDate(char *result, int maxlen) {
  // @param result = char[15] = string buffer to modify
  // @param maxlen = string buffer length
  // Note that GPS can have a valid date without a position; we can't rely on GPS.fix()
  // to know if the date is correct or not. So we deduce it from the yy/mm/dd values.
  char sDay[3];    // "12"
  char sYear[5];   // "2020"
  char aMonth[][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "err"};

  uint8_t yy = GPS.year;
  int mm     = GPS.month;
  int dd     = GPS.day;

  if (isDateValid(yy, mm, dd)) {
    int year  = yy + 2000;   // convert two-digit year into four-digit integer
    int month = mm - 1;      // GPS month is 1-based, our array is 0-based
    snprintf(result, maxlen, "%s %d, %4d",
             aMonth[month], dd, year);
  } else {
    // GPS does not have a valid date, we will display it as "0000-00-00"
    snprintf(result, maxlen, "0000-00-00");
  }
}

// ======== barometer and temperature helpers ==================
float getTemperature() {
  // returns temperature in Farenheight
  if (!baro.performReading()) {
    Serial.println("Error, failed to read barometer");
  }
  // continue anyway, for demo
  float tempF = baro.temperature * 9 / 5 + 32;
  return tempF;
}

// ========== text screen layout ===================================

// vertical placement of text rows
const int yRow1 = 54;           // program title: "Griduino GMT Clock"
const int yRow2 = yRow1 + 28;   // program version
const int yRow3 = yRow2 + 48;   // author line 1
const int yRow4 = yRow3 + 32;
const int yRow9 = 226;   // GMT date on bottom row, "230" will match other views

// clang-format off
TextField txtSplash[] = {
    //     text               x,y       color
    {PROGRAM_NAME, -1, yRow1, cTEXTCOLOR},   // [0] program title, centered
    {PROGRAM_VERSION, -1, yRow2, cLABEL},    // [1] normal size text, centered
    {PROGRAM_LINE1, -1, yRow3, cLABEL},      // [2] credits line 1, centered
    {PROGRAM_LINE2, -1, yRow4, cLABEL},      // [3] credits line 2, centered
    {"Compiled " PROGRAM_COMPILED, -1, yRow9, cTEXTCOLOR},   // [4] "Compiled", bottom row
                             // clang on
};
const int numSplashFields = sizeof(txtSplash) / sizeof(TextField);

void startSplashScreen() {

  clearScreen();                                         // clear screen
  txtSplash[0].setBackground(cBACKGROUND);               // set background for all TextFields
  TextField::setTextDirty(txtSplash, numSplashFields);   // make sure all fields are updated

  setFontSize(eFONTSMALL);
  for (int ii = 0; ii < 4; ii++) {
    txtSplash[ii].print();
  }

  setFontSize(eFONTSMALLEST);
  for (int ii = 4; ii < numSplashFields; ii++) {
    txtSplash[ii].print();
  }
}

// ========== main clock view helpers ==========================
// these are names for the array indexes, must be named in same order as below
// clang-format off
enum txtIndex {
  TITLE=0, 
  HOURS, COLON1, MINUTES, COLON2, SECONDS,
  GMTDATE, DEGREES, LOCALTIME, TIMEZONE, NUMSATS,
};

TextField txtClock[] = {
  //   text           x,y      color             index
  {PROGRAM_TITLE,    -1, 14,   cTEXTCOLOR},  // [TITLE]     program title, centered
  {"hh",             12, 90,   cVALUE},      // [HOURS]     giant clock hours
  {":",              94, 90,   cVALUE},      // [COLON1]    :
  {"mm",            120, 90,   cVALUE},      // [MINUTES]   giant clock minutes
  {":",             206, 90,   cVALUE},      // [COLON2]    :
  {"ss",            230, 90,   cVALUE},      // [SECONDS]   giant clock seconds
  {"MMM dd, yyyy",   94,130,   cVALUE},      // [GMTDATE]   GMT date
  {"12.3 F",        132,164,   cVALUE},      // [DEGREES]   Temperature
  {"hh:mm:ss",      118,yRow9, cTEXTCOLOR},  // [LOCALTIME] Local time
  {"-7h",             8,yRow9, cFAINT},      // [TIMEZONE]  addHours time zone
  {"6#",            308,yRow9, cFAINT, ALIGNRIGHT},  // [NUMSATS]   numSats
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
  {  "+",   66,204,  36,30, { 30,180, 110,59},  4,  cTEXTCOLOR, timeZonePlus  },  // Up
  {  "-",  226,204,  36,30, {190,180, 110,59},  4,  cTEXTCOLOR, timeZoneMinus },  // Down
};
const int nTimeButtons = sizeof(timeButtons)/sizeof(Button);
// clang-format on

void startViewScreen() {
  // one-time setup for static info on the display

  clearScreen();
  setFontSize(eFONTSMALLEST);
  txtClock[TITLE].print();

  setFontSize(eFONTGIANT);
  for (int ii = HOURS; ii <= SECONDS; ii++) {
    txtClock[ii].print();
  }

  setFontSize(eFONTSMALL);
  for (int ii = GMTDATE; ii <= NUMSATS; ii++) {
    txtClock[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALL);
  for (int ii = 0; ii < nTimeButtons; ii++) {
    TimeButton item = timeButtons[ii];
    tft.fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft.drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

#ifdef SHOW_TOUCH_TARGETS
    tft.drawRect(item.hitTarget.ul.x, item.hitTarget.ul.y,   // debug: draw outline around hit target
                 item.hitTarget.size.x, item.hitTarget.size.y,
                 cBUTTONOUTLINE);
#endif

    tft.setCursor(item.x + item.w / 2 - 7, item.y + item.h / 2 + 5);
    tft.setTextColor(item.color);
    tft.print(item.text);
  }

  // debug: show centerline on display
  // tft.drawLine(tft.width()/2,0, tft.width()/2,tft.height(), cWARN); // debug
}

void updateView() {
  /*
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
    |              Temp:  101.7 F             |
    |                                         |
    +-------+                         +-------+
    | +1 hr |    Local: HH:MM:SS      | -1 hr |
    +-------+-------------------------+-------+
  */
  // GMT Time
  char sHour[8], sMinute[8], sSeconds[8];
  snprintf(sHour, sizeof(sHour), "%02d", GPS.hour);
  snprintf(sMinute, sizeof(sMinute), "%02d", GPS.minute);
  snprintf(sSeconds, sizeof(sSeconds), "%02d", GPS.seconds);

  Serial.print("GMT = (");
  Serial.print(sHour);
  Serial.print("):(");
  Serial.print(sMinute);
  Serial.print("):(");
  Serial.print(sSeconds);
  Serial.println(")");

  setFontSize(eFONTGIANT);
  txtClock[HOURS].print(sHour);
  txtClock[MINUTES].print(sMinute);
  txtClock[SECONDS].print(sSeconds);
  txtClock[COLON2].dirty = true;
  txtClock[COLON2].print();

  setFontSize(eFONTSMALL);

  // GMT Date
  char sDate[16];   // strlen("Jan 12, 2020 ") = 14
  getDate(sDate, sizeof(sDate));
  txtClock[GMTDATE].print(sDate);

  // Temperature
  float t = getTemperature();
  double intpart;
  double fractpart = modf(t, &intpart);

  int degr = (int)intpart;
  int frac = (int)fractpart * 10;

  char sTemp[9];   // strlen("123.4 F") = 7
  snprintf(sTemp, sizeof(sTemp), "%d.%d F", degr, frac);
  txtClock[DEGREES].print(sTemp);

  // Hours to add/subtract from GMT for local time
  char sign[2] = {0, 0};                       // prepend a plus-sign when >=0
  sign[0]      = (gTimeZone >= 0) ? '+' : 0;   // (don't need to add a minus-sign bc the print stmt does that for us)
  char sTimeZone[6];                           // strlen("-10h") = 4
  snprintf(sTimeZone, sizeof(sTimeZone), "%s%dh", sign, gTimeZone);
  txtClock[TIMEZONE].print(sTimeZone);

  // Local Time
  char sTime[10];   // strlen("01:23:45") = 8
  getTimeLocal(sTime, sizeof(sTime));
  txtClock[LOCALTIME].print(sTime);

  // Satellite Count
  char sBirds[4];   // strlen("5#") = 2
  snprintf(sBirds, sizeof(sBirds), "%d#", gSatellites);
  // change colors by number of birds
  txtClock[NUMSATS].color = (gSatellites < 1) ? cWARN : cFAINT;
  txtClock[NUMSATS].print(sBirds);
  // txtClock[NUMSATS].dump();         // debug
}

void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connection to the PC
  // and the operator takes awhile to restart the IDE console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong * 1000;
  while (millis() < targetTime) {
    if (Serial)
      break;
  }
}

//=========== time helpers =====================================
#define TIME_FOLDER  "/GMTclock"   // 8.3 names
#define TIME_FILE    TIME_FOLDER "/AddHours.cfg"
#define TIME_VERSION "v02"

void timeZonePlus() {
  gTimeZone++;
  if (gTimeZone > 12) {
    gTimeZone = -11;
  }
  updateView();
  Serial.print("Time zone changed to ");
  Serial.println(gTimeZone);
  SaveRestore myconfig = SaveRestore(TIME_FOLDER, TIME_FILE, TIME_VERSION, gTimeZone);
  int rc               = myconfig.writeConfig();
  if (rc == 0) {
    Serial.print("Failed to write time zone, rc = ");
    Serial.println(rc);
  }
}
void timeZoneMinus() {
  gTimeZone--;
  if (gTimeZone < -12) {
    gTimeZone = 11;
  }
  updateView();
  Serial.print("Time zone changed to ");
  Serial.println(gTimeZone);
  SaveRestore myconfig = SaveRestore(TIME_FOLDER, TIME_FILE, TIME_VERSION, gTimeZone);
  int rc               = myconfig.writeConfig();
  if (rc == 0) {
    Serial.print("Failed to write time zone, rc = ");
    Serial.println(rc);
  }
}

//=========== distance helpers =================================

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  static int addDotX = 10;   // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count   = 0;
  const int SCALEF   = 2048;   // how much to slow it down so it becomes visible

  count = (count + 1) % SCALEF;
  if (count == 0) {
    addDotX = (addDotX + 1) % tft.width();     // advance
    rmvDotX = (rmvDotX + 1) % tft.width();     // advance
    tft.drawPixel(addDotX, row, foreground);   // write new
    tft.drawPixel(rmvDotX, row, background);   // erase old
  }
}

//=========== setup ============================================
void setup() {

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);   // set backlight to full brightness

  // ----- init TFT display
  tft.begin();                  // initialize TFT display
  tft.setRotation(LANDSCAPE);   // 1=landscape (default is 0=portrait)
  clearScreen();                // note that "begin()" did not clear screen

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);   // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);       // display very first screen
                                      // AND wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION " " HARDWARE_VERSION);  // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init GPS
  GPS.begin(9600);   // 9600 NMEA is the default baud rate for Adafruit MTK GPS's
  delay(50);         // is delay really needed?

  // ***** 576000 is for Adafruit Ultimate GPS only
  Serial.println("Set GPS baud rate to 57600: ");
  Serial.println(PMTK_SET_BAUD_57600);
  GPS.sendCommand(PMTK_SET_BAUD_57600);
  delay(50);
  GPS.begin(57600);
  Serial.println("Turn on RMC (recommended minimum) and GGA (fix data) including altitude: ");
  Serial.println(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  delay(50);

  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz
  // GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ); // Every 5 seconds
  delay(50);

  // ----- query GPS
  Serial.println("Query GPS Firmware version: ");
  Serial.println(PMTK_Q_RELEASE);    // Echo query to console
  GPS.sendCommand(PMTK_Q_RELEASE);   // Send query to GPS unit
                                     // expected reply: $PMTK705,AXN_2.10...
  delay(50);

  // ----- init onboard LED
  pinMode(RED_LED, OUTPUT);           // diagnostics RED LED

  // ----- announce ourselves
  startSplashScreen();

  delay(6000);   // milliseconds

  // ----- init BMP388 or BMP390 barometer
  if (!baro.begin_SPI(BMP_CS)) {
    // failed to initialize hardware
    Serial.println("Error, unable to initialize BMP388, check your wiring");

#define RETRYLIMIT 10
    // clang-format off
    TextField txtError[] = {
      //        text                 x,y     color  
      {"Error!",                    12, 32,  cWARN},       // [0]
      {"Unable to init barometer",  12, 62,  cWARN},       // [1]
      {"Please check your wiring",  12, 92,  cWARN},       // [2]
      {"Retrying...",               12,152,  cWARN},       // [3]
      {"1",                        150,152,  cTEXTCOLOR, ALIGNRIGHT},  // [4]
      {"of 50",                    162,152,  cWARN},       // [5]
    };
    // clang-format on
    const int numErrorFields = sizeof(txtError) / sizeof(TextField);

    setFontSize(eFONTSMALL);
    clearScreen();
    for (int ii = 0; ii < numErrorFields; ii++) {
      txtError[ii].print();
    }

    for (int ii = 1; ii <= RETRYLIMIT; ii++) {
      txtError[4].print(ii);
      if (baro.begin_SPI(BMP_CS)) {
        break;   // success, baro sensor finally initialized
      }
      delay(1000);
    }
  }

  // Set up BMP388 oversampling and filter initialization
  // baro.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);

  // ----- restore configuration settings
  SaveRestore myconfig = SaveRestore(TIME_FOLDER, TIME_FILE, TIME_VERSION);
  if (myconfig.readConfig()) {
    gTimeZone = myconfig.intSetting;
  }
  Serial.print("Time zone restored to ");
  Serial.println(gTimeZone);

  startViewScreen();
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTimeGPS             = millis();

// GPS_PROCESS_INTERVAL is how frequently to update the model from GPS data.
const int GPS_PROCESS_INTERVAL   = 1000;   // milliseconds between updating the model's GPS data
uint32_t prevTimeTouch           = millis();
const int TOUCH_PROCESS_INTERVAL = 5;   // milliseconds between polling for touches

void loop() {

  // if our timer or system millis() wrapped around, reset it
  if (prevTimeGPS > millis()) {
    prevTimeGPS = millis();
  }
  if (prevTimeTouch > millis()) {
    prevTimeTouch = millis();
  }

  GPS.read();   // if you can, read the GPS serial port every millisecond
                // this example sketch reads the serial port continuously during idle time

  if (GPS.newNMEAreceived()) {
    // sentence received -- verify checksum, parse it
    // GPS parsing: https://learn.adafruit.com/adafruit-ultimate-gps/parsed-data-output
    if (!GPS.parse(GPS.lastNMEA())) {
      // parsing failed -- restart main loop to wait for another sentence
      // this also sets the newNMEAreceived() flag to false
      return;
    } else {
#ifdef ECHO_GPS_SENTENCE
      Serial.print(GPS.lastNMEA());   // debug
#endif
    }
  }

  // periodically, process and save the current GPS time
  if (millis() - prevTimeGPS > GPS_PROCESS_INTERVAL) {
    prevTimeGPS = millis();   // restart another interval

    processGPS();   // update GMT time
    updateView();   // update current screen
  }

  // if there's touchscreen input, handle it
  if (millis() - prevTimeTouch > TOUCH_PROCESS_INTERVAL) {
    prevTimeTouch = millis();   // start another interval
    ScreenPoint screenLoc;
    if (tsn.newScreenTap(&screenLoc, tft.getRotation())) {

#ifdef SHOW_TOUCH_TARGET
      // show where touched
      const int radius = 2;
      tft.fillCircle(screenLoc.x, screenLoc.y, radius, cTOUCHTARGET);
#endif

      for (int ii = 0; ii < nTimeButtons; ii++) {
        Point whereTouched = {screenLoc.x, screenLoc.y};   // convert TSPoint into Point
        if (timeButtons[ii].hitTarget.contains(whereTouched)) {
          timeButtons[ii].function();   // dispatch to timeZonePlus() or timeZoneMinus()
          Serial.print("Hit! target = ");
          Serial.println(ii);
        }
      }
    }
  }

  // small activity bar crawls along bottom edge to give
  // a sense of how frequently the main loop is executing
  showActivityBar(tft.height() - 1, ILI9341_RED, cBACKGROUND);
}
