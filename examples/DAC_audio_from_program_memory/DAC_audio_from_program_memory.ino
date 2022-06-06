// Please format this file with clang before check-in to GitHub
/*
  DAC audio playback from stored program memory

  Version history:
            2022-06-05 refactored pin definitions into hardware.h
            2020-03-14 created

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
            The largest 12-bit number is 4,096:
            * Writing 0 will set the DAC to minimum (0.0 v) output.
            * Writing 4096 sets the DAC to maximum (3.3 v) output.

            This example program has no user inputs.
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include <DS1804.h>             // DS1804 digital potentiometer library
#include "elapsedMillis.h"      // short-interval timing functions
#include "hardware.h"           // Griduino pin definitions
#include "sample1.h"            // audio clip 1
#include "sample2.h"            // audio clip 2

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE    "DAC Audio in Program Mem"
#define PROGRAM_VERSION  "v1.08"
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// ---------- TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ------------ Audio output
// ctor         DS1804( ChipSel pin, Incr pin,  U/D pin,  maxResistance (K) )
DS1804 volume = DS1804(PIN_VCS, PIN_VINC, PIN_VUD, DS1804_TEN);
int gVolume   = 10;   // initial digital potentiometer wiper position, 0..99
                      // note that speaker distortion begins at wiper=40 when powered by USB

// ------------ typedef's
struct Point {
  int x, y;
};

// ----- Griduino color scheme
// RGB 565 true color: https://chrishewett.com/blog/true-rgb565-colour-picker/
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND    0x00A          // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cSCALECOLOR    0xF844         //
#define cTEXTCOLOR     ILI9341_CYAN   // 0, 255, 255
#define cLABEL         ILI9341_GREEN
#define cVALUE         ILI9341_YELLOW   // 255, 255, 0
#define cINPUT         ILI9341_WHITE
#define cBUTTONFILL    ILI9341_NAVY
#define cBUTTONOUTLINE ILI9341_CYAN
#define cBUTTONLABEL   ILI9341_YELLOW
#define cWARN          0xF844        // brighter than ILI9341_RED but not pink
#define cTOUCHTARGET   ILI9341_RED   // outline touch-sensitive areas

// ------------ global scope
const int howLongToWait = 5;   // max number of seconds at startup waiting for Serial port to console
int gLoopCount          = 0;

const int gSampleRate = 8000;                // 8 kHz audio file
const int gHoldTime   = 1E6 / gSampleRate;   // microseconds to hold each output sample

// ========== splash screen ====================================
const int xLabel = 8;            // indent labels, slight margin on left edge of screen
const int yRow1  = 20;           // title
const int yRow2  = yRow1 + 20;   // program version
const int yRow3  = yRow2 + 20;   // compiled date
const int yRow4  = yRow3 + 40;   //
const int yRow5  = yRow4 + 40;   // volume wiper setting
const int yRow6  = yRow5 + 40;   // loop counter

void startSplashScreen() {
  tft.setTextSize(2);

  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print(PROGRAM_TITLE);

  tft.setCursor(xLabel, yRow2);
  tft.setTextColor(cLABEL);
  tft.print(PROGRAM_VERSION);

  tft.setCursor(xLabel, yRow3);
  tft.println(PROGRAM_COMPILED);
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
  unsigned long targetTime = millis() + howLong * 1000;
  while (millis() < targetTime) {
    if (Serial)
      break;
  }
}

//=========== setup ============================================
void setup() {

  // ----- init TFT display
  tft.begin();          // initialize TFT display
  tft.setRotation(1);   // 1=landscape (default is 0=portrait)
  clearScreen();        // note that "begin()" does not clear screen

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);   // start at full brightness

  // ----- announce ourselves
  startSplashScreen();

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);           // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

  // ----- init digital potentiometer
  volume.unlock();      // unlock digipot (in case someone else, like an example pgm, has locked it)
  volume.setToZero();   // set digipot hardware to match its ctor (wiper=0) because the chip cannot be read
                        // and all "setWiper" commands are really incr/decr pulses. This gets it sync.
  volume.setWiperPosition(gVolume);
  Serial.print("Volume wiper = ");
  Serial.println(gVolume);

// ----- init onboard DAC
#if defined(SAMD_SERIES)
  // Only set DAC resolution on devices that have a DAC
  analogWriteResolution(12);   // 1..32, sets DAC output resolution to 12 bit (4096 levels)
                               // because Feather M4 maximum output resolution is 12 bit
#endif
  delay(100);   // settling time

  // ----- debug: echo our initial settings to the console
  char msg[256], ff[32], gg[32];
  byte wiperPos = volume.getWiperPosition();
  sprintf(msg, "Initial wiper position(%d)", wiperPos);
  Serial.println(msg);
}

void playAudio(const unsigned char *audio, int audiosize) {
  for (int ii = 0; ii < audiosize; ii++) {
    int value = audio[ii] << 4;   // max sample is 2^8, max DAC output 2^12, so shift left by 4
    analogWrite(DAC0, value);
    delayMicroseconds(gHoldTime);   // hold the sample value for the sample time
  }
}

int volSequence[] = {
    // lookup table of wiper positions
    // Experimental table using 1.5 dB steps
    // Ratio(1.5 dB) = 10^(1.5 / 10) = 1.412538
    // which yields 14 (!) volume levels from zero to maximum
    /* ***
    0,    // [ 0] mute, lowest allowed wiper position
    1,    // [ 1] lowest possible position with non-zero output
    2,    // [ 2] next lowest possible
    3,    // [ 3]  2.00 * 1.4125 =  2.83
  *** */
    4,    // [ 4]  2.83 * 1.4125 =  3.99
    6,    // [ 5]  3.99 * 1.4125 =  5.64
    8,    // [ 6]  5.64 * 1.4125 =  7.96
    11,   // [ 7]  7.96 * 1.4125 = 11.25
    16,   // [ 8] 11.25 * 1.4125 = 15.88
    22,   // [ 9] 15.88 * 1.4125 = 22.44
    32,   // [10] 22.44 * 1.4125 = 31.69
    45,   // [11] 31.69 * 1.4125 = 44.77 (distortion begins at wiper position 41 on USB power)
    65,   // [12] 44.77 * 1.4125 = 63.24
    89,   // [13] 63.24 * 1.4125 = 89.33 (sounds clean on 13.8v power)
};
int numVols = sizeof(volSequence) / sizeof(int);
/*
 * For sake of comparison to Griduino.ino:
   #define numLevels 11
   const int volLevel[numLevels] = {
      // Digital potentiometer settings,
      // about  2  dB steps = ratio 1.585
      // about 1.5 dB steps = ratio 1.41253
      0,    // [0] mute, lowest allowed wiper position
      1,    // [1] lowest possible position with non-zero output
      2,    // [2] next lowest poss
      4,    // [3]  2.000 * 1.585 =  4.755
      7,    // [4]  4.755 * 1.585 =  7.513
      12,   // [5]  7.513 * 1.585 = 11.908
      19,   // [6] 11.908 * 1.585 = 18.874
      29,   // [7] 18.874 * 1.585 = 29.916
      47,   // [8] 29.916 * 1.585 = 47.417
      75,   // [9] 47.417 * 1.585 = 75.155
    };
 */

