/*
  DAC Timer v4 -- basic audio tone using interrupt level to control DAC

  Change log:
            2020-12-24 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Generate an audio sine wave in the speaker from an ISR.
            Let's keep this simple and make an 800-Hz sine wave.
            This is one more step along the way to playing WAV files.

  Guess'n check table size:
            * 800 Hz = each complete cycle takes 1/800 = 1,250 usec
            * 20 steps/wave = 1250/20 = 63 usec per interrupt = 16 kHz

            But I want an 8 kHz interrupt rate (not 16 kHz) to match the 
            sample rate of WAV files. So:
            * 800 Hz @ 10 steps/wave = 1/800/10 = 125 usec per interrupt
            * Resulting in a lookup table for the waveform of 10 steps/wave

            Note the lookup table size varies for the desired audio frequency.
            If you want to generate, say, 1100 Hz audio tone:
            * 1100 Hz = each complete cycle takes 1/1100 = 909 usec
            * For 1100 Hz: table size = (8000 Hz)/(1100 Hz) =  7.27 entries
            * For 1000 Hz: table size = (8000 Hz)/(1000 Hz) =  8.00 entries
            * For  800 Hz: table sixe = (8000 Hz)/( 800 Hz) = 10.00 entries 
            * For  700 Hz: table size = (8000 Hz)/( 700 Hz) = 11.43 entries

  Note:     Does not control the backlight.
            Watch the crawler on the bottom row - it should move smoothly.

  Conclusion: 
            This sketch produces steady 800 Hz tone, as desired.
            But that's the only thing it can do. It is lacking a proper 
            interface for using different tones.

  Based on: Dennis-van-Gils/SAMD51_InterruptTimer 
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>         // TFT color display library
#include <DS1804.h>                   // DS1804 digital potentiometer library
#include "SAMD51_InterruptTimer.h"    // https://github.com/Dennis-van-Gils/SAMD51_InterruptTimer

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "DAC Timer v4"
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

// ------------ Audio output
#define DAC_PIN      DAC0             // onboard DAC0 == pin A0
#define PIN_SPEAKER  DAC0             // uses DAC

// Adafruit Feather M4 Express pin definitions
#define PIN_VCS      A1               // volume chip select
#define PIN_VINC      6               // volume increment
#define PIN_VUD      A2               // volume up/down

// ctor         DS1804( ChipSel pin, Incr pin,  U/D pin,  maxResistance (K) )
DS1804 volume = DS1804( PIN_VCS,     PIN_VINC,  PIN_VUD,  DS1804_TEN );
int gWiper = 15;                      // initial digital potentiometer wiper position, 0..99

// ---------- Morse Code ----------
const unsigned int isrFrequency = 8000;  // 8 kHz interrupt rate to play WAV files
const unsigned int isrTime = 125;     // time between interrupts, 125 = 1/8000 usec
int gFrequency = 800;                 // initial Morse code sidetone pitch
int gWordsPerMinute = 15;             // initial Morse code sending speed

#include "morse_isr.h"                // Morse Code using digital-audio converter DAC0
DACMorseSenderISR dacMorse(DAC_PIN, isrTime, gFrequency, gWordsPerMinute);

// "morse_isr.cpp" replaced "morse_dac.cpp" which replaced  "morse.h"
//#include "morse.h"                  // Morse Code Library for Arduino with Non-Blocking Sending
//                                    // https://github.com/markfickett/arduinomorse

// ------------ definitions
const int howLongToWait = 4;          // max number of seconds at startup waiting for Serial port to console
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

void startSplashScreen() {
  tft.setTextSize(2);
  tft.setCursor(xLabel, 20); tft.setTextColor(cTEXTCOLOR, cBACKGROUND); tft.print(PROGRAM_TITLE);
  tft.setCursor(xLabel, 40); tft.setTextColor(cLABEL, cBACKGROUND);     tft.print(PROGRAM_VERSION);
  tft.setCursor(xLabel, tft.height() - 40);                             tft.println("Compiled");
  tft.setCursor(xLabel, tft.height() - 20);                             tft.println(PROGRAM_COMPILED);
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
  static int addDotX = 10;            // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count = 0;
  const int SCALEF = 8192;            // how much to slow it down so it becomes visible

  count = (count + 1) % SCALEF;
  if (count == 0) {
    addDotX = (addDotX + 1) % tft.width();    // advance
    rmvDotX = (rmvDotX + 1) % tft.width();    // advance
    tft.drawPixel(addDotX, row, foreground);  // write new
    tft.drawPixel(rmvDotX, row, background);  // erase old
  }
}

// ========== interrupt service routine ========================
volatile int wIndex = 0;              // index into waveform table

extern unsigned int waveform[sizeWavetable]; // waveform lookup table is computed once and saved

void myISR() {

  unsigned int sound = waveform[wIndex];  // look up sound level

  //if (wIndex < sizeWavetable/2) {
  //  sound = 4000;    // max peak of wave - debug
  //} else {
  //  sound = 100;   // max valley of wave - debug
  //}
  
  analogWrite(PIN_SPEAKER, sound);    // output to speaker

  wIndex++;                           // increment counter
  if (wIndex >= sizeWavetable) {
    wIndex = 0;
  }
}

//=========== setup ============================================
void setup() {

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(1);                 // 1=landscape (default is 0=portrait)
  clearScreen();                      // note that "begin()" does not clear screen 

  // ----- announce ourselves
  startSplashScreen();

  // ----- init TFT backlight
  //pinMode(TFT_BL, OUTPUT);          // <------ THIS CAUSES PROGRAM CRASH 
  //analogWrite(TFT_BL, 255);         // <------ THIS CAUSES PROGRAM CRASH 
                                      // Symptom: screen is dark, LED stops flashing, USB port non-responsive
                                      // To recover, double-click Feather's "Reset" button and load another pgm

  // ----- init serial monitor
  Serial.begin(115200);               // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);       // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init digital potentiometer
  volume.unlock();                    // unlock digipot (in case someone else, like an example pgm, has locked it)
  volume.setToZero();                 // set digipot hardware to match its ctor (wiper=0) because the chip cannot be read
                                      // and all "setWiper" commands are really incr/decr pulses. This gets it sync.
  volume.setWiperPosition(20);        // set default volume in digital pot, '20' is reasonable

  // ----- init DAC for audio/morse code
  #if defined(SAMD_SERIES)
    // Only set DAC resolution on devices that have a DAC
    analogWriteResolution(12);        // 1..32, sets DAC output resolution to 12 bit (4096 levels)
                                      // because Feather M4 maximum output resolution is 12 bit
  #endif
  dacMorse.setup();                   // required Morse Code initialization
  dacMorse.dump();                    // debug

  // ----- announce timer interrupt
  Serial.println("CPU Frequency = " + String(F_CPU / 1000000) + " MHz");

  //TC.startTimer(250000, myISR);     // 250,000 usec
  TC.startTimer(isrTime, myISR);      // 1/8000 = 125 usec
}

//=========== main work loop ===================================
void loop() {

  // small activity bar crawls along bottom edge to give 
  // a sense of how frequently the main loop is executing
  showActivityBar(tft.height()-1, ILI9341_RED, cBACKGROUND);
}
