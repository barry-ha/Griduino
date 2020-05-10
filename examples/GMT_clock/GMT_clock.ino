/*
  GMT_clock - bright colorful Greenwich Mean Time based on GPS

  Date:     2020-04-22 created
            2020-04-29 added touch adjustment of local time zone
            2020-05-01 added save/restore to nonvolatile RAM

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This program offers a bright colorful GMT clock for your shack
            or dashboard. After all, once a rover arrives at a destination
            they no longer need grid square navigation.

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)
            Spec: https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341
            Spec: http://adafru.it/1743
            How to: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2
            SPI Wiring: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/spi-wiring-and-test
            Touchscreen: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/resistive-touchscreen

         3. Adafruit Ultimate GPS
            Spec: https://www.adafruit.com/product/746

*/

#include "SPI.h"                    // Serial Peripheral Interface
#include "Adafruit_GFX.h"           // Core graphics display library
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "Adafruit_GPS.h"           // Ultimate GPS library
#include "TouchScreen.h"            // Touchscreen built in to 3.2" Adafruit TFT display
#include "Adafruit_BMP3XX.h"        // Precision barometric and temperature sensor
#include "save_restore.h"           // Save configuration in non-volatile RAM

// ------- Identity for console
#define PROGRAM_TITLE   "Griduino GMT Clock"
#define PROGRAM_VERSION "v0.06"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"

// ========== forward reference ================================
void timePlus();
void timeMinus();

// ---------- Hardware Wiring ----------
/* Same as Griduino platform
*/

// TFT display and SD card share the hardware SPI interface, and have
// separate 'select' pins to identify the active device on the bus.
#if defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  // To compile for Feather M0/M4, install "additional boards manager"
  // https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup
  
  #define TFT_BL   4    // TFT backlight
  #define TFT_CS   5    // TFT chip select pin
  #define TFT_DC  12    // TFT display/command pin
  #define BMP_CS  13    // BMP388 sensor, chip select

#elif defined(ARDUINO_AVR_MEGA2560)
  #define TFT_BL   6    // TFT backlight
  #define SD_CCS   7    // SD card select pin - Mega
  #define SD_CD    8    // SD card detect pin - Mega
  #define TFT_DC   9    // TFT display/command pin
  #define BMP_CS  13    // BMP388 sensor, chip select

#else
  // todo: Unknown platform
  #warning You need to define pins for your hardware

#endif

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Touch Screen
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// This sketch has only two touch areas to make it easy for operator to select
// "left half" and "right half" without looking. Touch target precision is not essential.
#if defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  #define PIN_XP  A3    // Touchscreen X+ can be a digital pin
  #define PIN_XM  A4    // Touchscreen X- must be an analog pin, use "An" notation
  #define PIN_YP  A5    // Touchscreen Y+ must be an analog pin, use "An" notation
  #define PIN_YM   9    // Touchscreen Y- can be a digital pin
#elif defined(ARDUINO_AVR_MEGA2560)
  // Arduino Mega 2560 and others
  #define PIN_XP   4    // Touchscreen X+ can be a digital pin
  #define PIN_XM  A3    // Touchscreen X- must be an analog pin, use "An" notation
  #define PIN_YP  A2    // Touchscreen Y+ must be an analog pin, use "An" notation
  #define PIN_YM   5    // Touchscreen Y- can be a digital pin
#else
  // todo: Unknown platform
  #warning You need to define pins for your hardware

#endif
TouchScreen ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, 295);

// ---------- Barometric and Temperature Sensor
Adafruit_BMP3XX baro(BMP_CS); // hardware SPI

// ---------- Onboard LED
#define RED_LED 13    // diagnostics RED LED

// ---------- GPS ----------
/* "Ultimate GPS" pin wiring is connected to a dedicated hardware serial port
    available on an Arduino Mega, Arduino Feather and others.

    The GPS' LED indicates status:
        1-sec blink = searching for satellites
        15-sec blink = position fix found
*/

// Hardware serial port for GPS
Adafruit_GPS GPS(&Serial1);

// ------------ typedef's
typedef struct {
  int x;
  int y;
} Point;

// ------------ definitions
const int howLongToWait = 6;  // max number of seconds at startup waiting for Serial port to console
#define gScreenWidth 320      // pixels wide
#define gScreenHeight 240     // pixels high

