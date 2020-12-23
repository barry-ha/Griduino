/*
  DAC Timer v1 -- basic toggle LED from interrupt level

  Change log:
            2020-12-20 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Blink the red LED from an interrupt level 
            Remove Griduino's front cover to see the red LED next to the USB connector
            This is the first minimal stripped-down example on my way to an 8 kHz ISR 
            to playback WAV files. Here, the ISR toggles the LED at about 4 Hz.

  Conclusion: 
            The LED + TFT works okay but we can't control backlight.
            There is a conflict between ISR and PWM output.
            The program crashes *if* we un-comment "analogWrite(TFT_BL, 255)" around line 155.
            This is a PWM pin for the TFT backlight.
            Apparently the PWM timer conflicts with the Interrupt clock timer.

  Steps to reproduce:
            1. Comment out "analogWrite()", run program.  Result: successful blinking of red LED
            2. Un-comment "analogWrite()", run program.   Result: program crash

  Based on: Dennis-van-Gils/SAMD51_InterruptTimer 
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>         // TFT color display library
#include "SAMD51_InterruptTimer.h"    // https://github.com/Dennis-van-Gils/SAMD51_InterruptTimer

// Here's another possible library as a replacement
//nclude "SAMD_TimerInterrupt.h"      // https://github.com/khoih-prog/SAMD_TimerInterrupt

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "DAC Timer v1"
#define PROGRAM_VERSION "v0.30"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ---------- Griduino TFT Display
  #define TFT_BL   4                  // TFT backlight
  #define TFT_CS   5                  // TFT chip select pin
  #define TFT_DC  12                  // TFT display/command pin

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ------------ definitions
const int howLongToWait = 4;          // max number of seconds at startup waiting for Serial port to console
#define gScreenWidth 320              // pixels wide, landscape orientation
#define gScreenHeight 240             // pixels high
#define SCREEN_ROTATION 1             // 1=landscape, 3=landscape 180 degrees

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND     0x00A           // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cTEXTCOLOR      ILI9341_CYAN    // 0, 255, 255
#define cLABEL          ILI9341_GREEN
#define cVALUE          ILI9341_YELLOW  // 255, 255, 0

// ------------ global scope
int gLoopCount = 0;

// ========== splash screen ====================================
const int xLabel = 8;                 // indent labels, slight margin on left edge of screen
const int yRow1 = 20;                 // title
const int yRow2 = yRow1 + 40;         // program version
const int yRow3 = yRow2 + 20;         // compiled date
const int yRow4 = yRow3 + 40;         // 
const int yRow5 = yRow4 + 20;         // 

void startSplashScreen() {
  tft.setTextSize(2);

  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print(PROGRAM_TITLE);

  tft.setCursor(xLabel, yRow2);
  tft.setTextColor(cLABEL);
  tft.print(PROGRAM_VERSION);
  
  tft.setCursor(xLabel, yRow2 + 140);
  tft.println("Compiled");

  tft.setCursor(xLabel, yRow2 + 160);
  tft.println(PROGRAM_COMPILED);

  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR);
  tft.print(PROGRAM_TITLE);
}

// ========== screen helpers ===================================
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

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  
  static int addDotX = 10;                    // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count = 0;
  const int SCALEF = 1024;                    // how much to slow it down so it becomes visible

  count = (count + 1) % SCALEF;
  if (count == 0) {
    addDotX = (addDotX + 1) % tft.width();    // advance
    rmvDotX = (rmvDotX + 1) % tft.width();    // advance
    tft.drawPixel(addDotX, row, foreground);  // write new
    tft.drawPixel(rmvDotX, row, background);  // erase old
  }
}

// ========== interrupt service routine ========================
volatile bool toggle = true;
volatile bool updated = false;
volatile uint32_t dT = 0;

void myISR() {
  uint32_t now = micros();
  static uint32_t prev_tick = now;
  
  dT = now - prev_tick;
  prev_tick = now;
  updated = true;

  digitalWrite(PIN_LED, toggle);
  toggle = not(toggle);
}

//=========== setup ============================================
void setup() {

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(1);                 // 1=landscape (default is 0=portrait)
  clearScreen();                      // note that "begin()" does not clear screen 

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);            
  //analogWrite(TFT_BL, 255);           // <------ THIS CAUSES PROGRAM CRASH 
                                      // Symptom: screen is dark, LED stops flashing, USB port non-responsive
                                      // To recover, double-click Feather's "Reset" button and load another pgm

  // ----- announce ourselves
  startSplashScreen();

  // ----- init serial monitor
  Serial.begin(115200);               // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);       // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  pinMode(PIN_LED, OUTPUT);

  TC.startTimer(250000, myISR); // 250,000 usec
}

//=========== main work loop ===================================
void loop() {
  
  if (updated) {
    Serial.println(dT);
    
    noInterrupts();
    updated = false;
    interrupts();
  }

  // small activity bar crawls along bottom edge to give 
  // a sense of how frequently the main loop is executing
  //showActivityBar(tft.height()-1, ILI9341_RED, cBACKGROUND);
}
