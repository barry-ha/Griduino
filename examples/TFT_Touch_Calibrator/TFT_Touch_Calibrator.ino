/*
  TFT Touchscreen Calibrator - Touch screen with X, Y and Z (pressure) readings

  Date:
            2020-11-23 created touch calibrator

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Interactive display of the effects of Touch Screen calibration settings.
            This program shows the current compiled touch settings alongside the
            current realtime measurements as you touch the screen.

  Usage:    1. Compile and run the program
            2. Slide a finger gently around the screen edges
            3. It will plot a scattergram of dots showing how resistance measurements are mapped to screen area
            4. Edit source code "Touch configuration parameters" to adjust the mapping
               a. use larger X value to move apparent screen response left
               b. use larger Y value to move apparent screen response down
            5. Recompile and run this again

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)     https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341    https://www.adafruit.com/product/1743
            How to:      https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2
            SPI Wiring:  https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/spi-wiring-and-test
            Touchscreen: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/resistive-touchscreen

*/

#include <Adafruit_ILI9341.h>         // TFT color display library
#include "constants.h"                // Griduino constants, colors, typedefs
#include "TouchScreen.h"              // Touchscreen built in to 3.2" Adafruit TFT display
#include "TextField.h"                // Optimize TFT display text for proportional fonts

// ------- TFT 4-Wire Resistive Touch Screen configuration parameters
#define TOUCHPRESSURE 200             // Minimum pressure threshhold considered an actual "press"
#define X_MIN_OHMS    240             // Expected range of measured X-axis readings
#define X_MAX_OHMS    800
#define Y_MIN_OHMS    320             // Expected range of measured Y-axis readings
#define Y_MAX_OHMS    760
#define XP_XM_OHMS    295             // Resistance in ohms between X+ and X- to calibrate pressure
                                      // measure this with an ohmmeter while Griduino turned off

// ------- Identity for splash screen and console --------
#define PROGRAM_NAME    "TFT Touch Calibrator"

#define SCREEN_ROTATION 1             // 1=landscape, 3=landscape 180-degrees

// ---------- Hardware Wiring ----------
/* Same as Griduino platform
*/

// TFT display and SD card share the hardware SPI interface, and have
// separate 'select' pins to identify the active device on the bus.
#if defined(SAMD_SERIES)
  // Adafruit Feather M4 Express pin definitions
  // To compile for Feather M0/M4, install "additional boards manager"
  // https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup
  
  #define TFT_BL   4                  // TFT backlight
  #define TFT_CS   5                  // TFT chip select pin
  #define TFT_DC  12                  // TFT display/command pin

#elif defined(ARDUINO_AVR_MEGA2560)
  #define TFT_BL   6                  // TFT backlight
  #define TFT_DC   9                  // TFT display/command pin
  #define TFT_CS  10                  // TFT chip select pin

#else
  #warning You need to define pins for your hardware

#endif

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Touch Screen
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
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
  #warning You need to define pins for your hardware

#endif
TouchScreen ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, XP_XM_OHMS);

// ------------ definitions
const int howLongToWait = 5;          // max number of seconds at startup waiting for Serial port to console
#define SCREEN_ROTATION 1             // 1=landscape, 3=landscape 180 degrees

#define gScreenWidth 320              // pixels wide, landscape orientation

// ============== touchscreen helpers ==========================
bool gTouching = false;               // keep track of previous state
bool newScreenTap(Point* pPoint) {
  // find leading edge of a screen touch
  // returns TRUE only once on initial screen press
  // if true, also return screen coordinates of the touch

  bool result = false;                // assume no touch
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

  // setRotation(1) = landscape orientation = x-,y-axis exchanged
  //          map(value         in_min,in_max,       out_min,out_max)
  screen->x = map(touch.y,  X_MIN_OHMS,X_MAX_OHMS,    0, tft.width());
  screen->y = map(touch.x,  Y_MAX_OHMS,Y_MIN_OHMS,    0, tft.height());
  if (tft.getRotation() == 3) {
    // if display is flipped, then also flip both x,y touchscreen coords
    screen->x = tft.width() - screen->x;
    screen->y = tft.height() - screen->y;
  }
  return;
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong*1000;
  int x = 0;
  int y = 0;
  int w = gScreenWidth;
  int h = gScreenHeight;
  bool done = false;
  
  while (millis() < targetTime) {
    if (Serial) break;
    if (done) break;
    delay(15);
  }
}