// ------------ global scope
int gTimeZone = -7;                     // default local time Pacific (-7 hours), saved in nonvolatile memory
int gSatellites = 0;                    // number of satellites
int gTextSize;                          // no such function as "tft.getTextSize()" so remember it on our own
int gUnitFontWidth, gUnitFontHeight;    // character cell size for TextSize(1)
int gCharWidth, gCharHeight;            // character cell size for TextSize(n)

// ----- screen layout
// using default fonts - screen pixel coordinates will identify top left of character cell

// splash screen layout
const int xLabel = 8;             // indent labels, slight margin on left edge of screen
#define yRow1   0                 // program title: "Griduino GMT Clock"
#define yRow2   yRow1 + 40        // program version
#define yRow3   yRow2 + 20        // author line 1
#define yRow4   yRow3 + 20        // author line 2

// main screen layout
const Point gmtTime   = {  16, 44 };            // "01:23:45"
const Point gmtDate   = {  90, gmtTime.y+58 };  // "Apr 29, 2020"
const Point xyTemp    = { 126, gmtDate.y+30 };  // "78.9 F"
const Point addHours  = {   8, 212 };           // "+8 hr"
const Point localTime = { 116, 212 };           // "09:23:45"
const Point buttonPlus = {  64, 204 };          // [ + ]
const Point buttonMinus= { 226, 204 };          // [ - ]
const Point birds     = { 286, 212 };           // "3 satellites"

// ----- color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND     0x00A             // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cTEXTCOLOR      ILI9341_CYAN      // 0, 255, 255
#define cTEXTFAINT      0x514             // 0, 160, 160 = blue, between CYAN and DARKCYAN
#define cLABEL          ILI9341_GREEN
#define cVALUE          ILI9341_YELLOW
#define cINPUT          ILI9341_WHITE
#define cBUTTONFILL     ILI9341_NAVY
#define cBUTTONOUTLINE  ILI9341_BLUE      // 0,   0, 255 = darker than cyan
#define cWARN           0xF844            // brighter than ILI9341_RED but not pink

typedef void (*simpleFunction)();
typedef struct {
  char text[26];
  int x;
  int y;
  int w;
  int h;
  int radius;
  uint16_t color;
  simpleFunction function;
} Button;

// ========== constants ===============================

const int margin = xLabel;        // slight margin between button border and edge of screen
const int btnRadius = 4;          // rounded corners
const int btnWidth = 36;          // small and inconspicuous 
const int btnHeight = 30;
const int nTimeButtons = 2;
Button timeButtons[nTimeButtons] = {
  // text           x,y                     w,h              r       color       function
  {"+",  buttonPlus.x,buttonPlus.y,  btnWidth,btnHeight, btnRadius, cTEXTCOLOR, timePlus  },  // Up
  {"-", buttonMinus.x,buttonMinus.y, btnWidth,btnHeight, btnRadius, cTEXTCOLOR, timeMinus },  // Down
};

// ============== GPS helpers ==================================
void processGPS() {
  // keep track of number of GPS satellites in view as a confidence indicator
  gSatellites = GPS.satellites;

  // this sketch doesn't use GPS position, so we ignore NMEA sentences (except #satellites)
  // IF the GPS has a battery, then its realtime clock remembers the time
  // and for all practical purposes the GMT reading is always right.
}

