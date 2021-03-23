// Please format this file with clang before check-in to GitHub
/*
  Play spoken words from stored program memory using DAC audio output

  Date:     2021-03-12 created to announce random grid names

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This sketch speaks grid names, e.g. "CN87" as "Charlie November Eight Seven"
            It plays Barry's recorded voice sampled at 16 khz.
            Sample audio is stored in the 2MB Flash chip.
            WAV files are stored in the chip by temporarily loading CircuitPy
            and then drag'n drop files from within Windows, then loading our sketch again.

  Conclusion:
            .

  Preparing audio files:
            Prepare your WAV file to 16 kHz mono:
            1. Open Audacity
            2. Open a project, e.g. \Documents\Arduino\Griduino\work_in_progress\Spoken Word Originals\Barry
            3. Select "Project rate" of 16000 Hz
            4. Select a piece of audio, such as "Charlie"
            5. Menu bar Effect > Normalize > Remove DC offset, normalize peaks -1.0 dB
            5. Menu bar > File > Export > Export selected audio
               a. todo
               b. Choose WAV, Unsigned int 16-bit
               c. Filename = e.g. "h_bwh_16.wav"
            6. The output file contains floating point numbers in the range -16535 to +16535, like:

  Relevant background:
            http://www.lightlink.com/tjweber/StripWav/WAVE.txt
            http://www.lightlink.com/tjweber/StripWav/Canon.html
            https://www.instructables.com/Playing-Wave-file-using-arduino/
            https://www.arduino.cc/en/Tutorial/SimpleAudioPlayer
            https://learn.adafruit.com/introducing-itsy-bitsy-m0/using-spi-flash
            https://www.arduino.cc/en/reference/SD
            https://www.arduino.cc/en/Reference/FileRead
            https://forum.arduino.cc/index.php?topic=695228.0
  
  Mono Audio: The DAC on the SAMD51 is a 12-bit output, from 0 - 3.3v.
            The largest 12-bit number is 4,096:
            * Writing 0 will set the DAC to minimum (0.0 v) output.
            * Writing 4096 sets the DAC to maximum (3.3 v) output.
            This example program has no user inputs.
*/

#include <Adafruit_ILI9341.h>    // TFT color display library
#include <SdFat.h>               // for FAT file systems on Flash and Micro SD cards
#include <Adafruit_SPIFlash.h>   // for FAT file systems on SPI flash chips
#include <DS1804.h>              // DS1804 digital potentiometer library
#include "elapsedMillis.h"       // short-interval timing functions

// ------- Identity for splash screen and console --------
#define EXAMPLE_TITLE    "DAC Grid Names"
#define EXAMPLE_VERSION  "v0.38"
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

struct WaveInfo {
  // this is what we want to know about a single "Microsoft WAV" 16-bit PCM sampled sound
  char letter;             // key, e.g. 'c'
  char filename[32];       // filename,                           e.g. "/male/c_bwh_16.wav"
  int bitrate;             // bitrate,                            e.g. '16000'
  int filesize;            // total number of bytes in this WAV file
  int numBytesPerSample;   // number of bytes in each sample      e.g. '2' for 16-bit int
  int numSamples;          // number of samples in this wave file
  int holdtime;            // microseconds to hold each sample    e.g. 1e6/bitrate = 62 usec
};

// ---------- Hardware Wiring ----------
// Same as Griduino platform

// ---------- Touch Screen
#define TFT_BL 4    // TFT backlight
#define TFT_CS 5    // TFT chip select pin
#define TFT_DC 12   // TFT display/command pin

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Flash chip
// SD card share the hardware SPI interface with TFT display, and have
// separate 'select' pins to identify the active device on the bus.
const int chipSelectPin = 7;
const int chipDetectPin = 8;

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

// ------------ typedef's
struct Point {
  int x, y;
};

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND 0x00A            // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cSCALECOLOR 0xF844           //
#define cTEXTCOLOR  ILI9341_CYAN     // 0, 255, 255
#define cLABEL      ILI9341_GREEN    //
#define cVALUE      ILI9341_YELLOW   // 255, 255, 0
#define cWARN       0xF844           // brighter than ILI9341_RED but not pink

// ------------ global scope
const int howLongToWait = 8;                    // max number of seconds at startup waiting for Serial port to console
Adafruit_FlashTransport_QSPI gFlashTransport;   // Quad-SPI 2MB memory chip
Adafruit_SPIFlash gFlash(&gFlashTransport);     //
FatFileSystem gFatfs;                           // file system object from SdFat

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

