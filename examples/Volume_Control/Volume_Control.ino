/*
  Example Touchscreen UI for volume control buttons

  Date:     2019-12-26 created
            2019-12-26 moved pin X+ from D6 to A3 to match John's schematic
            2020-05-12 updated TouchScreen code

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  From:     https://forum.arduino.cc/index.php?topic=359928.0

  Purpose:  Displays two rectangles, each of which will trap touches
            and increase/decrease an audio volume setting.
            This tries to change volume by 3 dB on each step.
            Since the linear potentiometer ranges from 0..99,
            there are 9 steps: 0, 1, 2, 4, 8, 16, 32, 64, 99 
            (I am not at all sure these are correct for 3 dB changes.)

            +------------------------+
            |               +------+ |
            | Volume        |  Up  | |
            |               +------+ |
            | Setting: 5    | Down | |
            |               +------+ |
            +------------------------+

            You can expect calibration is needed for your touchscreen.
            (a) measure resistance across X plate with ohmmeter, and
            (b) run this program and watch Serial console reports to
                see what min/max touch values are reported.

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)
            Spec: https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341
            Spec: http://adafru.it/1743
            How to: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2
            SPI Wiring: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/spi-wiring-and-test
            Touchscreen: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/resistive-touchscreen
*/

#include "SPI.h"                    // Serial Peripheral Interface
#include "Adafruit_GFX.h"           // Core graphics display library
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "TouchScreen.h"            // Touchscreen built in to 3.2" Adafruit TFT display

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "Volume Control Demo"
#define PROGRAM_VERSION "v1.0"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

#define SCREEN_ROTATION 1   // 1=landscape, 3=landscape 180-degrees

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
*/

// TFT display and SD card share the hardware SPI interface, and have
// separate 'select' pins to identify the active device on the bus.
#if defined(ARDUINO_AVR_MEGA2560)
  #define TFT_DC   9    // TFT display/command pin
  #define TFT_CS  10    // TFT select pin

#elif defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  // To compile for Feather M0/M4, install "additional boards manager"
  // https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup
  
  #define TFT_CS   5    // TFT select pin
  #define TFT_DC  12    // TFT display/command pin

#else
  #warning You need to define pins for your hardware

#endif

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Touch Screen
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// The demo program used 300 ohms across the X plate
// Barry's display, ILI-9341, measured 295 ohms across the X plate.
#if defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  #define PIN_XP  A3    // Touchscreen X+ can be a digital pin
  #define PIN_XM  A4    // Touchscreen X- must be an analog pin, use "An" notation
  #define PIN_YP  A5    // Touchscreen Y+ must be an analog pin, use "An" notation
  #define PIN_YM   9    // Touchscreen Y- can be a digital pin
#else
  // Arduino Mega 2560 and others
  #define PIN_XP   4    // Touchscreen X+ can be a digital pin
  #define PIN_XM  A3    // Touchscreen X- must be an analog pin, use "An" notation
  #define PIN_YP  A2    // Touchscreen Y+ must be an analog pin, use "An" notation
  #define PIN_YM   5    // Touchscreen Y- can be a digital pin
#endif
TouchScreen ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, 295);

// ------------ typedef's
struct Point {
  int x, y;
};
typedef struct {
  int left, top;
  int width, height;
  char label[12];
} Rectangle;

// ------------ definitions
const int howLongToWait = 6;  // max number of seconds at startup waiting for Serial port to console
const int gScreenWidth = 320; // pixels wide, landscape orientation
const int gScreenHeight = 240;  // pixels high
const int gRotation = 1;  // This rotates the display coordinate system
                          // but not the touchscreen x/y coordinates.
                          // Be aware that your reported touches will have different x/y axis
                          // than the display screen.

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND     0x00A           // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cLABEL          ILI9341_GREEN
#define cVALUE          ILI9341_YELLOW  // 255, 255, 0
#define cBUTTON_OUTLINE ILI9341_CYAN
#define cBUTTON_LABEL   ILI9341_WHITE

#define cWARN           0xF844          // brighter than ILI9341_RED but not pink
#define cTOUCHTARGET    ILI9341_RED     // outline touch-sensitive areas

const int gNumButtons = 2;
Rectangle gaRect[gNumButtons] = {
  //  x,   y, width, height, label
  { 160,   0,   160,    120, "Up" },
  { 160, 120,   160,    120, "Down" },
};

// ------------ global scope
// this whole program exists just only to set this volume level:
const int maxVolume = 99;   // maximum setting of DS1804 digital potentiometer
int gVolume = maxVolume/2;  // speaker volume, 0..99, in units that will be sent directly to DS1804 digital potentiometer

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for all this to settle before sending debug info
  unsigned long targetTime = millis() + howLong*1000;
  while (millis() < targetTime) {
    if (Serial) break;
  }
}
// ----- things that are shown on the display screen
void drawButtons() {
  // draw touch-sensitive areas and label them on screen
  // (these were initialized in the 'gaRect' array above)
  tft.setTextSize(2);

  for (int id = 0; id < gNumButtons; id++) {
    tft.drawRect( gaRect[id].left, gaRect[id].top, 
                  gaRect[id].width, gaRect[id].height, 
                  cBUTTON_OUTLINE);

    tft.setTextColor(cBUTTON_LABEL, cBACKGROUND);
    tft.setCursor(gaRect[id].left + 10, gaRect[id].top + 10);
    tft.print( gaRect[id].label );

    char msg[256];
    sprintf(msg, "Rect[%d] = (%3d,%3d, %d, %d, \"%s\")",
                       id, gaRect[id].left, gaRect[id].top, gaRect[id].width, gaRect[id].height, gaRect[id].label);
    Serial.println(msg);
  }
}
void showVolumeSetting(int vol) {
  tft.setTextSize(2);
  tft.setTextColor( cLABEL, cBACKGROUND );
  tft.setCursor( 8, 60); tft.print("Current");
  tft.setCursor( 9, 80); tft.print("Value = "); 
  tft.setTextColor( cVALUE, cBACKGROUND );
  tft.print(vol);
  tft.print(" ");
}