// Formatted GMT time
void getTime(char* result, int len) {
  // result = char[10] = string buffer to modify
  int hh = GPS.hour;                      // 24-hour clock, 0..23
  int mm = GPS.minute;
  int ss = GPS.seconds;
  snprintf(result, len, "%02d:%02d:%02d",
                         hh,  mm,  ss);
}
// Formatted Local time
void getTimeLocal(char* result, int len) {
  // result = char[10] = string buffer to modify
  int hh = (GPS.hour + gTimeZone) % 24;   // 24-hour clock, to match GMT time
  int mm = GPS.minute;
  int ss = GPS.seconds;
  snprintf(result, len, "%02d:%02d:%02d",
                          hh,  mm,  ss);
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
void getDate(char* result, int maxlen) {
  // @param result = char[15] = string buffer to modify
  // @param maxlen = string buffer length
  // Note that GPS can have a valid date without a position; we can't rely on GPS.fix()
  // to know if the date is correct or not. So we deduce it from the y/m/d values.
  char sDay[3];       // "12"
  char sYear[5];      // "2020"
  char aMonth[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "err" };

  uint8_t yy = GPS.year;
  int mm = GPS.month;
  int dd = GPS.day;

  if (isDateValid(yy,mm,dd)) {
    int year = yy + 2000;     // convert two-digit year into four-digit integer
    int month = mm - 1;       // GPS month is 1-based, our array is 0-based
    snprintf(result, maxlen, "%s %d, %4d", 
                  aMonth[month], dd, year);
  } else {
    // GPS does not have a valid date, we will display it as "0000-00-00"
    snprintf(result, maxlen, "0000-00-00");
  }
}

// ============== touchscreen helpers ==========================

bool gTouching = false;             // keep track of previous state
bool newScreenTap(Point* pPoint) {
  // find leading edge of a screen touch
  // returns TRUE only once on initial screen press
  // if true, also return screen coordinates of the touch

  bool result = false;        // assume no touch
  if (gTouching) {
    // the touch was previously processed, so ignore continued pressure until they let go
    if (!ts.isTouching()) {
      // Touching ==> Not Touching transition
      gTouching = false;
    }
  } else {
    // here, we know the screen was not being touched in the last pass,
    // so look for a new touch on this pass
    // The built-in "isTouching" function does most of the debounce and threshhold detection needed
    if (ts.isTouching()) {
      gTouching = true;
      result = true;

      // touchscreen point object has (x,y,z) coordinates, where z = pressure
      TSPoint touch = ts.getPoint();

      // convert resistance measurements into screen pixel coords
      mapTouchToScreen(touch, pPoint);
      Serial.print("Screen touch detected ("); Serial.print(pPoint->x);
      Serial.print(","); Serial.print(pPoint->y); Serial.println(")");
    }
  }
  //delay(100);   // no delay: code above completely handles debouncing without blocking the loop
  return result;
}

// 2020-05-03 CraigV and barry@k7bwh.com
// "isTouching()" is defined in touch.h but is not implemented Adafruit's TouchScreen library
// Note - For Griduino, if this function takes longer than 8 msec it can cause erratic GPS readings
// so we recommend against using https://forum.arduino.cc/index.php?topic=449719.0
bool TouchScreen::isTouching(void) {
  #define TOUCHPRESSURE 200       // Minimum pressure we consider true pressing
  static bool button_state = false;
  uint16_t pres_val = pressure();

  if ((button_state == false) && (pres_val > TOUCHPRESSURE)) {
    Serial.println("button down");     // debug
    button_state = true;
  }

  if ((button_state == true) && (pres_val < TOUCHPRESSURE)) {
    Serial.println("button up");       // debug
    button_state = false;
  }

  // Clean the touchScreen settings after function is used
  // Because LCD may use the same pins
  // todo - is this actually necessary?
  //pinMode(_xm, OUTPUT);     digitalWrite(_xm, LOW);
  //pinMode(_yp, OUTPUT);     digitalWrite(_yp, HIGH);
  //pinMode(_ym, OUTPUT);     digitalWrite(_ym, LOW);
  //pinMode(_xp, OUTPUT);     digitalWrite(_xp, HIGH);

  return button_state;
}

void mapTouchToScreen(TSPoint touch, Point* screen) {
  // convert from X+,Y+ resistance measurements to screen coordinates
  // param touch = resistance readings from touchscreen
  // param screen = result of converting touch into screen coordinates
  //
  // Measured readings in Barry's landscape orientation were:
  //   +---------------------+ X=876
  //   |                     |
  //   |                     |
  //   |                     |
  //   +---------------------+ X=160
  //  Y=110                Y=892
  //
  // Typical measured pressures=200..549

  screen->x = 0;
  screen->y = 0;

  // setRotation(1) = landscape orientation = x-,y-axis exchanged
  screen->x = map(touch.y, 100, 900,  0, tft.width());
  screen->y = map(touch.x, 900, 100,  0, tft.height());
  return;
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

// ========== screen helpers ===================================
void startSplashScreen() {
  tft.setTextSize(2);

  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR);
  tft.print(PROGRAM_TITLE);

  tft.setCursor(xLabel, yRow2);
  tft.setTextColor(cLABEL);
  tft.print(PROGRAM_VERSION);
  
  tft.setCursor(xLabel, yRow2 + 20);
  tft.println(PROGRAM_LINE1);

  tft.setCursor(xLabel, yRow2 + 40);
  tft.println(PROGRAM_LINE2);

  delay(2000);
  clearScreen();
  
  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR);
  tft.print(PROGRAM_TITLE);
}
void startViewScreen() {
  // one-time setup for static info on the display

  // ----- draw buttons
  for (int ii=0; ii<nTimeButtons; ii++) {
    Button item = timeButtons[ii];
    tft.fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft.drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    tft.setCursor(item.x+item.w/2-4, item.y+item.h/2-6);
    tft.setTextColor(item.color);
    tft.print(item.text);
  }

  // debug: show centerline on display
  //tft.drawLine(gScreenWidth/2,0, gScreenWidth/2,gScreenHeight, cWARN); // debug

}

