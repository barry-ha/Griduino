// Please format this file with clang before check-in to GitHub
/*
  Audio sine wave using onboard DAC (mono)

  Version history:
            2022-06-05 refactored pin definitions into hardware.h
            2020-08-19 use the display to show what's happening
            2019-12-28 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Simple Waveform generator with Adafruit Feather M4 Express
            Including DS1804 digital potentiometer for volume control.

            Feather M4 Express has dual DAC (on pins A0 and A1) that
            are capable of playing 12-bit stereo audio.
            This sketch is a simple monaural sine wave using only A0.

            The DAC on the SAMD51 is a 12-bit output, from 0 - 3.3v.
            The largest 12-bit number is 4,096:
            * Writing 0 will set the DAC to minimum (0.0 v) output.
            * Writing 4096 sets the DAC to maximum (3.3 v) output.

            This example program will step through increasing volume over time.
            It shows status on display but has no user inputs.
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include <DS1804.h>             // DS1804 digital potentiometer library
#include "elapsedMillis.h"      // short-interval timing functions
#include "hardware.h"           // Griduino pin definitions

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE    "DAC Sine Wave with DS1804 Volume Control"
#define PROGRAM_VERSION  "v1.12"
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

#define SCREEN_ROTATION 1   // 1=landscape, 3=landscape 180-degrees

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// ---------- TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ============== constants ====================================
const float twopi = PI * 2;
// const int gDacVolume = 2040;        // = maximum allowed multiplier, slightly less than half of 2^12 prevents overflow from rounding errors
const int gDacVolume = 1200;   // = limit DAC output to a value empirically determined, so the audio amp doesn't clip/distort
                               // note: 4-ohm speaker is too much load for the LM386 amplifier
const int gDacOffset = 2048;   // = exactly half of 2^12
                               // Requirement is: gDacOffset +/- gDacVolume = voltage range of DAC
// nst int maxVolume = 99;             // maximum allowed wiper position on digital potentiometer is 0..99
const int maxVolume = 40;   // but the speaker sounds distorted around 26 so we stop at about 30-50

// ctor         DS1804( ChipSel pin, Incr pin,  U/D pin,  maxResistance (K) )
DS1804 volume = DS1804(PIN_VCS, PIN_VINC, PIN_VUD, DS1804_TEN);

int gVolume = 0;   // initial digital potentiometer wiper position, 0..99

// ------------ definitions
float gPhase            = 0.0;   // phase angle, 0..2pi
const int howLongToWait = 10;    // max number of seconds before using Serial port to console
elapsedMicros usec      = 0;     //

//=======================================================================
// Set frequency and granularity
const float gFrequency          = 800.0;   // desired output frequency, Hz
const float gSamplesPerWaveform = 25.0;    // desired steps in each cycle
//=======================================================================

// ----- Griduino color scheme
// RGB 565 true color: https://chrishewett.com/blog/true-rgb565-colour-picker/
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND 0x00A          // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cTEXTCOLOR  ILI9341_CYAN   // 0, 255, 255
#define cLABEL      ILI9341_GREEN

// screen layout
const int xLabel = 10;           // indent labels, slight margin on left edge of screen
const int yRow1  = 10;           // title: "DAC Sine Wave"
const int yRow2  = yRow1 + 40;   // program version
const int yRow3  = yRow2 + 20;   // compiled date
const int yRow4  = yRow3 + 20;   // author line 1
const int yRow5  = yRow4 + 20;   // author line 2
const int yRow6  = yRow5 + 60;   // "Wiper position NN of 99"
const int yRow7  = yRow6 + 20;   // "Pitch 1200 Hz"

float gfDuration = 1E6 / gFrequency / gSamplesPerWaveform;   // microseconds to hold each output sample
float gStep      = twopi / gSamplesPerWaveform;              // radians to advance around unit circle at each step

// ----- Do The Thing
void setVolume(int wiperPosition) {
  // set digital potentiometer
  // @param wiperPosition = 0..99
  volume.setWiperPosition(wiperPosition);
  Serial.print("Set wiper position ");
  Serial.println(wiperPosition);

  tft.setCursor(xLabel, yRow6);
  tft.print("Wiper position ");
  tft.print(wiperPosition);
  tft.print(" of 0-99  ");

  tft.setCursor(xLabel, yRow7);
  tft.print("Pitch ");
  tft.print(gFrequency, 0);
  tft.print(" Hz");
}
void increaseVolume() {
  // send new volume command to DS1804 digital potentiometer
  if (gVolume >= maxVolume) {
    gVolume = 0;
    Serial.println("----- Start over");
  } else {
    // digital pot has linear steps, so to increase by about 3 dB we double the setting
    gVolume = constrain(gVolume + 1, 0, maxVolume);
  }
  setVolume(gVolume);
}

// ========== splash screen helpers ============================
void startSplashScreen() {
  tft.setTextSize(2);

  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print(PROGRAM_TITLE);

  tft.setCursor(xLabel, yRow2);
  tft.setTextColor(cLABEL, cBACKGROUND);
  tft.print(PROGRAM_VERSION);

  tft.setCursor(xLabel, yRow3);
  tft.print(__DATE__ " " __TIME__);   // Report our compiled date

  tft.setCursor(xLabel, yRow4);
  tft.println(PROGRAM_LINE1);

  tft.setCursor(xLabel, yRow5);
  tft.println(PROGRAM_LINE2);

  tft.setCursor(xLabel, yRow6);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
}
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

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);           // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);   // start at full brightness

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(SCREEN_ROTATION);   // landscape (default is portrait)
  clearScreen();

  // ----- init digital potentiometer
  volume.setToZero();
  Serial.println(" 0. Set wiper position = 0");

#if defined(SAMD_SERIES)
  Serial.println("Compiled for Adafruit Feather M4 Express (or equivalent)");
#else
  Serial.println("Sorry, your hardware platform is not recognized.");
#endif

// ----- init onboard DAC
#if defined(SAMD_SERIES)
  // Only set DAC resolution on devices that have a DAC
  analogWriteResolution(12);   // 1..32, sets DAC output resolution to 12 bit (4096 levels)
                               // because Feather M4 maximum output resolution is 12 bit
#endif

  delay(100);                                            // settling time
  gfDuration = 1E6 / gFrequency / gSamplesPerWaveform;   // microseconds to hold each DAC sample
  gStep      = twopi / gSamplesPerWaveform;              // radians to advance after each sample

  // ----- debug: echo our initial settings to the console
  char msg[256], ff[32], gg[32];
  String(gFrequency, 1).toCharArray(ff, 32);   // this is how to print 'float' values
  String(gSamplesPerWaveform, 1).toCharArray(gg, 32);
  sprintf(msg, "gFrequency(%s), gSamplesPerWaveform(%s)", ff, gg);
  Serial.println(msg);

  String(gfDuration, 2).toCharArray(ff, 32);
  String(gStep, 4).toCharArray(gg, 32);
  sprintf(msg, "gfDuration(%s), gStep(%s)", ff, gg);
  Serial.println(msg);
  sprintf(msg, "gDacVolume(%d), gDacOffset(%d)", gDacVolume, gDacOffset);
  Serial.println(msg);

  byte wiperPos = volume.getWiperPosition();
  sprintf(msg, "Initial wiper position(%d)", wiperPos);
  Serial.println(msg);

  // ----- announce ourselves
  startSplashScreen();
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTime                = millis();
const int VOLUME_CHANGE_INTERVAL = 600;   // msec between changing volume setting

void loop() {

  // if a timer or system millis() wrapped around, reset it
  if (prevTime > millis()) {
    prevTime = millis();
  }

  // periodically change speaker volume
  if (millis() - prevTime > VOLUME_CHANGE_INTERVAL) {
    prevTime = millis();   // restart another interval

    increaseVolume();   // update digital potentiometer
  }

  // ----- generate sine wave
  float val = gDacVolume * sin(gPhase) + gDacOffset;
  analogWrite(DAC0, (int)val);
  delayMicroseconds((int)gfDuration);   // Hold the sample value for the sample time

  gPhase += gStep;   // step angle 0.03 radians, at 50 microseconds = 88 Hz
  if (gPhase >= twopi) {
    gPhase = 0;
  }
}
