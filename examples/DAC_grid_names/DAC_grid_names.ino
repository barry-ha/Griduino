// Please format this file with clang before check-in to GitHub
/*
  Play spoken words from stored program memory using DAC audio output

  Date:     2021-03-12 created to announce random grid names

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This sketch speaks grid names, e.g. "CN87" as "Charlie November Eight Seven"
            It plays Barry's recorded voice sampled at 16 khz.
            Sample audio is stored in program memory.

  Preparing audio files:
            Prepare your WAV file to 16 kHz mono:
            1. Open Audacity
            2. Open a project, e.g. \Documents\Arduino\Griduino\work_in_progress\Spoken Word Originals\Barry
            3. Select "Project rate" of 16000 Hz
            4. Select a piece of audio, such as "Charlie"
            5. Menu bar Effect > Normalize > Remove DC offset, normalize peaks -1.0 dB
            5. Menu bar > Sample Data Export
               a. Limit output to first: 99999
               b. Measurement scale: Linear
               c. Export data to: e.g. C_BARRY_16.txt
               d. Index: None   (or use "Sample Count" to see line numbers)
               e. Include header: All
               f. Channel layout: L-R on Same Line
               g. Show messages: Yes
            6. The output file contains floating point numbers in the range +1.000 to -1.000, like:
            
               C:\Users\barry\Documents\Arduino\Griduino\work_in_progress\Spoken Word Originals\Barry\C_BARRY_16.txt
               Sample Rate: 16000 Hz. Sample values on linear scale. 1 channel (mono).
               Length processed: 8336 samples, 0.52100 seconds.
               Peak amplitude: 0.89049 (linear) -1.00746 dB.  Unweighted RMS: -14.10552 dB.
               DC offset: -0.00002 linear, -94.45499 dB.

               -0.00218
               -0.00451
               -0.00135
               0.00394
               0.00490
               0.00142
               ...

            7. Edit this into a C++ header file:
               (Hint: change all "0." to ",0.")
               (Hint: change all "-,0" to ",-0")
            
               // C:\Users\barry\Documents\Arduino\Griduino\work_in_progress\Spoken Word Originals\Barry\C_BARRY_16.txt
               // Sample Rate: 16000 Hz. Sample values on linear scale. 1 channel (mono).
               // Length processed: 8336 samples, 0.52100 seconds.
               // Peak amplitude: 0.89049 (linear) -1.00746 dB.  Unweighted RMS: -14.10552 dB.
               // DC offset: -0.00002 linear, -94.45499 dB.
               const LetterInfo c_info = {
                 'c',                   // key
                 c_barry_16,            // ptr to array of float
                 16000,                 // bitrate 
                 sizeof(c_barry_16),    // number of bytes in this wave table
                 sizeof(c_barry_16)/sizeof(c_barry_16[0])    // number of samples in this wave file
               };
               const float c_barry_16[] = {
               -0.00218
               -0.00451
               -0.00135
               0.00394
               0.00490
               0.00142
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

struct LetterInfo {
  // LetterInfo describes everything about a single sampled sound stored in memory
  const char letter;      // key, e.g. 'c'
  const float *pTable;    // ptr to array of float,               e.g. 'c_barry_16'
  const int bitrate;      // bitrate,                             e.g. '16000'
  const int totalBytes;   // number of bytes in this wave table,  e.g. 'sizeof(c_barry_16)'
  const int numSamples;   // number of samples in this wave file, e.g. 'sizeof(c_barry_16)/sizeof(c_barry_16[0])'
};
#include "sound\a_barry_16.h"
#include "sound\b_barry_16.h"
#include "sound\c_barry_16.h"
#include "sound\d_barry_16.h"
#include "sound\e_barry_16.h"
#include "sound\f_barry_16.h"
#include "sound\g_barry_16.h"
#include "sound\h_barry_16.h"
//#include "sound\i_barry_16.h"
//#include "sound\j_barry_16.h"
//#include "sound\k_barry_16.h"
//#include "sound\l_barry_16.h"
//#include "sound\m_barry_16.h"
#include "sound\n_barry_16.h"
//#include "sound\o_barry_16.h"
//#include "sound\p_barry_16.h"
//#include "sound\q_barry_16.h"
//#include "sound\r_barry_16.h"
//#include "sound\s_barry_16.h"
//#include "sound\t_barry_16.h"
//#include "sound\u_barry_16.h"
//#include "sound\v_barry_16.h"
//#include "sound\w_barry_16.h"
//#include "sound\x_barry_16.h"
//#include "sound\y_barry_16.h"
//#include "sound\z_barry_16.h"
#include "sound\0_barry_16.h"
#include "sound\1_barry_16.h"
#include "sound\2_barry_16.h"
#include "sound\3_barry_16.h"
#include "sound\4_barry_16.h"
#include "sound\5_barry_16.h"
#include "sound\6_barry_16.h"
#include "sound\7_barry_16.h"
#include "sound\8_barry_16.h"
#include "sound\9_barry_16.h"

// ------- Identity for splash screen and console --------
#define EXAMPLE_TITLE    "DAC Grid Names"
#define EXAMPLE_VERSION  "v0.38"
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ---------- Hardware Wiring ----------
// Same as Griduino platform

// ---------- Touch Screen
#define TFT_BL 4    // TFT backlight
#define TFT_CS 5    // TFT chip select pin
#define TFT_DC 12   // TFT display/command pin

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ------------ Audio output
#define DAC_PIN     DAC0   // onboard DAC0 == pin A0
#define PIN_SPEAKER DAC0   // uses DAC

// Adafruit Feather M4 Express pin definitions
#define PIN_VCS  A1   // volume chip select
#define PIN_VINC 6    // volume increment
#define PIN_VUD  A2   // volume up/down

// ctor         DS1804( ChipSel pin, Incr pin,  U/D pin,  maxResistance (K) )
DS1804 volume = DS1804(PIN_VCS, PIN_VINC, PIN_VUD, DS1804_TEN);
int gVolume   = 32;   // initial digital potentiometer wiper position, 0..99
                      // note that speaker distortion begins at wiper=40 when powered by USB

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define BACKGROUND     0x00A            // a little darker than ILI9341_NAVY
#define cBACKGROUND    0x00A            // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cGRIDNAME      ILI9341_GREEN    //
#define cLABEL         ILI9341_GREEN    //
#define cDISTANCE      ILI9341_YELLOW   //
#define cVALUE         ILI9341_YELLOW   // 255, 255, 0
#define cVALUEFAINT    0xbdc0           // darker than cVALUE
#define cDISABLED      0x7bee           // 125, 125, 115 = gray for disabled screen item
#define cHIGHLIGHT     ILI9341_WHITE    //
#define cBUTTONFILL    ILI9341_NAVY     //
#define cBUTTONOUTLINE 0x0514           // was ILI9341_CYAN
#define cBREADCRUMB    ILI9341_CYAN     //
#define cTITLE         ILI9341_GREEN    //
#define cTEXTCOLOR     ILI9341_CYAN     // 0, 255, 255
#define cFAINT         0x0514           // 0, 160, 160 = blue, between CYAN and DARKCYAN
#define cBOXDEGREES    0x0410           // 0, 128, 128 = blue, between CYAN and DARKCYAN
#define cBUTTONLABEL   ILI9341_YELLOW   //
#define cCOMPASS       ILI9341_BLUE     // a little darker than cBUTTONOUTLINE
#define cWARN          0xF844           // brighter than ILI9341_RED but not pink
#define cTOUCHTARGET   ILI9341_RED      // outline touch-sensitive areas

// ------------ typedef's
struct Point {
  int x, y;
};

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND 0x00A   // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cSCALECOLOR 0xF844
#define cTEXTCOLOR  ILI9341_CYAN   // 0, 255, 255
#define cLABEL      ILI9341_GREEN
#define cVALUE      ILI9341_YELLOW   // 255, 255, 0

// ------------ global scope
const int howLongToWait = 5;   // max number of seconds at startup waiting for Serial port to console
int gLoopCount          = 1;

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
  tft.print(EXAMPLE_TITLE);

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

  // ----- init serial monitor
  Serial.begin(115200);           // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(EXAMPLE_TITLE " " EXAMPLE_VERSION);   // Report our program name to console
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
//void playAudio8bit(const unsigned char* audio, int audiosize, int holdTime) {
//  for (int ii=0; ii<audiosize; ii++) {
//    int value = audio[ii] << 4;       // max sample is 2^8, max DAC output 2^12, so shift left by 4
//    analogWrite(DAC0, value);
//    delayMicroseconds(holdTime);     // hold the sample value for the sample time
//  }
//}
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
}

// ------ here's the meat of this potato -------
void sayGrid(const char *name) {
  Serial.print("Say ");
  Serial.println(name);
  for (int ii = 0; ii < strlen(name); ii++) {

    char letter = name[ii];
    const LetterInfo* pInfo = getLetterInfo(letter);
  
#if(1)
    // removed - this causes linker error message:
    // collect2.exe: error: ld returned 1 exit status
    // Multiple libraries were found for "Adafruit_ZeroDMA.h"
    // Used:     C:\Users\barry\Documents\ArduinoData\packages\adafruit\hardware\samd\1.6.3\libraries\Adafruit_ZeroDMA
    // Not used: C:\Users\barry\Documents\Arduino\libraries\Adafruit_Zero_DMA_Library
    // exit status 1

    char letter             = name[ii];
    const LetterInfo *pInfo = &c_info;
    switch (letter) {
    case 'a':   pInfo = &a_info;   break;
    case 'b':   pInfo = &b_info;   break;
    case 'c':   pInfo = &c_info;   break;
    case 'd':   pInfo = &d_info;   break;
    case 'e':   pInfo = &e_info;   break;
    case 'f':   pInfo = &f_info;   break;
    case 'g':   pInfo = &g_info;   break;
    case 'h':   pInfo = &h_info;   break;
    case 'n':   pInfo = &n_info;   break;
    case '0':   pInfo = &w0_info;  break;
    case '1':   pInfo = &w1_info; break;
    case '2':   pInfo = &w2_info; break;
    case '3':   pInfo = &w3_info; break;
    case '4':   pInfo = &w4_info; break;
    case '5':   pInfo = &w5_info; break;
    case '6':   pInfo = &w6_info; break;
    case '7':   pInfo = &w7_info;  break;
    case '8':   pInfo = &w8_info;  break;
    case '9':   pInfo = &w9_info;  break;
    default:
      Serial.print("Letter ");
      Serial.print(letter);
      Serial.println(" not found in wave table");
      break;
      return;
    }
#endif
    int holdtime = 1E6 / pInfo->bitrate;
    showWaveInfo(yRow5, pInfo->numSamples, pInfo->totalBytes, sizeof(pInfo->pTable), pInfo->bitrate);
    playAudioFloat(pInfo->pTable, pInfo->numSamples, holdtime);   // play entire sample
  }
}

//=========== main work loop ===================================
const int AUDIO_CLIP_INTERVAL = 150;   // msec between complete spoken grid squares

void loop() {

  Serial.print("Loop ");
  Serial.println(gLoopCount);
  showLoopCount(yRow3, gLoopCount);
  showWiperPosition(yRow4, gVolume);

  // ----- play audio clip at 16 khz/float
  sayGrid("cn87");
  delay(AUDIO_CLIP_INTERVAL);
  sayGrid("cn88");
  delay(AUDIO_CLIP_INTERVAL);
  sayGrid("cn89");
  delay(AUDIO_CLIP_INTERVAL);
  sayGrid("dn07");
  delay(AUDIO_CLIP_INTERVAL);
  sayGrid("dn08");
  delay(AUDIO_CLIP_INTERVAL);   // insert pause between clips

  delay(AUDIO_CLIP_INTERVAL * 10);   // extra pause between loops
  gLoopCount++;
}
