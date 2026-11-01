// Please format this file with clang before check-in to GitHub
/*
  Audio sine wave using onboard DAC (mono)

  Version history:
            2026-11-01 code reviewed by Claude
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
            The largest 12-bit number is 4,095:
            * Writing 0 will set the DAC to minimum (0.0 v) output.
            * Writing 4095 sets the DAC to maximum (3.3 v) output.

            This example program will step through increasing volume over time.
            It shows status on display but has no user inputs.
*/

#include <Adafruit_ILI9341.h>  // TFT display, Adafruit: https://github.com/adafruit/Adafruit_ILI9341
#include <DS1804.h>            // digital pot, Rob Tillaart: https://github.com/RobTillaart/DS1804
#include "elapsedMillis.h"     // msec timing, Paul Stoffregen: https://github.com/pfeerick/elapsedMillis/wiki
#include "hardware.h"          // Griduino pin definitions

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE "DAC Sine Wave with DS1804 Volume Control"
#define PROGRAM_VERSION "v1.15"
#define PROGRAM_LINE1 "Barry K7BWH"
#define PROGRAM_LINE2 "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__
#define SCREEN_ROTATION 1  // 1=landscape, 3=landscape 180-degrees

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// ---------- TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ============== constants ====================================
const float twopi = PI * 2;
//nst int gDacVolume = 2040;  // = maximum allowed multiplier, slightly less than half of 2^12 prevents overflow from rounding errors
const int gDacVolume = 1200;  // = limit DAC output to a value empirically determined, so the audio amp doesn't clip/distort
                              // note: 4-ohm speaker is too much load for the LM386 amplifier
const int gDacOffset = 2048;  // = exactly half of 2^12
                              // Requirement is: gDacOffset +/- gDacVolume = voltage range of DAC
const int maxVolume = 40;     // max allowed wiper 0..99, but the speaker sounds distorted around 26 so we stop at about 30-50


// ctor         DS1804(ChipSel, Incr pin, U/D pin, maxResistance (K) )
DS1804 volume = DS1804(PIN_VCS, PIN_VINC, PIN_VUD /*,DS1804_TEN*/);

int gVolume = 0;  // initial digital potentiometer wiper position, 0..99

// ------------ definitions
float gPhase = 0.0;            // phase angle, 0..2pi
const int howLongToWait = 10;  // max number of seconds before using Serial port to console
elapsedMicros usec = 0;        //

//=======================================================================
// Set frequency and granularity
const float gFrequency = 800.0;          // desired output frequency, Hz
const float gSamplesPerWaveform = 25.0;  // desired steps in each cycle

const float gfDuration = 1E6 / gFrequency / gSamplesPerWaveform;  // microseconds to hold each output sample
const float gStep = twopi / gSamplesPerWaveform;                  // radians to advance around unit circle at each step
//=======================================================================

// ----- Griduino color scheme
// RGB 565 true color: https://chrishewett.com/blog/true-rgb565-colour-picker/
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND 0x00A        // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cSCALECOLOR 0xF844       // color picker: http://www.barth-dev.de/online/rgb565-color-picker/
#define cTEXTCOLOR ILI9341_CYAN  // 0, 255, 255
#define cLABEL ILI9341_GREEN
#define cVALUE ILI9341_YELLOW
#define cINPUT ILI9341_WHITE

// screen layout
const int xLabel = 10;         // indent labels, slight margin on left edge of screen
const int yRow1 = 10;          // title: "DAC Sine Wave"
const int yRow2 = yRow1 + 40;  // program version
const int yRow3 = yRow2 + 20;  // compiled date
const int yRow4 = yRow3 + 20;  // author line 1
const int yRow5 = yRow4 + 20;  // author line 2
const int yRow6 = yRow5 + 60;  // "Wiper position NN of 99"
const int yRow7 = yRow6 + 20;  // "Pitch 1200 Hz"

// ----- Do The Thing
int gWiperNow = 0;  // wiper position currently in the DS1804 (the chip cannot be read back)

