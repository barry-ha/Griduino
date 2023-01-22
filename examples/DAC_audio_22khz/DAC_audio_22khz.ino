// Please format this file with clang before check-in to GitHub
/*
  DAC audio playback from stored program memory

  Version history:
            2022-06-05 refactored pin definitions into hardware.h
            2021-03-11 updated to alternate between 22khz/float and 8khz/8bit
            2021-03-11 created 16-bit floating point 22khz sketch
            2020-03-14 created 8-bit 8khz sketch

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This sketch is a simple monaural program using only DAC0.
            It plays a mono WAV file sampled at 22,050 Hz.
            Sample audio is stored in program memory.
            Limited length to 3 seconds due to 16-bit array index size:
            65535 bytes * (1 sample/4 bytes) / (1 second / 22050 samples) = 2.97 seconds

  Preparing audio files:
            Prepare your WAV file to 22 kHz mono:
            1. Open Audacity
            2. Open your WAV file, e.g. HEREWEGO.wav
            3. Select "Project rate" of 22050 Hz
            4. Ctrl-A to select all samples
            5. Menu bar > Sample Data Export
               a. Limit output to first: 16000
               b. Measurement scale: Linear
               c. Export data to: e.g. \Documents\Griduino\work_in_progress\HEREWEGO.txt
               d. Index: None   (or use "Sample Count" to see line numbers)
               e. Include header: Standard
               f. Channel layout: L-R on Same Line
               g. Show messages: Yes
            6. The output file contains floating point numbers in the range +1.000 to -1.000, like:

               C:\Users\barry\Documents\Audacity\HEREWEGO.txt   1 channel (mono)
               Sample Rate: 22050 Hz. Sample values on linear scale.
               Length processed: 27255 samples 1.23605 seconds.
               0.26563
               0.24219
               0.26563
               0.26563
               0.26563
               0.28125
               ...

            7. Copy, rename, and edit this into a C++ header file:

               // C:\Users\barry\Documents\Audacity\HEREWEGO.txt   1 channel (mono)
               // Sample Rate: 22050 Hz. Sample values on linear scale.
               // Length processed: 27255 samples 1.23605 seconds.
               #define NUM_SAMPLE1  (sizeof(sample1)/sizeof(sample1[0]))
               const float sample1[] PROGMEM = {
               0.26563,
               0.24219,
               0.26563,
               0.26563,
               0.26563,
               0.28125,
               ...

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
#include "herewego.h"           // audio clip 1, 22 khz/float
#include "sample1.h"            // audio clip 2, 8 khz/8-bit

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE    "DAC Audio 22 kHz"
#define PROGRAM_VERSION  "v1.12"
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
int gVolume   = 32;   // initial digital potentiometer wiper position, 0..99
                      // note that speaker distortion begins at wiper=40 when powered by USB

// ------------ typedef's
struct Point {
  int x, y;
};

// ----- Griduino color scheme
// RGB 565 true color: https://chrishewett.com/blog/true-rgb565-colour-picker/
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND 0x00A            // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cLABEL      ILI9341_GREEN    //
#define cVALUE      ILI9341_YELLOW   // 255, 255, 0
#define cTEXTCOLOR  0x67FF           // rgb(102,255,255) = hsl(180,100,70%)

// ------------ definitions
const int howLongToWait = 5;   // max number of seconds at startup waiting for Serial port to console
int gLoopCount          = 1;

const int gSampleRate = 22050;   // 20 kHz audio file

// ========== splash screen ====================================
const int xLabel = 8;            // indent labels, slight margin on left edge of screen
const int yRow1  = 8;            // title
const int yRow2  = yRow1 + 20;   // compiled date
const int yRow3  = yRow2 + 40;   // loop count
const int yRow4  = yRow3 + 40;   // volume
const int yRow5  = yRow4 + 24;   // wave info

void startSplashScreen() {
  tft.setTextSize(2);

  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print(PROGRAM_TITLE);

  tft.setCursor(xLabel, yRow2);
  tft.setTextColor(cVALUE, cBACKGROUND);
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

int audioFloatToInt(float item) {   // scale floating point sample to DAC integer
  // input:  (-1 ... +1)
  // output: (0 ... 2^12)
  return int((item + 1.0) * 2040.0);
}
void playAudioFloat(const float *audio, unsigned int audiosize, int holdTime) {
  for (int ii = 0; ii < audiosize; ii++) {
    int value = audioFloatToInt(audio[ii]);
    analogWrite(DAC0, value);
    delayMicroseconds(holdTime);   // hold the sample value for the sample time
  }
  // reduce speaker click by setting resting output sample to midpoint
  int midpoint = audioFloatToInt((audio[0] + audio[audiosize - 1]) / 2.0);
  analogWrite(DAC0, midpoint);
}
void playAudio8bit(const unsigned char *audio, int audiosize, int holdTime) {
  for (int ii = 0; ii < audiosize; ii++) {
    int value = audio[ii] << 4;   // max sample is 2^8, max DAC output 2^12, so shift left by 4
    analogWrite(DAC0, value);
    delayMicroseconds(holdTime);   // hold the sample value for the sample time
  }
}
void showWiperPosition(int row, int wiper) {
  tft.setCursor(xLabel, row);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Volume wiper ");

  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(wiper);
  tft.print(" ");
}
void showLoopCount(int row, int loopCount) {
  tft.setCursor(xLabel, row);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Starting playback ");

  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(loopCount);
  tft.print(" ");
}
void showMessage(int x, int y, const char *msg, int value) {
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.setCursor(x, y);
  tft.print(msg);
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(value);
  tft.print("   ");
}
void showWaveInfo(int row, int numItems, int numBytes, int bytesPerItem, int bitrate) {

  float playbackTime = (1.0 / bitrate * numItems);
  int x              = xLabel;
  showMessage(x, row + 0, "Total entries ", numItems);
  showMessage(x, row + 20, "Bytes / sample ", bytesPerItem);
  showMessage(x, row + 40, "Total bytes ", numBytes);
  showMessage(x, row + 60, "Bit rate ", bitrate);

  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.setCursor(xLabel, row + 80);
  tft.print("Playback time ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(playbackTime, 3);
  tft.print("  ");
  /*
  const int c=cTEXTCOLOR;
  const int b=cBACKGROUND;
  const int v=cVALUE;
  tft.setTextColor(c, b); tft.setCursor(xLabel, row + 0); tft.print("Total entries ");  tft.setTextColor(v,b); tft.print(numItems); tft.print("   ");
  tft.setTextColor(c, b); tft.setCursor(xLabel, row +20); tft.print("Bytes / sample "); tft.setTextColor(v,b); tft.print(bytesPerItem); tft.print("   ");
  tft.setTextColor(c, b); tft.setCursor(xLabel, row +40); tft.print("Total bytes ");    tft.setTextColor(v,b); tft.print(numBytes);  tft.print("   ");
  tft.setTextColor(c, b); tft.setCursor(xLabel, row +60); tft.print("Bit rate ");       tft.setTextColor(v,b); tft.print(bitrate);  tft.print("   ");
  tft.setTextColor(c, b); tft.setCursor(xLabel, row +80); tft.print("Playback time ");  tft.setTextColor(v,b); tft.print(playbackTime, 3);  tft.print("   ");
  */
}

//=========== main work loop ===================================
const int AUDIO_CLIP_INTERVAL = 2000;   // msec between one audio clip and the next

void loop() {
  Serial.print("Starting playback ");
  Serial.println(gLoopCount);
  showLoopCount(yRow3, gLoopCount);
  showWiperPosition(yRow4, gVolume);

  // ----- play audio clip at 22 khz/float
  showWaveInfo(yRow5, NUM_SAMPLE22KHZ, sizeof(sample22khz), sizeof(sample22khz[0]), bitrate_22khz);
  playAudioFloat(sample22khz, NUM_SAMPLE22KHZ, holdtime_22khz);   // play sample
  delay(AUDIO_CLIP_INTERVAL);                                     // insert pause between clips

  // ----- play audio clip at 8 khz/8-bit
  showWaveInfo(yRow5, NUM_SAMPLE8KHZ, sizeof(sample8khz), sizeof(sample8khz[0]), bitrate_8khz);
  playAudio8bit(sample8khz, NUM_SAMPLE8KHZ, holdtime_8khz);   // play sample
  delay(AUDIO_CLIP_INTERVAL);                                 // insert pause between clips
  gLoopCount++;
}
