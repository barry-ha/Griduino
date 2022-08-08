// Please format this file with clang before check-in to GitHub
/*
  Play spoken words from 2MB QuadSPI memory chip FatFs using DAC audio output

  Version history:
            2022-06-05 refactored pin definitions into hardware.h
            2021-03-28 created to announce random grid names

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This sketch speaks grid names, e.g. "CN87" as "Charlie November Eight Seven"
            Example data (750 KB) includes Barry's recorded voice sampled at 16 khz mono.
            Sample audio is stored and played using the 2MB Flash chip.
            WAV files are stored in the chip by temporarily loading CircuitPy and
            then drag'n drop files from within Windows, then loading our sketch again.
            This sketch is used in Griduino at https://github.com/barry-ha/Griduino

  Preparing audio files:
            Prepare your WAV file to 16 kHz mono:
            1. Open Audacity
            2. Open a project, e.g. \Documents\Arduino\Griduino\work_in_progress\Spoken Word Originals\Barry
            3. Select "Project rate" of 16000 Hz
            4. Select an audio fragment, such as "Charlie"
            5. Menu bar > Effect > Normalize
               a. Remove DC offset
               b. Normalize peaks -1.0 dB
            5. Menu bar > File > Export > Export as WAV
               a. Save as type: WAV (Microsoft)
               b. Encoding: Signed 16-bit PCM
               c. Filename = e.g. "c_bwh_16.wav"
            6. The output file contains 2-byte integer numbers in the range -32767 to +32767

  Mono Audio: The DAC on the SAMD51 is a 12-bit output, from 0 - 3.3v.
            The largest 12-bit number is 4,096:
            * Writing 0 will set the DAC to minimum (0.0 v) output.
            * Writing 4096 sets the DAC to maximum (3.3 v) output.

            This example program has no user inputs.
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include <DS1804.h>             // DS1804 digital potentiometer library
#include "hardware.h"           // Griduino pin definitions
#include "audio_qspi.h"         // Play WAV files from Quad-SPI memory chip

// ------- Identity for splash screen and console --------
#define EXAMPLE_TITLE    "DAC Grid Names"
#define EXAMPLE_VERSION  "v1.08"
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ---------- configuration options
const int SHORT_PAUSE = 300;                // msec between spoken sentences
const int LONG_PAUSE  = SHORT_PAUSE * 10;   // msec between test cases

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// ---------- TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ------------ Audio output
// ctor         DS1804( ChipSel pin, Incr pin,  U/D pin,  maxResistance (K) )
DS1804 volume = DS1804(PIN_VCS, PIN_VINC, PIN_VUD, DS1804_TEN);
int gVolume   = 14;   // initial digital potentiometer wiper position, 0..99
                      // note that speaker distortion begins at wiper=40 when powered by USB

// here's our audio player being demonstrated
AudioQSPI audio_qspi;

// ----- Griduino color scheme
// RGB 565 true color: https://chrishewett.com/blog/true-rgb565-colour-picker/
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND 0x00A            // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cSCALECOLOR 0xF844           //
#define cTEXTCOLOR  ILI9341_CYAN     // 0, 255, 255
#define cLABEL      ILI9341_GREEN    //
#define cVALUE      ILI9341_YELLOW   // 255, 255, 0
#define cWARN       0xF844           // brighter than ILI9341_RED but not pink

// ------------ global scope
const int howLongToWait = 8;   // max number of seconds at startup waiting for Serial port to console

// ========== splash screen ====================================
const int indent = 8;            // indent labels, slight margin on left edge of screen
const int yRow1  = 8;            // title
const int yRow2  = yRow1 + 20;   // compiled date
const int yRow3  = yRow2 + 40;   // loop count
const int yRow4  = yRow3 + 20;   // volume
const int yRow5  = yRow4 + 24;   // wave info

void startSplashScreen() {
  tft.setTextSize(2);

  tft.setCursor(indent, yRow1);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print(EXAMPLE_TITLE);

  tft.setCursor(indent, yRow2);
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
  Serial.println(EXAMPLE_TITLE " " EXAMPLE_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

  // ----- initialize audio interface and look for memory card
  audio_qspi.begin();

  // ----- init digital potentiometer
  volume.unlock();      // unlock digipot (in case someone else, like an example pgm, has locked it)
  volume.setToZero();   // set digipot hardware to match its ctor (wiper=0) because the chip cannot be read
                        // and all "setWiper" commands are really incr/decr pulses. This gets it sync.
  volume.setWiperPosition(gVolume);
  Serial.print("Volume wiper = ");
  Serial.println(gVolume);

  // ----- debug: echo our initial settings to the console
  char msg[256];
  byte wiperPos = volume.getWiperPosition();
  snprintf(msg, sizeof(msg), "Initial wiper position(%d)", wiperPos);
  Serial.println(msg);
}

// ===== screen helpers
void showWiperPosition(int row, int wiper) {
  tft.setCursor(indent, row);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print("Volume wiper ");

  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(wiper);
  tft.print(" ");
}
void showLoopCount(int row, int loopCount) {
  tft.setCursor(indent, row);
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

  Serial.print(msg);
  Serial.println(value);
}
void showWaveInfo(WaveInfo meta) {
  int x = indent;
  int y = yRow5;
  showMessage(x, y + 0, "Number samples ", meta.numSamples);
  showMessage(x, y + 20, "Bytes / sample ", meta.bytesPerSample);
  showMessage(x, y + 40, "Overall file size ", meta.filesize);
  showMessage(x, y + 60, "Samples / sec ", meta.samplesPerSec);
  showMessage(x, y + 80, "Hold time ", meta.holdtime);

  float playbackTime = (1.0 / meta.samplesPerSec * meta.numSamples);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.setCursor(indent, y + 100);
  tft.print("Playback time ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(playbackTime, 3);
  tft.print("  ");
  Serial.print("Playback time ");
  Serial.println(playbackTime, 3);
}

// ------ here's the meat of this potato -------
void sayGrid(const char *name) {
  Serial.print("Say ");
  Serial.println(name);

  for (int ii = 0; ii < strlen(name); ii++) {

    // choose the filename to play
    char myfile[32];
    char letter = name[ii];
    snprintf(myfile, sizeof(myfile), "/male/%c_bwh_16.wav", letter);

    // example: read WAV attributes and display it on screen while playing it
    WaveInfo info;
    audio_qspi.getInfo(&info, myfile);
    showWaveInfo(info);

    // example: play audio through DAC
    bool rc = audio_qspi.play(myfile);
    if (!rc) {
      Serial.print("sayGrid(");
      Serial.print(letter);
      Serial.println(") failed");
      delay(LONG_PAUSE);
    }
  }
}

//=========== main work loop ===================================
int gLoopCount = 1;

void loop() {
  Serial.print("Loop ");
  Serial.println(gLoopCount);
  showLoopCount(yRow3, gLoopCount);
  showWiperPosition(yRow4, gVolume);

  // ----- play audio clip at 16 khz/float
  sayGrid("aaaa");
  delay(LONG_PAUSE);   // extra pause between tests

  sayGrid("abcdefghijklmnopqrstuvwxyz0123456789");
  delay(LONG_PAUSE);   // extra pause between tests

  sayGrid("cn87");
  delay(SHORT_PAUSE);
  sayGrid("cn88");
  delay(SHORT_PAUSE);
  sayGrid("dn07");
  delay(SHORT_PAUSE);
  sayGrid("dn08");
  delay(SHORT_PAUSE);
  sayGrid("k7bwh");
  delay(SHORT_PAUSE);

  delay(LONG_PAUSE);   // extra pause between loops
  gLoopCount++;
}
