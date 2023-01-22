// Please format this file with clang before check-in to GitHub
/*
  DAC Morse Code Generator -- refactored into DACMorseSender class

  Version history:
            2022-06-05 refactored pin definitions into hardware.h
            2019-12-30 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Combines these elements of producing Morse code.
            1. C++ object with simple programming interface
            2. Synthesized audio on DAC converter
            3. Digital potentiometer to control volume
            4. LM386 audio amplifier with small speaker
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include <DS1804.h>             // DS1804 digital potentiometer library
#include "hardware.h"           // Griduino pin definitions
#include "morse_dac.h"          // class DACMorseSender

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE    "DAC Morse v3"
#define PROGRAM_VERSION  "v1.12"
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// ---------- TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Digital potentiometer
// ctor         DS1804( ChipSel pin, Incr pin,  U/D pin,  maxResistance (K) )
DS1804 volume = DS1804(PIN_VCS, PIN_VINC, PIN_VUD, DS1804_TEN);
int gWiper    = 3;   // initial digital potentiometer wiper position, 0..99

// ------------ definitions
const int howLongToWait = 6;   // max number of seconds at startup waiting for Serial port to console
#define gScreenWidth    320    // pixels wide, landscape orientation
#define gScreenHeight   240    // pixels high
#define SCREEN_ROTATION 1      // 1=landscape, 3=landscape 180 degrees

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND 0x00A          // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cTEXTCOLOR  ILI9341_CYAN   // 0, 255, 255
#define cLABEL      ILI9341_GREEN
#define cVALUE      ILI9341_YELLOW
int gLoopCount = 0;

// ========== screen helpers ===================================

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong * 1000;
  while (millis() < targetTime) {
    if (Serial)
      break;
  }
}

// ========== splash screen ====================================
const int xLabel = 8;            // indent labels, slight margin on left edge of screen
const int yRow1  = 20;           // title
const int yRow2  = yRow1 + 40;   // program version
const int yRow3  = yRow2 + 20;   // compiled date
const int yRow4  = yRow3 + 40;   // author line 1
const int yRow5  = yRow4 + 20;   // author line 2

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

  delay(2000);
  clearScreen();

  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR);
  tft.print(PROGRAM_TITLE);
}

void showFreqSpeed(int frequency, int wpm) {
  tft.setCursor(xLabel, yRow4);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Speed ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(wpm);
  tft.print(" wpm ");
  Serial.print(wpm);
  Serial.println(" wpm");

  tft.setCursor(xLabel, yRow5);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Frequency ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(frequency);
  tft.print(" Hz ");
  Serial.print(frequency);
  Serial.println(" Hz");
}

void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  static int addDotX = 10;   // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count   = 0;
  const int SCALEF   = 128;   // how much to slow it down so it becomes visible

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

  // ----- init TFT display
  tft.begin();                     // initialize TFT display
  tft.setRotation(1);              // 1=landscape (default is 0=portrait)
  tft.fillScreen(ILI9341_BLACK);   // note that "begin()" does not clear screen

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);   // start at full brightness

  // ----- announce ourselves
  startSplashScreen();

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);           // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

  // ----- init digital potentiometer
  volume.unlock();                   // unlock digipot (in case someone else, like an example pgm, has locked it)
  volume.setToZero();                // set digipot hardware to match its ctor (wiper=0) because the chip cannot be read
                                     // and all "setWiper" commands are really incr/decr pulses. This gets it sync.
  volume.setWiperPosition(gWiper);   // set default volume in digital pot
  Serial.print("Set wiper position = ");
  Serial.println(gWiper);

  // ========== TEST 1 ==========
  //                       outpin, freq, wpm
  DACMorseSender dacMorse(DAC_PIN, 1000, 15);
  showFreqSpeed(1000, 15);
  dacMorse.setup();
  dacMorse.dump();   // debug

  // ----- test series of dits
  Serial.print("\nStarting dits\n");
  for (int ii = 1; ii <= 20; ii++) {
    Serial.print(ii);
    Serial.print(" ");
    if (ii % 10 == 0)
      Serial.print("\n");
    dacMorse.send_dit();
  }
  Serial.print("Finished dits\n");
  dacMorse.send_word_space();

  // ----- test dit-dah
  Serial.print("\nStarting dit-dah\n");
  for (int ii = 1; ii <= 10; ii++) {
    Serial.print(ii);
    Serial.print(" ");
    if (ii % 10 == 0)
      Serial.print("\n");
    dacMorse.send_dit();
    dacMorse.send_dah();
  }
  dacMorse.send_word_space();
  Serial.print("Finished dit-dah\n");

  // ----- test messages
  dacMorse.setMessage("de k7bwh es km7o");
  dacMorse.sendBlocking();
  dacMorse.setMessage("tu  ");   // thank you
  dacMorse.sendBlocking();
  dacMorse.setMessage("ese ee");   // shave and a haircut
  dacMorse.sendBlocking();
  dacMorse.setMessage("S");   // PROSIGN_SK
  dacMorse.sendBlocking();
  dacMorse.setMessage("A");   // PROSIGN_AS
  dacMorse.sendBlocking();

  // ========== TEST 2 ==========
  //                    outpin, freq, wpm
  DACMorseSender morse2(DAC_PIN, 800, 18);
  showFreqSpeed(800, 18);
  morse2.setup();
  morse2.setMessage("abcdefghijklmnopqrstuvwxyz");
  morse2.sendBlocking();
  morse2.setMessage("01234 56789");
  morse2.sendBlocking();
  morse2.setMessage("de k7bwh es km7o  A  ");
  morse2.sendBlocking();

  // ========== TEST 3 ==========
  DACMorseSender morse3(DAC_PIN, 1100, 20);
  showFreqSpeed(1100, 20);
  morse3.setup();
  morse3.setMessage("de k7bwh es km7o  A  ");
  morse3.sendBlocking();

  // ========== TEST 4 ==========
  Serial.println("1200 Hz");
  DACMorseSender morse4(DAC_PIN, 1200, 25);
  showFreqSpeed(1200, 25);
  morse4.setup();
  morse4.setMessage("de k7bwh es km7o  A  ");
  morse4.sendBlocking();

  // ========== TEST 5 ==========
  Serial.println("1300 Hz");
  DACMorseSender morse5(DAC_PIN, 1300, 30);
  showFreqSpeed(1300, 30);
  morse5.setup();
  morse5.setMessage("de k7bwh es km7o  S S");
  morse5.sendBlocking();
}

//=========== main work loop ===================================
void loop() {

  // small activity bar crawls along bottom edge to give
  // a sense of how frequently the main loop is executing
  showActivityBar(tft.height() - 1, ILI9341_RED, cBACKGROUND);
}