void updateView() {
/*
  +-----------------------------------------+
  | Griduino GMT Clock                      |... yRow1
  |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |... yGMTTime
  |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
  |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
  |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
  |  HHHHH HHHH  :  MMMM MMMM  : SSSS SSSS  |
  |  GMT Date: Apr 29, 2020                 |... yGMTDate
  |  Temp:  101.7 F                         |... yTemperature
  |                                         |
  |  Local: HH : MM : SS                    |... yLocalTime
  |                                         |
  +-------+                         +-------+... yButton
  | +1 hr |                         | -1 hr |... yAdjust
  +-------+-------------------------+-------+
*/  
  tft.setTextSize(6);

  // GMT Time
  char sTime[10];         // strlen("19:54:14") = 8
  getTime(sTime, sizeof(sTime));
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.setCursor(gmtTime.x, gmtTime.y);
  tft.print(sTime);

  tft.setTextSize(2);

  // GMT Date
  char sDate[16];         // strlen("Jan 12, 2020 ") = 14
  getDate(sDate, sizeof(sDate));
  strcat(sDate, " "); // append blank to erase possible trailing character on month rollover
  tft.setCursor(gmtDate.x, gmtDate.y);
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(sDate);       // todo

  // Temperature
  float t = getTemperature();
  tft.setCursor(xyTemp.x, xyTemp.y);
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(t, 1);
  tft.print(" F");

  // Hours to add/subtract from GMT for local time
  tft.setCursor(addHours.x, addHours.y);
  tft.setTextColor(cTEXTFAINT, cBACKGROUND);
  if (gTimeZone >= 0) {
    tft.print("+");
  }
  tft.print(gTimeZone);
  tft.print("h");
  if (gTimeZone >-10 && gTimeZone < 10) {
    // erase possible leftover trailing character
    tft.print(" ");
  }

  // Local Time
  getTimeLocal(sTime, sizeof(sTime));
  tft.setCursor(localTime.x, localTime.y);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print(sTime);

  // Satellite Count
  tft.setCursor(birds.x, birds.y);
  tft.setTextColor(cTEXTFAINT, cBACKGROUND);
  tft.print(gSatellites);
  tft.print("#");
}

/* Using fonts: https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
*/

int getOffsetToCenterText(String text) {
  // measure width of given text in current font and 
  // calculate X-offset to make it centered left-right on screen
  
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);  // compute "pixels wide"
  return (gScreenWidth - w) / 2;
}
void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong*1000;
  while (millis() < targetTime) {
    if (Serial) break;
  }
}

//=========== time helpers =====================================
#define TIME_FOLDER  "/GMTclock"     // 8.3 names
#define TIME_FILE    TIME_FOLDER "/AddHours.cfg"
#define TIME_VERSION "v01"

void timePlus() {
  gTimeZone++;
  if (gTimeZone > 12) {
    gTimeZone = -11;
  }
  updateView();
  Serial.print("Time zone changed to "); Serial.println(gTimeZone);
  SaveRestore myconfig = SaveRestore(TIME_FOLDER, TIME_FILE, TIME_VERSION, gTimeZone);
  myconfig.writeConfig();
}
void timeMinus() {
  gTimeZone--;
  if (gTimeZone < -12) {
    gTimeZone = 11;
  }
  updateView();
  Serial.print("Time zone changed to "); Serial.println(gTimeZone);
  SaveRestore myconfig = SaveRestore(TIME_FOLDER, TIME_FILE, TIME_VERSION, gTimeZone);
  myconfig.writeConfig();
}

//=========== distance helpers =================================

int gAddDotX = 10;                     // current screen column, 0..319 pixels
int gRmvDotX = 0;
void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  gAddDotX = (gAddDotX + 1) % gScreenWidth;   // advance
  gRmvDotX = (gRmvDotX + 1) % gScreenWidth;   // advance
  tft.drawPixel(gAddDotX, row, foreground);   // write new
  tft.drawPixel(gRmvDotX, row, background);   // erase old
}

