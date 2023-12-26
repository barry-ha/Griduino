// Please format this file with clang before check-in to GitHub
/*
  TFT Touchscreen Calibrator - Touch screen with X, Y and Z (pressure) readings

  Version history:
            2023-12-24 improved debounce by adding hysteresis
            2020-11-23 created touch calibrator
            2022-05-20 updated calibration constants X_MIN_OHMS, X_MAX_OHMS, Y_MIN_OHMS, Y_MAX_OHMS

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Interactive display of the effects of Touch Screen calibration settings.
            This program shows the current compiled touch settings alongside the
            current realtime measurements as you touch the screen.

  Usage:    1. Compile and run the program
            2. Tap gently with a stylus around the center, then work your way around the screen edges
            3. Each tap leaves a dot, showing how resistance measurements are mapped to screen area
            4. Edit source code "Touch configuration parameters" to adjust the mapping
               a. use larger X ohms to move apparent screen response left
               b. use larger Y ohms to move apparent screen response down
            5. Recompile and run this again
            6. Once you're satisifed with the values, you can copy them directly into Griduino.ino

  Coordinate system:
            X and Y are in terms of the native touchscreen coordinates, which corresponds to
            native portrait mode (screen rotation 0).

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)     https://www.adafruit.com/product/3857

         2. Adafruit 3.2" TFT color LCD display ILI-9341    https://www.adafruit.com/product/1743
            How to:      https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2
            SPI Wiring:  https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/spi-wiring-and-test
            Touchscreen: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/resistive-touchscreen

*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include <TouchScreen.h>        // Touchscreen built in to 3.2" Adafruit TFT display
#include "constants.h"          // Griduino constants, colors, typedefs
#include "hardware.h"           // Griduino pin definitions
#include "TextField.h"          // Optimize TFT display text for proportional fonts

// ------- Identity for splash screen and console --------
#define PROGRAM_NAME "TFT Touch Calibrator"

// ---------- extern
extern bool newScreenTap(TSPoint *pPoint, int orientation);   // Touch.cpp
extern void initTouchScreen(void);                            // Touch.cpp

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ------------ definitions
const int howLongToWait = 6;   // max number of seconds at startup waiting for Serial port to console

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong * 1000;

  int x     = 0;
  int y     = 0;
  int w     = gScreenWidth;
  int h     = gScreenHeight;
  bool done = false;

  while (millis() < targetTime) {
    if (Serial)
      break;
    if (done)
      break;
    delay(15);
  }
}

// ========== splash screen ====================================
void startSplashScreen() {
  const int x1    = 8;            // indent labels, slight margin on left edge of screen
  const int yRow1 = 20;           // title: "Touchscreen Calibrator"
  const int yRow2 = yRow1 + 40;   // program version
  const int yRow3 = yRow2 + 20;   // compiled date
  const int yRow4 = yRow3 + 40;   // author line 1
  const int yRow5 = yRow4 + 20;   // author line 2
  const int yRow6 = yRow5 + 30;   // instructions line 1
  const int yRow7 = yRow6 + 20;   // instructions line 2

  TextField txtSplash[] = {
      //     text               x,y       color
      {PROGRAM_NAME, x1, yRow1, cTEXTCOLOR},              // [0] program title
      {PROGRAM_VERSION, x1, yRow2, cLABEL},               // [1] program version
      {PROGRAM_COMPILED, x1, yRow3, cLABEL},              // [2] compiled date
      {PROGRAM_LINE1, x1, yRow4, cLABEL},                 // [3] credits line 1
      {PROGRAM_LINE2, x1, yRow5, cLABEL},                 // [4] credits line 2
      {"Tap screen in the middle,", x1, yRow6, cLABEL},   // [5] hint line 1
      {"and then toward edges", x1, yRow7, cLABEL},       // [6] hint line 2
  };
  const int numSplashFields = sizeof(txtSplash) / sizeof(TextField);

  clearScreen();                                         // clear screen
  txtSplash[0].setBackground(cBACKGROUND);               // set background for all TextFields
  TextField::setTextDirty(txtSplash, numSplashFields);   // make sure all fields are updated

  setFontSize(eFONTSMALLEST);
  for (int ii = 0; ii < numSplashFields; ii++) {
    txtSplash[ii].print();
  }
}

void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  static int addDotX = 10;   // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count   = 0;
  const int SCALEF   = 32;   // how much to slow it down so it becomes visible

  count = (count + 1) % SCALEF;
  if (count == 0) {
    addDotX = (addDotX + 1) % tft.width();     // advance
    rmvDotX = (rmvDotX + 1) % tft.width();     // advance
    tft.drawPixel(addDotX, row, foreground);   // write new
    tft.drawPixel(rmvDotX, row, background);   // erase old
  }
}

/* ========== main screen ====================================
        xul
  yul..+-:-----------------------------------------+
       | Touch Screen Calibrator           800 max | 1
       |                                        :  | 2
       |                                        :  | 3   ^
       |                                        :  | 4   |
       | Current pressure = ...            ...  X  | 5   X-axis
       | Pressure threshhold = 200              :  | 6   |
       |                                        :  | 7   v
       |                                        :  | 8
       |                                   240 min | 9
       | 320              ...                  760 | 10
       | min ~ ~ ~ ~ ~ ~ ~ Y ~ ~ ~ ~ ~ ~ ~ ~ ~ max | 11
       +-------------------:--:------------:-----:-+
  pixel coords:          x1 x2           x3    x4

  touch coords:      <-- Y-axis -->
*/
#define cXGROUP ILI9341_MAGENTA
#define cYGROUP ILI9341_GREENYELLOW

