/*
  Altimeter -- a functional altimeter with comparison to GPS altitudes

  Date:     2020-03-06 created 0.9

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Display altimeter readings on a 3.2" display. 
            Use the touchscreen to enter current sea-level barometer setting.
            The GPS on Griduino also offers altitude readings, so display
            them simultaneously to help understand the relative accuracies
            of these two methods.

            This is a standalone program with a single display screen,
            written in the process of developing Griduino.

            +---------------------------------+
            | Griduino Altimeter Demo         |. . .line1
            |                                 |
            | Barometer:      17.8 feet       |. . .line2
            | GPS:           123.4 feet       |. . .line3
            |                                 |
            | Enter your current local        |. . .line4
            | pressure at sea level:          |. . .line5
            +-----+                     +-----+
            |  ^  |     1016.7 hPa      |  v  |. . .line6
            +-----+---------------------+-----+

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

         4. Adafruit BMP388 - Precision Barometric Pressure and Altimeter
            Spec: https://www.adafruit.com/product/3966

*/

#include <Wire.h>
#include "SPI.h"                  // Serial Peripheral Interface
#include "Adafruit_GFX.h"         // Core graphics display library
#include "Adafruit_ILI9341.h"     // TFT color display library
#include "Adafruit_GPS.h"         // Ultimate GPS library
#include "TouchScreen.h"          // Touchscreen built in to 3.2" Adafruit TFT display
#include "Adafruit_BMP3XX.h"      // Precision barometric sensor

// ------- Identity for console
#define PROGRAM_TITLE   "Griduino Altimeter Demo"
#define PROGRAM_VERSION "v0.9"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"

// ---------- Hardware Wiring ----------
/*                                Arduino       Adafruit
  ___Label__Description______________Mega_______Feather M4__________Resource____
TFT Power:
   GND  - Ground                  - ground      - J2 Pin 13
   VIN  - VCC                     - 5v          - Pin 10 J5 Vusb
TFT SPI: 
   SCK  - SPI Serial Clock        - Digital 52  - SCK (J2 Pin 6)  - uses hardw SPI
   MISO - SPI Master In Slave Out - Digital 50  - MI  (J2 Pin 4)  - uses hardw SPI
   MOSI - SPI Master Out Slave In - Digital 51  - MO  (J2 Pin 5)  - uses hardw SPI
   CS   - SPI Chip Select         - Digital 10  - D5  (Pin 3 J5)
   D/C  - SPI Data/Command        - Digital  9  - D12 (Pin 8 J5)
TFT Resistive touch:
   X+   - Touch Horizontal axis   - Digital  4  - A3  (Pin 4 J5)
   X-   - Touch Horizontal        - Analog  A3  - A4  (J2 Pin 8)  - uses analog A/D
   Y+   - Touch Vertical axis     - Analog  A2  - A5  (J2 Pin 7)  - uses analog A/D
   Y-   - Touch Vertical          - Digital  5  - D9  (Pin 5 J5)
TFT No connection:
   3.3  - 3.3v output             - n/c         - n/c
   RST  - Reset                   - n/c         - n/c
   IM0/3- Interface Control Pins  - n/c         - n/c
GPS:
   VIN  - VCC                     - 5v          - Vin
   GND  - Ground                  - ground      - ground
   >RX  - data into GPS           - TX1 pin 18  - TX  (J2 Pin 2)  - uses hardware UART
   <TX  - data out of GPS         - RX1 pin 19  - RX  (J2 Pin 3)  - uses hardware UART
*/

// TFT display and SD card share the hardware SPI interface, and have
// separate 'select' pins to identify the active device on the bus.
#if defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  // To compile for Feather M0/M4, install "additional boards manager"
  // https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup
  
  #define TFT_BL   4    // TFT backlight
  #define TFT_CS   5    // TFT select pin
  #define TFT_DC  12    // TFT display/command pin

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
// "top half" and "bottom half" without looking. Exact precision is not essential.
// "up" on the left, and "down" on the right. Touch target precision is not essential.
#if defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  #define PIN_XP  A3    // Touchscreen X+ can be a digital pin
  #define PIN_XM  A4    // Touchscreen X- must be an analog pin, use "An" notation
  #define PIN_YP  A5    // Touchscreen Y+ must be an analog pin, use "An" notation
  #define PIN_YM   9    // Touchscreen Y- can be a digital pin