//=========== setup ============================================
void setup() {

  // ----- init serial monitor
  Serial.begin(115200);           // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " __DATE__ " " __TIME__);  // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init GPS
  GPS.begin(9600);                              // 9600 NMEA is the default baud rate for Adafruit MTK GPS's
  delay(200);                                   // is delay really needed?
  GPS.sendCommand(PMTK_SET_BAUD_57600);         // set baud rate to 57600
  delay(200);
  GPS.begin(57600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); // turn on RMC (recommended minimum) and GGA (fix data) including altitude

  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    // 1 Hz update rate
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ); // Once every 5 seconds update
  //GPS.sendCommand(PGCMD_ANTENNA);             // Request updates on whether antenna is connected or not (comment out to keep quiet)

  // ----- query GPS
  Serial.print("Sending command to query GPS Firmware version");
  Serial.println(PMTK_Q_RELEASE);     // Echo query to console
  GPS.sendCommand(PMTK_Q_RELEASE);    // Send query to GPS unit
                                      // expected reply: $PMTK705,AXN_2.10...
  // ----- init onboard LED
  // todo: how to turn off the solid red led next to the USB connector?
  pinMode(RED_LED, OUTPUT);           // diagnostics RED LED
  digitalWrite(RED_LED, LOW);         // this led defaults to "on" so turn it off
  
  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(1);                 // landscape (default is portrait)
  clearScreen();

  // ----- init barometer
  if (!baro.begin()) {
    Serial.println("Error, unable to initialize BMP388, check your wiring");
    tft.setCursor(0, 80);
    tft.setTextColor(cWARN);
    tft.setTextSize(3);
    tft.println("Error!\n Unable to init\n  BMP388 sensor\n   check wiring");
    delay(4000);
  }
  // Set up BMP388 oversampling and filter initialization
  // baro.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);

  // ----- restore configuration settings
  SaveRestore myconfig = SaveRestore(TIME_FOLDER, TIME_FILE, TIME_VERSION);
  if (myconfig.readConfig()) {
    gTimeZone = myconfig.intSetting;
  }
  Serial.print("Time zone restored to "); Serial.println(gTimeZone);

  // ----- announce ourselves
  startSplashScreen();

  startViewScreen();
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTimeGPS = millis();
const int GPS_PROCESS_INTERVAL = 1000;  // milliseconds between updating the model's GPS data
uint32_t prevTimeTouch = millis();
const int TOUCH_PROCESS_INTERVAL = 10;  // milliseconds between polling for touches

void loop() {

  // if our timer or system millis() wrapped around, reset it
  if (prevTimeGPS > millis()) {
    prevTimeGPS = millis();
  }
  if (prevTimeTouch > millis()) {
    prevTimeTouch = millis();
  }

  GPS.read();   // if you can, read the GPS serial port every millisecond in an interrupt
                // this sketch reads the serial port continuously during idle time

  if (GPS.newNMEAreceived()) {
    // sentence received -- verify checksum, parse it
    // GPS parsing: https://learn.adafruit.com/adafruit-ultimate-gps/parsed-data-output
    if (!GPS.parse(GPS.lastNMEA())) {
      // parsing failed -- restart main loop to wait for another sentence
      // this also sets the newNMEAreceived() flag to false
      return;
    } else {
      Serial.print(GPS.lastNMEA());   // debug
    }
  }

  // periodically, process and save the current GPS time
  if (millis() - prevTimeGPS > GPS_PROCESS_INTERVAL) {
    prevTimeGPS = millis();           // restart another interval

    processGPS();                     // update GMT time
    updateView();                     // update current screen
  }

  // periodically check for screen touch
  if (millis() - prevTimeTouch > TOUCH_PROCESS_INTERVAL) {
    prevTimeTouch = millis();         // start another interval
    Point touch;
    if (newScreenTap(&touch)) {
      if (touch.x < gScreenWidth / 2) {
        timePlus();                   // left half screen
      } else {
        timeMinus();                  // right half screen
      }
    }
  }

  // make a small progress bar crawl along bottom edge
  // this gives a sense of how frequently the main loop is executing
  delay(2);         // slow down activity bar so it can be seen
  showActivityBar(239, ILI9341_RED, ILI9341_BLACK);
}
