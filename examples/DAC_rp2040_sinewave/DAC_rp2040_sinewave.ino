// Please format this file with clang before check-in to GitHub
/*
  Generate waveforms with the MCP4725 DAC on Griduino v6.

  Version history:
            2022-12-15 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Based on: sinewave.pde, Adafruit Industries, license BSD (see license.txt)
  https://github.com/adafruit/Adafruit_MCP4725/blob/master/examples/sinewave/sinewave.ino

  This is an example sketch for the Adafruit MCP4725 breakout board
  ----> http://www.adafruit.com/products/935
*/
/**************************************************************************/

#include <Wire.h>   // defines "Wire1" as an instance of "Class TwoWire"
#include <Adafruit_MCP4725.h>

Adafruit_MCP4725 dac;

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE    "Generate waveforms with the MCP4725 DAC on v6 Griduino"
#define PROGRAM_VERSION  "v1.11"
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ============== constants ====================================
const int nSine8   = 8;
const int Sine8[8] = {2048, 3495, 4095, 3495, 2048, 600, 0, 600};

const int nSquare8   = 8;
const int Square8[8] = {4095, 0, 4095, 0, 4095, 0, 4095, 0};

//=========== setup ============================================
void setup(void) {
  delay(10000);   // wait for Serial console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

#if defined(ARDUINO_PICO_REVISION)
  Serial.println("Compiled for Adafruit Feather RP2040 (product 4884)");
#else
  Serial.println("Sorry, your hardware platform is not recognized.");
#endif

  // ----- init onboard DAC
  Wire1.begin();
  // Wire1.setClock(3400000);  // setClock() has no detectable effect
  dac.begin(0x61, &Wire1);

#define MY_I2C_FREQ 2000000
  Serial.print("Generating a wave using I2C frequency ");
  Serial.println(MY_I2C_FREQ);
  //  unused yields 1.10 kHz 8-step sine,  4.40 kHz square
  //  400000 yields 1.10 kHz 8-step sine,  4.40 kHz square
  //  700000 yields 1.74 kHz 8-step sine,  6.98 kHz square
  // 1000000 yields 2.31 kHz 8-step sine,  9.22 kHz square
  // 2000000 yields 3.61 kHz 8-step sine, 14.46 kHz square
  // 2200000 yields 3.81 kHz 8-step sine, 15.31 kHz square
  // 2400000 yields 4.01 kHz 8-step sine, 16.12 kHz square
  // 2480000 yields 4.01 kHz 8-step sine, 16.12 kHz square
  // 2500000 returns failure
  // 3000000 returns failure
  // 3400000 returns failure
}

//=========== main work loop ===================================
void loop() {
  // sine8();
  square8();
}

// ---------- waveform generators
void sine8() {
  for (int ii = 0; ii < nSine8; ii++) {
    dac.setVoltage(Sine8[ii], false, MY_I2C_FREQ);
  }
}

void square8() {
  for (int ii = 0; ii < nSquare8; ii++) {
    dac.setVoltage(Square8[ii], false, MY_I2C_FREQ);
  }
}