void setVolume(int wiperPosition) {
  // set digital potentiometer
  // @param wiperPosition = 0..99
  // Move only by the difference from the current position, so the volume
  // does not drop to zero on every step
  if (wiperPosition > gWiperNow) {
    volume.moveUp(wiperPosition - gWiperNow);
  } else if (wiperPosition < gWiperNow) {
    volume.moveDown(gWiperNow - wiperPosition);
  }
  gWiperNow = wiperPosition;

  // These output statements stop sine wave generation. Keep these short to minimize clicks.
  Serial.print("Wiper ");
  Serial.println(wiperPosition);

  tft.setCursor(xLabel, yRow6);
  tft.print(wiperPosition);
}
void increaseVolume() {
  // send new volume command to DS1804 digital potentiometer
  if (gVolume >= maxVolume) {
    gVolume = 0;
    Serial.println("----- Start over");
  } else {
    // digital pot has linear steps of resistance
    // in theory, to change volume by about 3 dB we would double or halve resistance
    // for this example sketch, we add 1 to test all steps
    gVolume = constrain(gVolume + 1, 0, maxVolume);
  }
  setVolume(gVolume);
}

// ========== splash screen ====================================
void startSplashScreen() {
  tft.setTextSize(2);

  tft.setCursor(xLabel, yRow1);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print(PROGRAM_TITLE);

  tft.setCursor(xLabel, yRow2);
  tft.setTextColor(cLABEL, cBACKGROUND);
  tft.print(PROGRAM_VERSION);

  tft.setCursor(xLabel, yRow3);
  tft.print(__DATE__ " " __TIME__);  // Report our compiled date

  tft.setCursor(xLabel, yRow4);
  tft.println(PROGRAM_LINE1);

  tft.setCursor(xLabel, yRow5);
  tft.println(PROGRAM_LINE2);

  tft.setCursor(xLabel, yRow6);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);

  tft.setCursor(xLabel, yRow7);
  tft.print("Pitch ");
  tft.print(gFrequency, 0);
  tft.print(" Hz");

}

// ========== screen helpers ===================================
void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connection to the PC
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
  tft.begin();                       // initialize TFT display
  tft.setRotation(SCREEN_ROTATION);  // 1=landscape (default is 0=portrait)
  clearScreen();                     // note that "tft.begin()" does not clear screen

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);  // start backlight at full brightness

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);          // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);  // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init digital potentiometer
  // Griduino PCB v4 has DS1804 volume control using SPI
  pinMode(PIN_VCS, OUTPUT);  // init pin in case another sketch changed it
  volume.begin();            // required by RobTillaart's library
  volume.select(true);       // required
  volume.moveDown(100);      // reset volume to zero

  gWiperNow = 0;
  Serial.println(" 0. Set wiper position = 0");

#if defined(SAMD_SERIES)
  Serial.println("Compiled for Adafruit Feather M4 Express (or equivalent)");
#else
  Serial.println("Sorry, your hardware platform is not recognized.");
#endif

// ----- init onboard DAC
#if defined(SAMD_SERIES)
  // Only set DAC resolution on devices that have a DAC
  analogWriteResolution(12);  // 1..32, sets DAC output resolution to 12 bit (4096 levels)
                              // because Feather M4 maximum output resolution is 12 bit
  delay(100);                 // settling time
#endif

  // ----- debug: echo our initial settings to the console
  char msg[256], ff[32], gg[32];
  String(gFrequency, 1).toCharArray(ff, 32);  // this is how to print 'float' values
  String(gSamplesPerWaveform, 1).toCharArray(gg, 32);
  sprintf(msg, "gFrequency(%s), gSamplesPerWaveform(%s)", ff, gg);
  Serial.println(msg);

  String(gfDuration, 2).toCharArray(ff, 32);
  String(gStep, 4).toCharArray(gg, 32);
  sprintf(msg, "gfDuration(%s), gStep(%s)", ff, gg);
  Serial.println(msg);
  sprintf(msg, "gDacVolume(%d), gDacOffset(%d)", gDacVolume, gDacOffset);
  Serial.println(msg);

  //byte wiperPos = volume.getWiperPosition();
  //sprintf(msg, "Initial wiper position(%d)", wiperPos);
  //Serial.println(msg);

  // ----- announce ourselves
  startSplashScreen();
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTime = millis();
const int VOLUME_CHANGE_INTERVAL = 600;  // msec between changing volume setting

void loop() {

  // periodically change speaker volume
  if (millis() - prevTime > VOLUME_CHANGE_INTERVAL) {
    prevTime = millis();  // restart another interval

    increaseVolume();  // update digital potentiometer
  }

  // ----- generate sine wave
  float val = gDacVolume * sin(gPhase) + gDacOffset;
  analogWrite(DAC_PIN, (int)val);
  delayMicroseconds((int)gfDuration);  // Hold the sample value for the sample time

  gPhase += gStep;  // step angle 0.03 radians, at 50 microseconds = 88 Hz
  if (gPhase >= twopi) {
    gPhase = 0;
  }
}
