/*
  DAC audio playback from stored program memory

  Date:     2020-03-14 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This sketch is a simple monaural program using only DAC0. 
            It plays a mono WAV file sampled at 8 kHz.
            Sample audio is stored in program memory.

  Preparing audio files:
            Must convert a WAV file to 8 kHz mono
            1. Open a web browser https://online-audio-converter.com
               Settings: 8000 Hz, 1 channel
               Convert and save an 8 kHz mono audio WAV file
            2. Install EncodeAudio application from http://highlowtech.org/?p=1963
               Process the 8 kHz file (above)
               Paste from the clipboard into an array of bytes

  Mono Audio: The DAC on the SAMD51 is a 12-bit output, from 0 - 3.3v.
            The largest 12-bit number is  4,096:
            * Writing 0 will set the DAC to minimum (0.0 v) output.
            * Writing  4096 sets the DAC to maximum (3.3 v) output.

            This example program has no user interface controls or inputs.
*/

#include "Adafruit_ILI9341.h"       // TFT color display library
#include "DS1804.h"                 // DS1804 digital potentiometer library
#include "elapsedMillis.h"          // short-interval timing functions
#include "sample1.h"                // audio clip 1
#include "sample2.h"                // audio clip 2

// ------- Identity for console
#define PROGRAM_TITLE   "DAC Audio in Program Mem"
#define PROGRAM_VERSION "v1.0"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ---------- Hardware Wiring ----------
/* Same as Griduino platform
*/

  #define TFT_BL   4    // TFT backlight
  #define TFT_CS   5    // TFT chip select pin
  #define TFT_DC  12    // TFT display/command pin

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ------------ Audio output
#define DAC_PIN      DAC0     // onboard DAC0 == pin A0
#define PIN_SPEAKER  DAC0     // uses DAC

// Adafruit Feather M4 Express pin definitions
#define PIN_VCS      A1       // volume chip select
#define PIN_VINC      6       // volume increment
#define PIN_VUD      A2       // volume up/down

// ctor         DS1804( ChipSel pin, Incr pin,  U/D pin,  maxResistance (K) )
DS1804 volume = DS1804( PIN_VCS,     PIN_VINC,  PIN_VUD,  DS1804_TEN );
int gVolume = 85;             // initial digital potentiometer wiper position, 0..99

// ------------ typedef's
struct Point {
  int x, y;
};

// ----- screen layout
// screen pixel coordinates for top left of character cell

// ----- color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND     0x00A             // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cSCALECOLOR     0xF844
#define cTEXTCOLOR      ILI9341_CYAN      // 0, 255, 255
#define cLABEL          ILI9341_GREEN
#define cVALUE          ILI9341_YELLOW
#define cINPUT          ILI9341_WHITE
#define cBUTTONFILL     ILI9341_NAVY
#define cBUTTONOUTLINE  ILI9341_CYAN
#define cBUTTONLABEL    ILI9341_YELLOW
#define cWARN           0xF844            // brighter than ILI9341_RED but not pink

// ------------ global scope
const int howLongToWait = 10; // max number of seconds before using Serial port to console
int gLoopCount = 0;

const int gSampleRate = 8000;               // 8 kHz audio file
const int gHoldTime = 1E6 / gSampleRate;    // microseconds to hold each output sample

// ========== splash screen helpers ============================
// splash screen layout
const int xLabel = 8;             // indent labels, slight margin to left edge of screen
#define yRow1   20                // program title: "DAC Audio"
#define yRow2   yRow1 + 20        // program version
#define yRow3   yRow2 + 20        // author line 1
#define yRow4   yRow3 + 20        // author line 2

void startSplashScreen() {
  tft.setTextSize(2);

  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR);
  tft.print(PROGRAM_TITLE);

  tft.setCursor(xLabel, yRow2);
  tft.setTextColor(cLABEL);
  tft.print(PROGRAM_VERSION);
  
  tft.setCursor(xLabel, yRow3);
  tft.println(PROGRAM_LINE1);

  tft.setCursor(xLabel, yRow4);
  tft.println(PROGRAM_LINE2);
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

//=========== setup ============================================
void setup() {

  // ----- init serial monitor
  Serial.begin(115200);                               // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);                       // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(1);                 // 1=landscape (default is 0=portrait)
  clearScreen();

  // ----- announce ourselves
  startSplashScreen();

  // ----- init digital potentiometer
  volume.setWiperPosition(gVolume);
  Serial.print("Set wiper position = "); Serial.println(gVolume);

  // ----- init onboard DAC
  #if defined(SAMD_SERIES)
    // Only set DAC resolution on devices that have a DAC
    analogWriteResolution(12);        // 1..32, sets DAC output resolution to 12 bit (4096 levels)
                                      // because Feather M4 maximum output resolution is 12 bit
  #endif
  delay(100);                         // settling time

  // ----- debug: echo our initial settings to the console
  char msg[256], ff[32], gg[32];
  byte wiperPos = volume.getWiperPosition();
  sprintf(msg, "Initial wiper position(%d)", wiperPos);
  Serial.println(msg);
}

void playAudio(const unsigned char* audio, int audiosize) {
  for (int ii=0; ii<audiosize; ii++) {
    int value = audio[ii] << 4;     // max sample is 2^8, max DAC output 2^12, so shift left by 4
    analogWrite(DAC0, value);
    delayMicroseconds(gHoldTime);   // hold the sample value for the sample time
  }
}

//=========== main work loop ===================================
const int AUDIO_CLIP_INTERVAL = 1500;    // msec between one audio clip and the next

void loop() {

  // ----- play audio clip 1 several times
  for (int ii=0; ii<8; ii++) {
    Serial.print("Starting playback "); Serial.print(gLoopCount); Serial.print(","); Serial.println(ii);
    playAudio(sample1, sizeof(sample1));
    delay(AUDIO_CLIP_INTERVAL);       // insert pause between clips
  }

  // ----- play audio clip 2 once
  Serial.println("Starting playback of different audio sample");
  playAudio(sample2, sizeof(sample2));
  delay(AUDIO_CLIP_INTERVAL);

  gLoopCount++;
}