const int xul   = 8;                       // upper left corner of screen text
const int yul   = 0;                       //
const int ht    = 20;                      // row height
const int x1    = gScreenWidth / 2 - 10;   // = 320/2 = 160
const int x2    = x1 + 10;
const int x3    = gScreenWidth - 80;
const int x4    = gScreenWidth - 8;
const int row1  = yul + ht;
const int row2  = row1 + ht;
const int row3  = row2 + ht;
const int row4  = row3 + ht;
const int row5  = row4 + ht;
const int row6  = row5 + ht;
const int row7  = row6 + ht;
const int row8  = row7 + ht;
const int row9  = row8 + ht;
const int row10 = row9 + ht + 10;   // a little extra room for bottom two rows
const int row11 = row10 + ht;

TextField txtScreen[] = {
    // row 1
    {PROGRAM_NAME, xul, row1, cTEXTCOLOR},    // [0]
    {X_MAX_OHMS, x3, row1, cXGROUP},          // [1]
    {"max", x4, row1, cXGROUP, ALIGNRIGHT},   // [2]
    // row 4
    // row 5
    {"Current pressure:", xul, row5, cLABEL},   // [3]
    {"ppp", x2, row5, cVALUE},                  // [4] current pressure value
    {"xxx", x3, row5, cVALUE},                  // [5] current X value
    {"X   ", x4, row5, cXGROUP, ALIGNRIGHT},    // [6]
    // row 6
    {"Threshhold:", xul, row6, cLABEL},         // [7]
    {START_TOUCH_PRESSURE, x2, row6, cLABEL},   // [8]
    // row 7
    // row 8
    // row 9
    {X_MIN_OHMS, x3, row9, cXGROUP},          // [9]
    {"min", x4, row9, cXGROUP, ALIGNRIGHT},   // [10]
    // row 10
    {Y_MIN_OHMS, xul, row10, cYGROUP},              // [11]
    {"yyy", x1, row10, cVALUE},                     // [12] current Y value
    {Y_MAX_OHMS, x4, row10, cYGROUP, ALIGNRIGHT},   // [13]
    // row 11
    {"min", xul, row11, cYGROUP},              // [14]
    {"Y", x1, row11, cYGROUP},                 // [15]
    {"max", x4, row11, cYGROUP, ALIGNRIGHT},   // [16]
};
const int numScreenFields = sizeof(txtScreen) / sizeof(TextField);

// ========== main screen ====================================
void startScreen() {

  clearScreen();                                         // clear screen
  txtScreen[0].setBackground(cBACKGROUND);               // set background for all TextFields
  TextField::setTextDirty(txtScreen, numScreenFields);   // make sure all fields are updated

  setFontSize(eFONTSMALLEST);
  for (int ii = 0; ii < numScreenFields; ii++) {
    txtScreen[ii].print();
  }
}
void updateScreen(TSPoint tp) {
  txtScreen[5].print(tp.x);    // current X value
  txtScreen[12].print(tp.y);   // current Y value
  if (tp.z > 0) {
    txtScreen[4].print(tp.z);   // current pressure value
  }
}