#else
  // todo: Unknown platform
  #warning You need to define pins for your hardware

#endif
TouchScreen ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, 295);

// ---------- Barometric Sensor
Adafruit_BMP3XX baro(BMP_CS); // hardware SPI

// ---------- Onboard LED
#define RED_LED 13    // diagnostics RED LED

//const int ledPin = 14;     // for blinking

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

typedef struct {
  char text[26];
  int x;
  int y;
  uint16_t color;
} Label;

typedef struct {
  char text[26];
  int x;
  int y;
  int w;
  int h;
  int radius;
  uint16_t color;
} Button;

// ------------ definitions
#define gScreenWidth 320      // pixels wide, landscape orientation
#define gScreenHeight 240     // pixels high
#define FEET_PER_METER 3.28084

const int howLongToWait = 8;  // max number of seconds at startup waiting for Serial port to console

// ----- screen layout
// screen pixel coordinates for top left of character cell
#define yRow1   0            // title: "Griduino Altimeter Demo"
#define yRow2   yRow1 + 40   // barometer reading
#define yRow3   yRow2 + 24   // GPS altitude
#define yRow4   138          // label: "Your current local"
#define yRow5   yRow4 + 20   // label: "pressure at sea level:"
#define yRow6   yRow5 + 42   // value: "1016.2 hPa"
const int xLabel = 8;        // indent labels, slight margin to left edge of screen

// ----- color scheme
#define cBACKGROUND   0x00A   // a little darker than ILI9341_NAVY, but not black
#define cSCALECOLOR   0xF844  // color picker: http://www.barth-dev.de/online/rgb565-color-picker/
#define cTEXTCOLOR    ILI9341_CYAN
#define cLABEL        ILI9341_GREEN
#define cVALUE        ILI9341_YELLOW
#define cINPUT        ILI9341_WHITE
#define cBUTTONFILL    ILI9341_NAVY
#define cBUTTONOUTLINE ILI9341_CYAN
#define cBUTTONLABEL   ILI9341_YELLOW
#define cWARN         0xF844        // a little brighter than ILI9341_RED

// ------------ global GPS data
double gLatitude = 0;               // GPS position, floating point, decimal degrees
double gLongitude = 0;              // GPS position, floating point, decimal degrees
//float  gAltitude = 0;               // Altitude in meters above MSL
uint8_t gSatellites = 0;            // number of satellites in use

// ------------ global barometric data
float gSeaLevelPressure = 1017.4;   // default starting value; adjustable by touchscreen
float inchesHg;
float Pa;
float hPa;
float feet;
float tempF;
float tempC;
float altMeters;
float altFeet;

const int nButtons = 2;
const int margin = 10;      // slight margin between button border and edge of screen
const int radius = 10;      // rounded corners
Button buttons[nButtons] = {
  // text   x,y        w,h        r      color
  {"",      8, 180,   64, 60,  radius, cBUTTONLABEL }, // Up
  {"",     244,180,   64, 60,  radius, cBUTTONLABEL }, // Down
};

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