// ============== touchscreen helpers ==========================

int getRectangleID(int x, int y) {
  // given the screen coordinates of a touch, 
  // and a lookup table of rectangles on the screen,
  // return the index of what button contains it, 
  // or -1 if not found
  int id = -1;
  for (int count = 0; count < gNumButtons; count++) {
    if (x > gaRect[count].left && x < (gaRect[count].left + gaRect[count].width)) {
      if ( y > gaRect[count].top && y < (gaRect[count].top + gaRect[count].height)) {
        id = count;
        break;
      }
    }
  }
  return id;
}

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
  //delay(10);   // no delay: code above completely handles debouncing without blocking the loop
  return result;
}

// 2020-05-12 barry@k7bwh.com
// We need to replace TouchScreen::pressure() and implement TouchScreen::isTouching()

// 2020-05-03 CraigV and barry@k7bwh.com
uint16_t myPressure(void) {
  pinMode(PIN_XP, OUTPUT);   digitalWrite(PIN_XP, LOW);   // Set X+ to ground
  pinMode(PIN_YM, OUTPUT);   digitalWrite(PIN_YM, HIGH);  // Set Y- to VCC

  digitalWrite(PIN_XM, LOW); pinMode(PIN_XM, INPUT);      // Hi-Z X-
  digitalWrite(PIN_YP, LOW); pinMode(PIN_YP, INPUT);      // Hi-Z Y+

  int z1 = analogRead(PIN_XM);
  int z2 = 1023-analogRead(PIN_YP);

  return (uint16_t) ((z1+z2)/2);
}

// "isTouching()" is defined in touch.h but is not implemented Adafruit's TouchScreen library
// Note - For Griduino, if this function takes longer than 8 msec it can cause erratic GPS readings
// so we recommend against using https://forum.arduino.cc/index.php?topic=449719.0
bool TouchScreen::isTouching(void) {
  #define TOUCHPRESSURE 200       // Minimum pressure we consider true pressing
  static bool button_state = false;
  uint16_t pres_val = ::myPressure();

  if ((button_state == false) && (pres_val > TOUCHPRESSURE)) {
    Serial.print(". pressed, pressure = "); Serial.println(pres_val);     // debug
    button_state = true;
  }

  if ((button_state == true) && (pres_val < TOUCHPRESSURE)) {
    Serial.print(". released, pressure = "); Serial.println(pres_val);       // debug
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

  // magic numbers need calibration for each individual display
  // measured by running this program, watching Serial console reports
  int x0 = 260;     // lowest expected X-axis reading
  int x1 = 800;     // highest X
  int y0 = 250;     // lowest expected Y-axis reading
  int y1 = 825;     // highest Y

  // gRotation = 1: x-,y-axis exchanged
  screen->x = map(touch.y, x0, x1,  0, tft.width());
  screen->y = map(touch.x, y0, y1,  tft.height(), 0 );

  char msg[256];
  sprintf(msg, "touch(%d,%d, %d) -> screen(%d,%d)",
                touch.x, touch.y, touch.z, screen->x, screen->y);
  Serial.println(msg);

  screen->x = constrain(screen->x, 0, tft.width());
  screen->y = constrain(screen->y, 0, tft.height());
  return;
}

void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

// ----- Do The Thing
void increaseVolume() {
  Serial.println("+++ Increase volume! Yay!");
  gVolume = constrain(gVolume*2+1, 0, maxVolume);
  showVolumeSetting(gVolume);
  // todo: send volume change to DS1804 digital potentiometer
}
void decreaseVolume() {
  Serial.println("-v-v-v- Decrease volume :-(");
  gVolume = constrain(gVolume/2, 0, maxVolume);
  showVolumeSetting(gVolume);
  // todo: send volume change to DS1804 digital potentiometer
}

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  static int addDotX = 10;                    // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count = 0;
  const int SCALEF = 2048;                    // how much to slow it down so it becomes visible

  count = (count + 1) % SCALEF;
  if (count == 0) {
    addDotX = (addDotX + 1) % tft.width();    // advance
    rmvDotX = (rmvDotX + 1) % tft.width();    // advance
    tft.drawPixel(addDotX, row, foreground);  // write new
    tft.drawPixel(rmvDotX, row, background);  // erase old
  }
}

//=========== setup ============================================
void setup() {

  // ----- init serial monitor
  Serial.begin(115200);                               // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);                       // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(gRotation);         // 1=landscape (default is 0=portrait)
  clearScreen();

  tft.setTextSize(3);

  // ----- init screen appearance
  tft.setTextColor(cLABEL, cBACKGROUND);
  tft.setCursor( 8, 24);
  tft.print("Volume");

  showVolumeSetting(gVolume);

  drawButtons();
}

//=========== main work loop ===================================
void loop() {

  // if there's touchscreen input, handle it
  Point touch;
  if (newScreenTap(&touch)) {
    int id = getRectangleID(touch.x, touch.y);
    switch (id) {
      case 0:
        increaseVolume();
        break;
      case 1:
        decreaseVolume();
        break;
      default:
        // nothing - user did not touch inside one of the button areas
        break;
    }
  }
  // make a small progress bar crawl along bottom edge
  // this gives a sense of how frequently the main loop is executing
  //showActivityBar(239, ILI9341_RED, ILI9341_BLACK);
}