// ========== splash screen ====================================
void startSplashScreen() {
  const int x1 = 8;                   // indent labels, slight margin on left edge of screen
  const int yRow1 = 20;               // title: "Touchscreen Calibrator"
  const int yRow2 = yRow1 + 40;       // program version
  const int yRow3 = yRow2 + 20;       // compiled date
  const int yRow4 = yRow3 + 40;       // author line 1
  const int yRow5 = yRow4 + 20;       // author line 2
  
  TextField txtSplash[] = {
    //     text               x,y       color  
    {PROGRAM_NAME,     x1,yRow1,  cTEXTCOLOR}, // [0] program title
    {PROGRAM_VERSION,  x1,yRow2,  cLABEL},     // [1] program version
    {PROGRAM_COMPILED, x1,yRow3,  cLABEL},     // [2] compiled date
    {PROGRAM_LINE1,    x1,yRow4,  cLABEL},     // [3] credits line 1
    {PROGRAM_LINE2,    x1,yRow5,  cLABEL},     // [4] credits line 2
  };
  const int numSplashFields = sizeof(txtSplash)/sizeof(TextField);

  clearScreen();                               // clear screen
  txtSplash[0].setBackground(cBACKGROUND);     // set background for all TextFields
  TextField::setTextDirty( txtSplash, numSplashFields ); // make sure all fields are updated

  setFontSize(eFONTSMALLEST);
  for (int ii=0; ii<numSplashFields; ii++) {
    txtSplash[ii].print();
  }
}

void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  static int addDotX = 10;                    // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count = 0;
  const int SCALEF = 128;                     // how much to slow it down so it becomes visible

  count = (count + 1) % SCALEF;
  if (count == 0) {
    addDotX = (addDotX + 1) % tft.width();    // advance
    rmvDotX = (rmvDotX + 1) % tft.width();    // advance
    tft.drawPixel(addDotX, row, foreground);  // write new
    tft.drawPixel(rmvDotX, row, background);  // erase old
  }
}

// ========== main screen ====================================
  const int xul = 8;                  // upper left corner of screen text
  const int yul = 0;                  // 
  const int ht = 20;                  // row height
/*
              xul
       yul..+-:-----------------------------------------+
            | Touch Screen Calibrator           800 max | 1
            | Please touch screen                    :  | 2
            | normally near edges                    :  | 3
            |                                        :  | 4
            | Current pressure = ...            ...  X  | 5
            | Pressure threshhold = 200              :  | 6
            |                                        :  | 7
            |                                        :  | 8
            |                                   240 min | 9
            | 320              ...                  760 | 10
            | min ~ ~ ~ ~ ~ ~ ~ Y ~ ~ ~ ~ ~ ~ ~ ~ ~ max | 11
            +-------------------:--:------------:-----:-+
                                x1 x2           x3    x4
*/
  const int x1 = gScreenWidth/2-10;   // = 320/2 = 160
  const int x2 = x1 + 10;
  const int x3 = gScreenWidth-80;
  const int x4 = gScreenWidth-8;
  #define cXGROUP ILI9341_MAGENTA
  #define cYGROUP ILI9341_GREENYELLOW
  const int row1 = yul + ht;
  const int row2 = row1 + ht;
  const int row3 = row2 + ht;
  const int row4 = row3 + ht;
  const int row5 = row4 + ht;
  const int row6 = row5 + ht;
  const int row7 = row6 + ht;
  const int row8 = row7 + ht;
  const int row9 = row8 + ht;
  const int row10 = row9 + ht + 10;   // a little extra room for bottom two rows
  const int row11 = row10 + ht;

  TextField txtScreen[] = {
    // row 1
    {     PROGRAM_NAME,           xul,row1,  cTEXTCOLOR}, // [0]
    {     X_MAX_OHMS,              x3,row1,  cXGROUP},    // [1]
    {     "max",                   x4,row1,  cXGROUP, ALIGNRIGHT}, // [2]
    // row 2
    {     "Please touch screen",  xul,row2,  cLABEL},     // [3]
    // row 3
    {     "normally near edges",  xul,row3,  cLABEL},     // [4]
    // row 4
    // row 5
    {     "Current pressure:",    xul,row5,  cLABEL},     // [5]
    {     "ppp",                   x2,row5,  cVALUE},     // [6] current pressure value
    {     "xxx",                   x3,row5,  cVALUE},     // [7] current X value
    {     "X   ",                  x4,row5,  cXGROUP, ALIGNRIGHT}, // [8]
    // row 6
    {     "Threshhold:",          xul,row6,  cLABEL},     // [9]
    {     TOUCHPRESSURE,           x2,row6,  cLABEL},     // [10]
    // row 7
    // row 8
    // row 9
    {     X_MIN_OHMS,              x3,row9,  cXGROUP},    // [11]
    {     "min",                   x4,row9,  cXGROUP, ALIGNRIGHT}, // [12]
    // row 10
    {     Y_MIN_OHMS,             xul,row10, cYGROUP},    // [13]
    {     "yyy",                   x1,row10, cVALUE},     // [14] current Y value
    {     Y_MAX_OHMS,              x4,row10, cYGROUP, ALIGNRIGHT}, // [15]
    // row 11
    {     "min",                  xul,row11, cYGROUP},    // [16]
    {     "Y",                     x1,row11, cYGROUP},    // [17]
    {     "max",                   x4,row11, cYGROUP, ALIGNRIGHT}, // [18]
  };
  const int numScreenFields = sizeof(txtScreen)/sizeof(TextField);

