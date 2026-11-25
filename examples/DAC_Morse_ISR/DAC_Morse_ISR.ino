// Please format this file with clang before check-in to GitHub
/*
  DAC Morse ISR -- non-blocking Morse code from a timer interrupt

  Version history:
            2026-11-25 added non-blocking Morse code: startSending(), isBusy(), stop()
                       driven by a 16 kHz TC4 timer interrupt
            2022-06-05 refactored pin definitions into hardware.h
            2019-12-30 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Combines these elements of producing Morse code.
            1. C++ object with simple programming interface
            2. Synthesized audio on DAC converter
            3. Digital potentiometer to control volume
            4. LM386 audio amplifier with small speaker

  What to watch:
            The tests run once from loop(), alternating blocking and non-blocking sends.
            The red activity bar along the bottom edge only moves while loop() is running.
            * Blocking:     bar freezes until the message is finished.
            * Non-blocking: bar keeps crawling and the loop counter keeps climbing.
            The serial console reports how many times loop() ran during each non-blocking message.
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include <DS1804.h>             // digital pot, Tom Dudman: https://github.com/tom-dudman/DS1804
#include "hardware.h"           // Griduino pin definitions
#include "morse_dac.h"          // class DACMorseSender

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE    "DAC Morse ISR"
#define PROGRAM_VERSION  "v1.18"
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
unsigned long gLoopCount = 0;   // loop() passes since the current test started

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
const int xLabel = 8;   // indent labels, slight margin on left edge of screen
//                                  splash                tests
const int yRow1 = 20;           // title                 title
const int yRow2 = yRow1 + 40;   // program version       test number "Test 1 of 17"
const int yRow3 = yRow2 + 22;   // compiled date         mode "Blocking" or "Non-blocking"
const int yRow4 = yRow3 + 22;   // author line 1         morse string, in quotes so leading/trailing spaces are visible
const int yRow5 = yRow4 + 22;   // author line 2         speed "18 wpm"
const int yRow6 = yRow5 + 22;   // loop counter          frequency "700 Hz"
const int yRow7 = yRow6 + 22;   // unused                loop() count, to prove non-blocking operation

void startSplashScreen() {
  tft.setTextSize(2);

  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print(PROGRAM_TITLE);

  tft.setCursor(xLabel, yRow2);
  tft.setTextColor(cLABEL);
  tft.print(PROGRAM_VERSION);

  tft.setCursor(xLabel, yRow3);
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
  tft.setCursor(xLabel, yRow5);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Speed ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(wpm);
  tft.print(" wpm ");
  Serial.print(wpm);
  Serial.println(" wpm");

  tft.setCursor(xLabel, yRow6);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Frequency ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(frequency);
  tft.print(" Hz ");
  Serial.print(frequency);
  Serial.println(" Hz");
}

void showMode(const char *mode) {
  tft.setCursor(xLabel, yRow3);
  tft.setTextColor(cLABEL, cBACKGROUND);
  tft.print(mode);
  tft.print("            ");   // erase leftover text from a longer label
}

void showTest(int number, int total, const char *message) {
  // ----- test number, e.g. "Test 3 of 16"
  tft.setTextSize(2);
  tft.setCursor(xLabel, yRow2);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Test ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(number);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print(" of ");
  tft.print(total);
  tft.print("   ");

  // ----- test string, in quotes so leading/trailing spaces are visible
  tft.fillRect(0, yRow4, tft.width(), 16, cBACKGROUND);       // erase previous string
  int chars = strlen(message) + 2;                            // +2 for quotes
  int size  = (xLabel + chars * 12 <= tft.width()) ? 2 : 1;   // 12 or 6 pixels per char
  tft.setTextWrap(false);
  tft.setTextSize(size);
  tft.setCursor(xLabel, yRow4 + (size == 1 ? 4 : 0));
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print('"');
  tft.print(message);
  tft.print('"');
  tft.setTextSize(2);
  tft.setTextWrap(true);
}

void showLoopCount(unsigned long count) {
  tft.setCursor(xLabel, yRow7);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("loop() count ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(count);
  tft.print("       ");
}

void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  static int addDotX = 10;   // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count   = 0;
  const int SCALEF   = 4096;   // how much to slow it down so it becomes visible

  count = (count + 1) % SCALEF;
  if (count == 0) {
    addDotX = (addDotX + 1) % tft.width();     // advance
    rmvDotX = (rmvDotX + 1) % tft.width();     // advance
    tft.drawPixel(addDotX, row, foreground);   // write new
    tft.drawPixel(rmvDotX, row, background);   // erase old
  }
}

//=========== tests ============================================
// Each test runs from loop(), one after another, with a short pause between them.
enum TestMode {
  UNIT_TEST,     // blocking dits and dit-dahs, using the deprecated unit test functions
  BLOCKING,      // sendBlocking(): loop() is stalled until the message is finished
  NONBLOCKING,   // startSending(): returns immediately, loop() keeps running
  STOP_EARLY,    // startSending(), then stop() after a few seconds
};

struct MorseTest {
  TestMode mode;
  int frequency;   // Hz
  float wpm;       // words per minute
  const char *message;
};

// clang-format off
const MorseTest tests[] = {
    // mode,     Hz,  wpm, message
    {UNIT_TEST,   1000, 15, ""},
    {BLOCKING,    1000, 15, "de k7bwh es km7o"},
    {NONBLOCKING, 1000, 15, "de k7bwh es km7o"},   // same message, compare blocking vs non-blocking
    {NONBLOCKING,  800, 18, "abcdefghijklmnopqrstuvwxyz"},
    {BLOCKING,     800, 18, "abcdefghijklmnopqrstuvwxyz"}, // same message, compare blocking vs non-blocking
    {BLOCKING,     800, 18, "01234 56789"},
    {NONBLOCKING,  800, 18, "de k7bwh es km7o  A  "},
    {BLOCKING,    1100, 20, "de k7bwh es km7o  A  "},
    {NONBLOCKING, 1100, 20, "de k7bwh es km7o  A  "},
    {NONBLOCKING, 1200, 25, "de k7bwh es km7o  A  "},
    {STOP_EARLY,  1200, 25, "this message is stopped early"},
    {BLOCKING,    1300, 30, "de k7bwh es km7o  S S"},
    {NONBLOCKING, 1300, 30, "de k7bwh es km7o  S S"},
    {NONBLOCKING, 1000, 15, "ese ee"},             // shave and a haircut
    {BLOCKING,    1000, 15, "tu  "},               // thank you
    {BLOCKING,    1000, 15, "S"},                  // PROSIGN_SK
    {NONBLOCKING, 1000, 15, "A"},                  // PROSIGN_AS
};
const int numTests = sizeof(tests) / sizeof(tests[0]);
// clang-format on

const unsigned long PAUSE_BETWEEN_TESTS = 1000;   // msec
const unsigned long STOP_EARLY_AFTER    = 3000;   // msec

int testIndex                = 0;       // next test to run
bool waitingForSend          = false;   // true while a non-blocking test is sending
unsigned long testStartTime  = 0;       // millis() when the current test started
unsigned long pauseStartTime = 0;       // millis() when the previous test finished
bool stopEarly               = false;   // true if the current test calls stop()
bool allDone                 = false;

void runUnitTests(DACMorseSender &dacMorse) {
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
}

void runTest(int index) {
  const MorseTest &test = tests[index];

  Serial.print("\n----- Test ");
  Serial.print(index + 1);
  Serial.print(" of ");
  Serial.println(numTests);
  showTest(index + 1, numTests, test.mode == UNIT_TEST ? "dits and dit-dahs" : test.message);

  // The sender object can be local: startSending() copies everything the ISR
  // needs, so a non-blocking message keeps playing after this function returns.
  //                       outpin,   freq,           wpm
  DACMorseSender dacMorse(DAC_PIN, test.frequency, test.wpm);
  dacMorse.setup();
  showFreqSpeed(test.frequency, (int)test.wpm);
  dacMorse.setMessage(test.message);

  gLoopCount    = 0;
  testStartTime = millis();

  switch (test.mode) {
  case UNIT_TEST:
    showMode("Blocking dits");
    runUnitTests(dacMorse);
    showLoopCount(gLoopCount);   // still 0: loop() could not run
    pauseStartTime = millis();
    break;

  case BLOCKING:
    showMode("Blocking");
    dacMorse.sendBlocking();     // loop() and the activity bar stop until this returns
    showLoopCount(gLoopCount);   // still 0: loop() could not run
    pauseStartTime = millis();
    break;

  case NONBLOCKING:
  case STOP_EARLY:
    showMode(test.mode == NONBLOCKING ? "Non-blocking" : "Non-blocking, stop");
    dacMorse.startSending();   // returns immediately
    waitingForSend = true;
    stopEarly      = (test.mode == STOP_EARLY);
    break;
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

  // ----- init DAC
  analogWriteResolution(12);   // Feather M4 DAC is 12 bits (4096 levels)

  // all Morse code tests run from loop()
}

//=========== main work loop ===================================
void loop() {
  gLoopCount++;

  // small activity bar crawls along bottom edge to give
  // a sense of how frequently the main loop is executing
  showActivityBar(tft.height() - 1, ILI9341_RED, cBACKGROUND);

  // show the main loop is running: during non-blocking sends, between tests,
  // and after all tests are done (it freezes only during blocking sends)
  static unsigned long lastShown = 0;
  if (millis() - lastShown >= 250) {
    lastShown = millis();
    showLoopCount(gLoopCount);
  }

  // ----- a non-blocking test is in progress
  if (waitingForSend) {
    if (stopEarly && millis() - testStartTime >= STOP_EARLY_AFTER) {
      Serial.println("Calling stop()");
      DACMorseSender::stop();
      stopEarly = false;
    }

    if (DACMorseSender::isBusy()) {
      return;   // keep looping until the message is finished
    }

    // finished
    waitingForSend = false;
    showLoopCount(gLoopCount);
    Serial.print("Finished non-blocking send in ");
    Serial.print(millis() - testStartTime);
    Serial.print(" msec, loop() ran ");
    Serial.print(gLoopCount);
    Serial.println(" times meanwhile");
    pauseStartTime = millis();
  }

  // ----- short pause between tests (without blocking)
  if (millis() - pauseStartTime < PAUSE_BETWEEN_TESTS) {
    return;
  }

  // ----- start the next test
  if (testIndex < numTests) {
    runTest(testIndex);
    testIndex++;
  } else if (!allDone) {
    allDone = true;
    showMode("Done. Reset to run again.");
    Serial.println("\nAll tests done. Press reset to run them again.");
  }
}