void labelAxis() {
  const int xV = x3 - 4;      // screen X coord of vertical line
  const int yH = row9 + 10;   // screen Y coord of horizontal line

#define nDottedLines 5
  TwoPoints dottedLines[nDottedLines] = {
      {xul, yH, x4, yH, cYGROUP},          // horiz across bottom row
      {xV, yul, xV, row9 + 10, cXGROUP},   // vert along rhs
  };

  for (int ii = 0; ii < nDottedLines; ii++) {
    TwoPoints item = dottedLines[ii];
    if (item.x1 == item.x2) {
      // ----- vertical dotted line -----
      for (int yy = item.y1; yy < item.y2; yy++) {
        if (yy % 2) {
          tft.drawPixel(item.x1, yy, item.color);
        }
      }
    } else {
      // ----- horizontal dotted line -----
      for (int xx = item.x1; xx < item.x2; xx++) {
        if (xx % 2) {
          tft.drawPixel(xx, item.y1, cYGROUP);
        }
      }
    }
  }

  const int yA = row11 - 5;   // screen Y coord of left/rht arrow
  const int xA = x4 - 16;     // screen X coord of up/down arrow
#define nSolidLines 12
  TwoPoints solidLines[nSolidLines] = {
      {x1 + 30, yA, x1 + 50, yA, cYGROUP},       // right arrow
      {x1 + 42, yA - 3, x1 + 50, yA, cYGROUP},   // top arrowhead
      {x1 + 42, yA + 3, x1 + 50, yA, cYGROUP},   // bot arrowhead

      {x1 - 20, yA, x1 - 40, yA, cYGROUP},       // left arrow
      {x1 - 32, yA - 3, x1 - 40, yA, cYGROUP},   // top arrowhead
      {x1 - 32, yA + 3, x1 - 40, yA, cYGROUP},   // bot arrowhead

      {xA, row5 - 30, xA, row5 - 50, cXGROUP},   // up arrow
      {xA - 3, row5 - 40, xA, row5 - 50, cXGROUP},
      {xA + 3, row5 - 40, xA, row5 - 50, cXGROUP},

      {xA, row5 + 10, xA, row5 + 36, cXGROUP},   // down arrow
      {xA - 3, row5 + 24, xA, row5 + 36, cXGROUP},
      {xA + 3, row5 + 24, xA, row5 + 36, cXGROUP},
  };

  for (int ii = 0; ii < nSolidLines; ii++) {
    TwoPoints item = solidLines[ii];
    tft.drawLine(item.x1, item.y1, item.x2, item.y2, item.color);
  }
}

//=========== setup ============================================
void setup() {

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);   // start at full brightness

  // ----- init TFT display
  tft.begin();                     // initialize TFT display
  tft.setRotation(LANDSCAPE);      // 1=landscape (default is 0=portrait)
  tft.fillScreen(ILI9341_BLACK);   // note that "begin()" does not clear screen

  // ----- announce ourselves
  startSplashScreen();

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);   // init for debugging in the Arduino IDE

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_NAME " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init touch screen
  initTouchScreen();

  startScreen();
  labelAxis();
#ifdef SHOW_SCREEN_CENTERLINE
  // show centerline at    x1,y1             x2,row2             color
  tft.drawLine(tft.width() / 2, 0, tft.width() / 2, tft.height(), cTEXTFAINT);    // debug
  tft.drawLine(0, tft.height() / 2, tft.width(), tft.height() / 2, cTEXTFAINT);   // debug
#endif
}

//=========== main work loop ===================================

void loop() {

  // if there's touchscreen input, handle it
  TSPoint screenLoc;
  if (newScreenTap(&screenLoc, tft.getRotation())) {

    // debug: show where touched
    const int radius = 1;
    tft.fillCircle(screenLoc.x, screenLoc.y, radius, cTOUCHTARGET);
    updateScreen(screenLoc);
  }

  // small activity bar crawls along bottom edge to give
  // a sense of how frequently the main loop is executing
  showActivityBar(tft.height() - 1, ILI9341_RED, cBACKGROUND);
}
