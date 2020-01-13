/*
  Audio sine wave using onboard DAC (mono)

  Date:     2019-12-28 created

  Software: Barry Hansen, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Simple Waveform generator with Adafruit Feather M4 Express
            Including DS1804 digital potentiometer for volume control.

            Feather M4 Express has dual DAC (on pins A0 and A1) that 
            is capable of playing 12-bit stereo audio. 
            This sketch is a simple monaural program using only A0. 

            The DAC on the SAMD51 is a 12-bit output, from 0 - 3.3v.
            The largest 12-bit number is 4,096:
            * Writing 0 will set the DAC to minimum (0.0 v) output.
            * Writing 4096 sets the DAC to maximum (3.3 v) output.

            This example program will change volume every 3 seconds.
            It has no user interface controls or inputs.
*/

#include "DS1804.h"               // DS1804 digital potentiometer library
#include "elapsedMillis.h"        // short-interval timing functions

// ------- Identity for console
#define PROGRAM_TITLE   "DAC Sine Wave with DS1804 Volume Control"
#define PROGRAM_VERSION "v1.0"

// ---------- Hardware Wiring ----------
/*                                Arduino       Adafruit
  ___Label__Description______________Mega_______Feather M4__________Resource____
Audio Out:
   DAC0 - audio signal            - n/a         - A0  (J2 Pin 12) - uses onboard digital-analog converter
Digital potentiometer:
   PIN_VINC - volume increment    - n/a         - D6
   PIN_VUD  - volume up/down      - n/a         - A2
   PIN_VCS  - volume chip select  - n/a         - A1
*/

// ------------ Audio output
#define DAC_PIN      DAC0     // onboard DAC0 == pin A0
#define PIN_SPEAKER  DAC0     // uses DAC

// Adafruit Feather M4 Express pin definitions
#define PIN_VCS      A1       // volume chip select
#define PIN_VINC      6       // volume increment
#define PIN_VUD      A2       // volume up/down

const float twopi = PI * 2;
//const int gDacVolume = 2040;// = maximum allowed multiplier, slightly less than half of 2^12 prevents overflow from rounding errors
const int gDacVolume = 1200;  // = limit DAC output to a value empirically determined, so the audio amp doesn't clip/distort
                              // note: 4-ohm speaker is too much load for the LM386 amplifier
const int gDacOffset = 2048;  // = exactly half of 2^12
                              // Requirement is: gDacOffset +/- gDacVolume = voltage range of DAC
const int maxVolume = 99;     // maximum allowed wiper position on digital potentiometer

// ctor         DS1804( ChipSel pin, Incr pin,  U/D pin,  maxResistance (K) )
DS1804 volume = DS1804( PIN_VCS,     PIN_VINC,  PIN_VUD,  DS1804_TEN );
int gVolume = 0;              // initial digital potentiometer wiper position, 0..99

// ------------ global scope
float gPhase = 0.0;           // phase angle, 0..2pi
const int howLongToWait = 10; // max number of seconds before using Serial port to console
elapsedMicros usec = 0;       // 

//=======================================================================
// Set frequency and granularity
const float gFrequency = 1500.0;          // desired output frequency, Hz
const float gSamplesPerWaveform = 25.0;   // desired steps in each cycle
//=======================================================================

float gfDuration = 1E6 / gFrequency / gSamplesPerWaveform;  // microseconds to hold each output sample
float gStep = twopi / gSamplesPerWaveform;  // radians to advance around unit circle at each step

// ----- Do The Thing
void increaseVolume() {
  // send new volume command to DS1804 digital potentiometer
  if (gVolume >= maxVolume) {
    gVolume = 0;
    Serial.println("----- Start over");
  } else {
      // digital pot has linear steps, so to increase by about 3 dB we double the setting
    gVolume = constrain(gVolume*2+1, 0, maxVolume);
  }

  volume.setWiperPosition(gVolume);
  Serial.print("Set wiper position = "); Serial.println(gVolume);
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for all this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong*1000;
  while (millis() < targetTime) {
    if (Serial) break;
  }
}

//=========== setup ============================================
void setup() {

  // ----- init serial monitor
  Serial.begin(115200);           // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " __DATE__ " " __TIME__);  // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  #if defined(SAMD_SERIES)
    Serial.println("Compiled for Adafruit Feather M4 Express (or equivalent)");
  #else
    Serial.println("Sorry, your hardware platform is not recognized.");
  #endif

  // ----- init digital potentiometer
  volume.setToZero();
  Serial.println(" 0. Set wiper position = 0");

  // ----- init onboard DAC
  #if defined(SAMD_SERIES)
    // Only set DAC resolution on devices that have a DAC
    analogWriteResolution(12);        // 1..32, sets DAC output resolution to 12 bit (4096 levels)
                                      // because Feather M4 maximum output resolution is 12 bit
  #endif

  delay(100);                         // settling time
  gfDuration = 1E6 / gFrequency / gSamplesPerWaveform; // microseconds to hold each DAC sample
  gStep = twopi / gSamplesPerWaveform;                 // radians to advance after each sample

  // ----- debug: echo our initial settings to the console
  char msg[256], ff[32], gg[32];
  String(gFrequency,1).toCharArray(ff, 32);            // this is how to print 'float' values
  String(gSamplesPerWaveform,1).toCharArray(gg, 32);
  sprintf(msg, "gFrequency(%s), gSamplesPerWaveform(%s)", ff, gg);        Serial.println(msg);

  String(gfDuration,2).toCharArray(ff, 32);
  String(gStep,4).toCharArray(gg, 32);
  sprintf(msg, "gfDuration(%s), gStep(%s)", ff, gg);                      Serial.println(msg);
  sprintf(msg, "gDacVolume(%d), gDacOffset(%d)", gDacVolume, gDacOffset); Serial.println(msg);

  byte wiperPos = volume.getWiperPosition();
  sprintf(msg, "Initial wiper position(%d)", wiperPos);                   Serial.println(msg);
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTime = millis();
const int VOLUME_CHANGE_INTERVAL = 3000;    // msec between changing volume setting

void loop() {

  // if our timer or system millis() wrapped around, reset it
  if (prevTime > millis()) {
    prevTime = millis();
  }

  // periodically change speaker volume
  if (millis() - prevTime > VOLUME_CHANGE_INTERVAL) {
    prevTime = millis();     // restart another interval

    increaseVolume();        // update digital potentiometer
  }

  // ----- generate sine wave
  float val = gDacVolume * sin(gPhase) + gDacOffset;
  analogWrite(DAC0, (int)val);
  delayMicroseconds((int)gfDuration);  // Hold the sample value for the sample time

  gPhase += gStep;                     // step angle 0.03 radians, at 50 microseconds = 88 Hz
  if (gPhase >= twopi) {
    gPhase = 0;
  }
}