void showWiperPosition(int row, int wiper) {
  tft.setCursor(xLabel, row);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Volume wiper ");

  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(wiper);
  tft.print(" ");
}
void showLoopCount(int row, int loopCount, int ii) {
  tft.setCursor(xLabel, row);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Starting playback ");

  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(loopCount);
  tft.print(",");
  tft.print(ii);
  tft.print(" ");
}
void showSampleSize(int row, int len) {
  tft.setCursor(xLabel, row);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("WAV buffer size ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(len);
  tft.print(" ");
}

//=========== main work loop ===================================
const int AUDIO_CLIP_INTERVAL = 2000;   // msec between one audio clip and the next

void loop() {

  // ----- play audio clip 1 several times
  for (int ii = 0; ii < numVols; ii++) {
    Serial.print("Starting playback ");
    Serial.print(gLoopCount);
    Serial.print(",");
    Serial.println(ii);

    gVolume = volSequence[ii % numVols];   // get next volume
    volume.setWiperPosition(gVolume);      // set volume

    showWiperPosition(yRow4, gVolume);
    showLoopCount(yRow5, gLoopCount, ii);
    showSampleSize(yRow6, sizeof(sample1));

    playAudio(sample1, sizeof(sample1));   // play sample
    delay(AUDIO_CLIP_INTERVAL);            // insert pause between clips
  }

  // ----- play audio clip 2 once
  Serial.println("Starting playback of different audio sample");
  showWiperPosition(yRow4, 22);
  showLoopCount(yRow5, gLoopCount, 0);
  showSampleSize(yRow6, sizeof(sample2));

  volume.setWiperPosition(22);   // set volume to midpoint (instead of maximum, at end of loop above)
  playAudio(sample2, sizeof(sample2));

  delay(AUDIO_CLIP_INTERVAL);

  gLoopCount++;
}