// ========== file system helpers ==============================
#define MAXBUFFERSIZE 32000   // max = 32K @ 16 khz = max 2.0 seconds
int openFlash() {
  // returns 1=success, 0=failure

  // Initialize flash library and check its chip ID.
  if (!gFlash.begin()) {
    showErrorMessage("Error, failed to initialize onboard memory.");
    return 0;
  }
  Serial.print(". Flash chip JEDEC ID: 0x");
  Serial.println(gFlash.getJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!gFatfs.begin(&gFlash)) {
    showErrorMessage("Error, failed to mount filesystem");
    showErrorMessage("Was the flash chip formatted with the SdFat_format example?");
    return 0;
  }
  Serial.println(". Mounted SPI flash filesystem");
  return 1;
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

  // ----- look for memory card
  Serial.print("Detecting Flash memory using pin ");
  Serial.println(chipDetectPin);
  pinMode(chipDetectPin, INPUT_PULLUP);               // use internal pullup resistor
  bool isCardDetected = digitalRead(chipDetectPin);   // HIGH = no card; LOW = card detected
  if (isCardDetected) {
    Serial.println(". Success - found a memory chip");
  } else {
    Serial.println(". Failed - no memory chip found");
  }

  // ----- init FLASH memory chip on the Feather M4 board
  Serial.println("Initializing interface to Flash memory...");
  int result = openFlash();   // open file system

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
unsigned int scale16BitToDAC(int sample) {
  // input:  (-32767 ... +32767)
  // output: (0 ... 2^12)
  return map(sample, -32767,+32767, 0,4096);
}
void playAudio16Bit(int audio[MAXBUFFERSIZE], int audiosize, int holdTime) {
  for (int ii = 0; ii < audiosize; ii++) {
    int value = scale16BitToDAC(audio[ii]);
    analogWrite(DAC0, value);
    delayMicroseconds(holdTime);   // hold the sample value for the sample time
  }
}
//void playAudio8bit(const unsigned char* audio, int audiosize, int holdTime) {
//  for (int ii=0; ii<audiosize; ii++) {
//    int value = audio[ii] << 4;       // max sample is 2^8, max DAC output 2^12, so shift left by 4
//    analogWrite(DAC0, value);
//    delayMicroseconds(holdTime);     // hold the sample value for the sample time
//  }
//}
// ===== screen helpers
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
void showErrorMessage(const char *error) {
  // error is so critical we have to stop
  Serial.println(error);

  tft.setCursor(xLabel, tft.height()/2);
  tft.setTextColor(cWARN, cBACKGROUND);
  tft.print(error);
}
void showWaveInfo(int row, WaveInfo meta) {
  int x = xLabel;
  showMessage(x, row + 0, "Total entries ", meta.numSamples);
  showMessage(x, row + 20, "Bytes / sample ", meta.numBytesPerSample);
  showMessage(x, row + 40, "Total bytes ", meta.filesize);
  showMessage(x, row + 60, "Bit rate ", meta.bitrate);

  float playbackTime = (1.0 / meta.bitrate * meta.numSamples);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.setCursor(xLabel, row + 80);
  tft.print("Playback time ");
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(playbackTime, 3);
  tft.print("  ");
}
bool getWaveData(WaveInfo *pInfo, int pBuffer[MAXBUFFERSIZE], const char key) {
  // input: a single character
  // output: meta data filled in about the WAV file
  //         buffer filled in with data
  //         all files and file handles are closed
  bool rc       = true;   // assume success
  pInfo->letter = key;
  snprintf(pInfo->filename, sizeof(pInfo->filename), "/male/%c_bwh_16.wav", key);   // todo: select voice
  pInfo->bitrate           = 16000;   // todo: read bitrate from WAV file
  pInfo->filesize          = 0;       // todo: read filesize from WAV file
  pInfo->numBytesPerSample = 2;       // todo: read from WAV file
  pInfo->numSamples        = 0;       // todo: read #samples from WAV file

  // open wave file from 2MB flash memory
  Serial.print("Opening file: ");
  Serial.println(pInfo->filename);

  if (gFatfs.exists(pInfo->filename)) {
    // success
  } else {
    Serial.println(". EXISTS error, file not found");
  }
  
  File myFile = gFatfs.open(pInfo->filename);
  if (myFile) {
    pInfo->filesize = myFile.size();
    char header[44];
    int bytesread   = myFile.read(header, sizeof(header));
    if (bytesread == sizeof(header)) {
      //   Offset  Length   Contents
      //    0       4 bytes  'RIFF'
      //    4       4 bytes  <file length - 8>
      //    8       4 bytes  'WAVE'
      //    12      4 bytes  'fmt '
      //    16      4 bytes  0x00000010     // Length of the fmt data (16 bytes)
      //    20      2 bytes  0x0001         // Format tag: 1 = PCM
      //    22      2 bytes  <channels>     // Channels: 1 = mono, 2 = stereo
      //    24      4 bytes  <sample rate>  // Samples per second: e.g., 44100
      //    28      4 bytes  <bytes/second> // sample rate * block align
      //    32      2 bytes  <block align>  // channels * bits/sample / 8
      //    34      2 bytes  <bits/sample>  // 8 or 16
      //    36      4 bytes  'data'
      //    40      4 bytes  <length of the data block>
      //    44        bytes  <sample data>
      if (header[0]=='R' && header[1]=='I' && header[2]=='F' && header[3]=='F') {
        // ----- all good so far!
        Serial.println(". Successfully read WAV file header");
        //  data is successfully written to caller's buffer
        //  todo: parse the data (if you want to support mono/stereo, other formats, other PCM, other bit rates)
        // -----
      } else {
        Serial.println(". DATA error, file does not begin with 'RIFF'");
        rc = false;
      }
    } else {
      Serial.println(". READ error, did not read any bytes from file");
      rc = false;
    }
  } else {
    Serial.println(". OPEN error opening file");
    rc = false;
  }
  myFile.close();
  return rc;
}

// ------ here's the meat of this potato -------
void sayGrid(const char *name) {
  Serial.print("Say ");
  Serial.println(name);
  for (int ii = 0; ii < strlen(name); ii++) {

    char letter = name[ii];
    WaveInfo waveMeta;
    int waveData[MAXBUFFERSIZE];

    if (getWaveData(&waveMeta, waveData, letter)) {
      showWaveInfo(yRow5, waveMeta);
      playAudio16Bit(waveData, waveMeta.numSamples, waveMeta.holdtime);
    }
  }
}

//=========== main work loop ===================================
const int AUDIO_CLIP_INTERVAL = 150;   // msec between complete spoken grid squares
int gLoopCount                = 1;

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
  sayGrid("abcdn01789");
  delay(AUDIO_CLIP_INTERVAL);   // insert pause between clips

  delay(AUDIO_CLIP_INTERVAL * 10);   // extra pause between loops
  gLoopCount++;
}