// ============== GPS helpers ==================================
void getGPSData() {
  echoGPSinfo();    // send GPS statistics to serial console for debug

  if (GPS.fix) {
    // update model
    gLatitude = GPS.latitudeDegrees;    // position as double-precision float
    gLongitude = GPS.longitudeDegrees;
    //gAltitude = GPS.altitude;

  } else {
    // nothing
  }
  gSatellites = GPS.satellites;
}
// Provide formatted GMT date/time "2019-12-31  10:11:12"
void getDateTime(char* result) {
  // result = char[25] = string buffer to modify
  //if (GPS.fix) {
    int yy = GPS.year;
    if (yy >= 19) {
      // if GPS reports a date before 19, then it's bogus
      // and it's displayed as-is
      yy += 2000;
    }
    int mo = GPS.month;
    int dd = GPS.day;
    int hh = GPS.hour;
    int mm = GPS.minute;
    int ss = GPS.seconds;
    snprintf(result, 25, "%04d-%02d-%02d  %02d:%02d:%02d",
                          yy,  mo,  dd,  hh,  mm,  ss);
  //} else {
  //  strncpy(result, "0000-00-00 hh:mm:ss GMT", 25);
  //}
}
void echoGPSinfo() {
  // send GPS statistics to serial console for desktop debugging
  char sDate[20];         // strlen("0000-00-00 hh:mm:ss") = 19
  getDateTime(sDate);
  Serial.print("GPS: ");
  Serial.print(sDate);
  Serial.print("  Fix("); Serial.print((int)GPS.fix); Serial.println(")");

  if (GPS.fix) {
    //Serial.print("   Loc("); Serial.print(gsLatitude); Serial.print(","); Serial.print(gsLongitude);
    Serial.print("   Quality("); Serial.print((int)GPS.fixquality);
    Serial.print(") Sats("); Serial.print((int)GPS.satellites);
    Serial.print(") Speed("); Serial.print(GPS.speed); Serial.print(" knots");
    Serial.print(") Angle("); Serial.print(GPS.angle);
    Serial.print(") Alt("); Serial.print(GPS.altitude*FEET_PER_METER); Serial.print(" ft");
    Serial.println(")");
  }
}

// ============== barometer helpers ============================
void getBaroData() {
  if (!baro.performReading()) {
    Serial.println("Error, failed to read barometer");
  }
  // continue anyway, for demo
  tempC = baro.temperature;
  tempF= tempC * 9 / 5 + 32;
  Pa = baro.pressure;
  hPa = Pa / 100;
  inchesHg = 0.0002953 * Pa;
  altMeters = baro.readAltitude(gSeaLevelPressure);
  altFeet = altMeters * FEET_PER_METER;
}

void increaseSeaLevelPressure() {
  gSeaLevelPressure += 0.1;
}

void decreaseSeaLevelPressure() {
  gSeaLevelPressure -= 0.1;
}

// ============== Screen Helpers ===============================
void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}
void showReadings() {
  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR);
  tft.setTextSize(2);
  tft.print("Griduino Altimeter Demo");

  tft.setCursor(xLabel, yRow2);
  tft.setTextColor(cLABEL);
  tft.print("Barometer: ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(altFeet, 1);
  tft.println(" feet  ");

  tft.setCursor(xLabel, yRow3);
  tft.setTextColor(cLABEL);
  tft.print("GPS:       ");
  if (GPS.fix) {
    tft.setTextColor(cVALUE, cBACKGROUND);
    tft.print(GPS.altitude * FEET_PER_METER, 1);
  } else {
    tft.setTextColor(cWARN, cBACKGROUND);
    tft.print("xxx.x");
  }
  tft.println(" feet  ");

  tft.setCursor(xLabel, yRow4);
  tft.setTextColor(cLABEL);
  tft.print("Enter your current local");
  tft.setCursor(xLabel, yRow5);
  tft.print("pressure at sea level:");

  tft.setCursor(gScreenWidth/2-66, yRow6);
  tft.setTextColor(cINPUT, cBACKGROUND);
  tft.print(gSeaLevelPressure, 1);
  tft.print(" hPa ");

  drawButtons();
}