// ========== main screen ====================================
void startScreen() {

  clearScreen();                            // clear screen
  txtScreen[0].setBackground(cBACKGROUND);  // set background for all TextFields
  TextField::setTextDirty( txtScreen, numScreenFields ); // make sure all fields are updated

  setFontSize(eFONTSMALLEST);
  for (int ii=0; ii<numScreenFields; ii++) {
    txtScreen[ii].print();
  }
}
void updateScreen(TSPoint tp) {
  if (tp.z > 0) {
    txtScreen[6].print(tp.z);         // current pressure value
  }
  txtScreen[7].print(tp.x);           // current X value
  txtScreen[14].print(tp.y);          // current Y value
}

void labelAxis() {
  const int xV = x3-4;                // screen X coord of vertical line
  const int yH = row9+10;             // screen Y coord of horizontal line

  #define nLines 2
  TwoPoints myLines[nLines] = {
    // x1,y1    x2,y2
    { xul,yH,   x4,yH,   cYGROUP},    // horiz across bottom row
    {  xV,yul,  xV,row9+10, cXGROUP}, // vert along rhs
  };

  for (int ii=0; ii<nLines; ii++) {
    TwoPoints item = myLines[ii];
    if (item.x1 == item.x2) {
      // ----- vertical dotted line -----
      for (int yy=item.y1; yy<item.y2; yy++) {
        if (yy % 2) {
          tft.drawPixel(item.x1, yy, item.color);
        }
      }
    } else {
      // ----- horizontal dotted line -----
      for (int xx=item.x1; xx<item.x2; xx++) {
        if (xx % 2) {
          tft.drawPixel(xx, item.y1, cYGROUP);
        }
      }
    }
  }
}

//=========== setup ============================================
void setup() {

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(1);                 // 1=landscape (default is 0=portrait)
  tft.fillScreen(ILI9341_BLACK);      // note that "begin()" does not clear screen 

  // ----- announce ourselves
  startSplashScreen();

  // ----- init serial monitor
  Serial.begin(115200);               // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);       // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_NAME " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init touch screen
  ts.pressureThreshhold = TOUCHPRESSURE;

  startScreen();
  labelAxis();
  #ifdef SHOW_SCREEN_CENTERLINE
    // show centerline at    x1,y1             x2,row2             color
    tft.drawLine( tft.width()/2,0,   tft.width()/2,tft.height(), cTEXTFAINT); // debug
    tft.drawLine( 0,tft.height()/2,  tft.width(),tft.height()/2, cTEXTFAINT); // debug
  #endif

}

//=========== main work loop ===================================

void loop() {
  // a point object holds x y and z coordinates
  TSPoint p = ts.getPoint();          // read touch screen

  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means "no press"
  if (p.z > ts.pressureThreshhold) {
     Serial.print("x,y = "); Serial.print(p.x);
     Serial.print(","); Serial.print(p.y);
     Serial.print("\tPressure = "); Serial.println(p.z);

     //tft.setCursor(p.x, p.y);
     //tft.print("x");
  }

  // if there's touchscreen input, handle it
  Point touch;
  if (newScreenTap(&touch)) {

    const int radius = 1;
    tft.fillCircle(touch.x, touch.y, radius, cTOUCHTARGET);
    updateScreen(p);

  }

  // make a small progress bar crawl along bottom edge
  // this gives a sense of how frequently the main loop is executing
  showActivityBar(tft.height()-1, ILI9341_RED, ILI9341_BLACK);
}
