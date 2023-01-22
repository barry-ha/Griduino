// Please format this file with clang before check-in to GitHub
/*
  Audio sine wave with RP2040 and TPL0401 volume control (mono)

  Version history:
            2022-12-19 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Sine wave using DAC (MCP4725 chip) and volume control (TPL0401 chip).
            Our main goal is to test the digital potentiometer on the
            v6 pcb layout for volume control.

            This example program will step through increasing volume over time.
            It shows status on display but has no user inputs.

  Based on: example/DAC_rp2040_sinewave (for audio generation)
       and: example/DAC_sine_wave_with_volume_control (for program flow)
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
// #include <DS1804.h>          // DS1804 digital potentiometer library
#include "elapsedMillis.h"   // short-interval timing functions
#include "hardware.h"        // Griduino pin definitions

#include <Wire.h>               // defines "Wire1" as an instance of "Class TwoWire"
#include <Adafruit_MCP4725.h>   // class definition for MCP4725 attached by I2C
Adafruit_MCP4725 dac;           //

#include "TPL0401.h"
TPL0401 vol;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE    "DAC rp2040 Sine Wave with TPL0401 Volume Control"
#define PROGRAM_VERSION  "v1.11"
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

#define SCREEN_ROTATION 1   // 1=landscape, 3=landscape 180-degrees

// ============== constants ====================================
const int howLongToWait = 10;   // max number of seconds before using Serial port to console
const int maxVolume     = 40;   // maximum allowed wiper position on TPL0401 is 0..127
                                // but the speaker sounds distorted around XX? so we stop around 30-50 instead of 127

// clang-format off
const int nSine32    = 32;
const int Sine32[32] = {2048, 2447, 2831, 3185, 3495, 3750, 3939, 4056,
                        4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
                        2048, 1648, 1264,  910,  600,  345,  156,   39,
                           0,   39,  156,  345,  600,  910, 1264, 1648};
// clang-format on

// ============== globals ======================================
int gVolume        = 0;   // initial digital potentiometer wiper position, 0..127
elapsedMicros usec = 0;   //

//=======================================================================
// Set frequency and granularity
const float gFrequency          = 800.0;     // desired output frequency, Hz
const float gSamplesPerWaveform = nSine32;   // steps in each cycle
//=======================================================================

// ----- Griduino color scheme
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
const int yRow6  = yRow5 + 60;   // "Wiper position NN of 127"
const int yRow7  = yRow6 + 20;   // "Pitch 800 Hz"

// ----- Do The Thing
void setVolume(int wiperPosition) {
  vol.setWiper(wiperPosition);   // The Thing!

  Serial.print("Set wiper ");   // report to console
  Serial.println(wiperPosition);

  tft.setCursor(xLabel, yRow6);   // report to TFT display
  tft.print("Wiper position ");
  tft.print(wiperPosition);
  tft.print(" of 127");

  tft.setCursor(xLabel, yRow7);
  tft.print("Pitch ");
  tft.print(gFrequency, 0);
  tft.print(" Hz");
}
void increaseVolume() {
  // send new volume command to DS1804 or TPL0401 digital potentiometer
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

// ============== helpers ======================================
// ----- console Serial port helper
void waitForSerial(int howLong) {
  unsigned long targetTime = millis() + howLong * 1000;
  while (millis() < targetTime) {
    if (Serial)
      break;
  }
}

// ---------- waveform generator
void sine32(int i2c_freq) {
  for (int ii = 0; ii < nSine32; ii++) {
    dac.setVoltage(Sine32[ii], false, i2c_freq);
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

#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  Serial.println("Compiled for Adafruit Pi Pico RP2040 and Griduino Rev.6 board");

  Wire1.begin();
  dac.begin(0x61, &Wire1);
  vol.begin(TPL0401A_I2CADDR_DEFAULT, &Wire1);

  vol.setWiper(10);
  Serial.println(" 0. Set wiper = 10");

#elif defined(SAMD_SERIES)
  Serial.println("Compiled for Adafruit Feather M4 Express and Griduino Rev.4 board)");
  // Only set DAC resolution on devices that have a DAC
  analogWriteResolution(12);   // 1..32, sets DAC output resolution to 12 bit (4096 levels)
                               // because Feather M4 maximum output resolution is 12 bit
#endif

  delay(100);   // settling time

  // ----- debug: echo our initial settings to the console
  char msg[256], ff[32], gg[32];
  String(gFrequency, 1).toCharArray(ff, 32);   // this is how to print 'float' values
  String(gSamplesPerWaveform, 1).toCharArray(gg, 32);
  snprintf(msg, sizeof(msg), "gFrequency(%s), gSamplesPerWaveform(%s)", ff, gg);
  Serial.println(msg);

  /* ...
  snprintf(msg, sizeof(msg), "gDacVolume(%d), gDacOffset(%d)", gDacVolume, gDacOffset);
  Serial.println(msg);
  ... */

  byte wiperPos = 0;   // = volume.getWiperPosition();  // todo
  snprintf(msg, sizeof(msg), "Initial wiper position(%d)", wiperPos);
  Serial.println(msg);

  // ----- announce ourselves
  startSplashScreen();
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTime                = millis();
const int VOLUME_CHANGE_INTERVAL = 600;   // msec between changing volume setting

const int SIDETONE    = 800;                     // 800 Hz sidetone
const int MULTIPLIER  = 2085;                    // empirically determined for 800 Hz
const int CW_I2C_FREQ = SIDETONE * MULTIPLIER;   // I2C clock frequency

void loop() {
  // if a timer or system millis() wrapped around, reset it
  if (prevTime > millis()) {
    prevTime = millis();
  }

  // periodically change speaker volume
  if (millis() - prevTime > VOLUME_CHANGE_INTERVAL) {
    prevTime = millis();   // restart another interval
    increaseVolume();      // update digital potentiometer
  }

  sine32(CW_I2C_FREQ);
}