void drawButtons() {
  // ----- draw buttons
  for (int ii=0; ii<nButtons; ii++) {
    Button item = buttons[ii];
    tft.fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft.drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    tft.setCursor(item.x+20, item.y+32);
    tft.setTextColor(cINPUT);
    tft.print(item.text);
  }

  // ----- icons on buttons
  int sz = 12;
  int xx, yy;

  // UP button
  xx = buttons[0].x + buttons[0].w/2; // centerline is halfway in the middle
  yy = buttons[0].y + buttons[0].h/2; // baseline is halfway in the middle
  //                x0,y0,     x1,y1,     x2,y2,   color
  tft.fillTriangle(xx-sz,yy,  xx+sz,yy,  xx,yy-sz, cINPUT);  // arrow UP

  // DOWN button
  xx = buttons[1].x + buttons[1].w/2; // centerline is halfway in the middle
  yy = buttons[1].y + buttons[1].h/2; // baseline is halfway in the middle
  tft.fillTriangle(xx-sz,yy,  xx+sz,yy,  xx,yy+sz, cINPUT);  // arrow DOWN
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

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(1);                 // landscape (default is portrait)
  clearScreen();

  // ----- announce ourselves
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

  delay(3000);
  clearScreen();
/*
// --> bmp3_defs.h

// API success codes
#define BMP3_OK        INT8_C(0)

// API error codes
#define BMP3_E_NULL_PTR                   INT8_C(-1)
#define BMP3_E_DEV_NOT_FOUND              INT8_C(-2)
#define BMP3_E_INVALID_ODR_OSR_SETTINGS   INT8_C(-3)
#define BMP3_E_CMD_EXEC_FAILED            INT8_C(-4)
#define BMP3_E_CONFIGURATION_ERR          INT8_C(-5)
#define BMP3_E_INVALID_LEN                INT8_C(-6)
#define BMP3_E_COMM_FAIL                  INT8_C(-7)
#define BMP3_E_FIFO_WATERMARK_NOT_REACHED INT8_C(-8)

// API warning codes
#define BMP3_W_SENSOR_NOT_ENABLED         UINT8_C(1)
#define BMP3_W_INVALID_FIFO_REQ_FRAME_CNT UINT8_C(2)
*/

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
  baro.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  baro.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  baro.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_7);
  // baro.setOutputDataRate(BMP3_ODR_50_HZ);

  // Get first data point (done twice because first reading is always bad)
  getBaroData();
  getBaroData();
  getGPSData();
  showReadings();
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTimer1 = millis();    // timer for value update (5 min)
//nt32_t prevTimer2 = millis();    // timer for graph/array update (20 min)
//nt32_t prevTimer3 = millis();    // timer for emergency alert (10 sec)

const int READ_BAROMETER_INTERVAL = 1*1000;     // Timer 1
const int LOG_PRESSURE_INTERVAL = 20*60*1000;   // Timer 2
const int CHECK_CRASH_INTERVAL = 10*1000;       // Timer 3

void loop() {

  // if our timer or system millis() wrapped around, reset it
  if (prevTimer1 > millis()) { prevTimer1 = millis(); }
  // (prevTimer2 > millis()) { prevTimer2 = millis(); }
  // (prevTimer3 > millis()) { prevTimer3 = millis(); }

  GPS.read();   // if you can, read the GPS serial port every millisecond in an interrupt

  if (GPS.newNMEAreceived()) {
    // sentence received -- verify checksum, parse it
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and thereby might miss other sentences
    // so be very wary if using OUTPUT_ALLDATA, which generates loads of sentences,
    // since trying to print them all out is time-consuming
    // GPS parsing: https://learn.adafruit.com/adafruit-ultimate-gps/parsed-data-output
    if (!GPS.parse(GPS.lastNMEA())) {
      // parsing failed -- restart main loop to wait for another sentence
      // this also sets the newNMEAreceived() flag to false
      return;
    } else {
      Serial.print(GPS.lastNMEA());   // debug
    }
  }

  // periodically read temp and pressure, and display everything
  if (millis() - prevTimer1 > READ_BAROMETER_INTERVAL) {
    getBaroData();
    getGPSData();

    showReadings();
    prevTimer1 = millis();
  }

  // if there's touchscreen input, handle it
  Point touch;
  if (newScreenTap(&touch)) {
    if (touch.x < gScreenWidth / 2) {
      increaseSeaLevelPressure();
    } else {
      decreaseSeaLevelPressure();
    }
    getBaroData();
    showReadings();
  }
}
