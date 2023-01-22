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

#include <Wire.h>  // defines "Wire1" as an instance of "Class TwoWire"
#include <Adafruit_MCP4725.h>

Adafruit_MCP4725 dac;

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE "Generate waveforms with the MCP4725 DAC on v6 Griduino"
#define PROGRAM_VERSION "v1.11"
#define PROGRAM_LINE1 "Barry K7BWH"
#define PROGRAM_LINE2 "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ============== constants ====================================
// clang-format off
const int nSine8     = 8;
const int Sine8[8]   = {2048, 3495, 4095, 3495, 2048,  600,    0,  600};

const int nSquare8   = 8;
const int Square8[8] = {4095,    0, 4095,    0, 4095,    0, 4095,    0};

const int nSine16    = 16;
const int Sine16[16] = {2048, 2831, 3495, 3939, 4095, 3939, 3495, 2831,
                        2048, 1264,  600,  156,    0,  156,  600, 1264};

const int nSine32    = 32;
const int Sine32[32] = {2048, 2447, 2831, 3185, 3495, 3750, 3939, 4056,
                        4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
                        2048, 1648, 1264,  910,  600,  345,  156,   39,
                           0,   39,  156,  345,  600,  910, 1264, 1648};

const int nSine64    = 64;
const int Sine64[64] = {2048, 2248, 2447, 2642, 2831, 3013, 3185, 3346,
                        3495, 3630, 3750, 3853, 3939, 4007, 4056, 4085,
                        4095, 4085, 4056, 4007, 3939, 3853, 3750, 3630,
                        3495, 3346, 3185, 3013, 2831, 2642, 2447, 2248,
                        2048, 1847, 1648, 1453, 1264, 1082,  910,  749,
                         600,  465,  345,  242,  156,   88,   39,   10,
                           0,   10,   39,   88,  156,  242,  345,  465,
                         600,  749,  910, 1082, 1264, 1453, 1648, 1847};
// clang-format on

//=========== setup ============================================
void setup(void) {
  delay(10000);  // wait for Serial console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  Serial.println("Compiled for Adafruit Feather RP2040 (product id 4884)");
#else
  Serial.println("Sorry, your hardware platform is not recognized.");
  #error Sorry, your hardware platform is not recognized.
#endif

  // ----- init onboard DAC
  Wire1.begin();
  // Wire1.setClock(3400000);  // setClock() has no detectable effect
  dac.begin(0x61, &Wire1);

  // ----------------------------------------------------------------
  // Test 1: Select bus frequency and measure results on oscilloscope
  // ----------------------------------------------------------------
  // Known bug in hardware causes actual I2C bus frequency to be 2x the requested.
  // So the maximum usable number is (3,400,000 Hz / 2) = 1,700,000
  const int MY_I2C_FREQ = 1700000;  // I2C clock frequency

  /* ...
  Serial.print("Generating a wave using I2C frequency ");
  Serial.println(MY_I2C_FREQ);
  //  unused yields 1.10 kHz 8-step sine,  4.40 kHz square
  //  400000 yields 1.10 kHz 8-step sine,  4.40 kHz square
  //  700000 yields 1.74 kHz 8-step sine,  6.98 kHz square
  // 1000000 yields 2.31 kHz 8-step sine,  9.22 kHz square
  // 1700000 yields 3.27 kHz 8-step sine, 13.09 kHz square <-- max, per specs
  // 2000000 yields 3.61 kHz 8-step sine, 14.46 kHz square
  // 2200000 yields 3.81 kHz 8-step sine, 15.31 kHz square
  // 2400000 yields 4.01 kHz 8-step sine, 16.12 kHz square
  // 2480000 yields 4.01 kHz 8-step sine, 16.12 kHz square <-- max, empirical testing
  // 2500000 returns failure
  // 3000000 returns failure
  // 3400000 returns failure
  while(true) {
    square8(MY_I2C_FREQ);
    // sine8(MY_I2C_FREQ);
    // sine16(MY_I2C_FREQ);
    // sine32(MY_I2C_FREQ);
    // sine64(MY_I2C_FREQ);
  }
  ... */

  // ----------------------------------------------------------------
  // Test 2: Select sidetone and measure results to compute a multiplier
  // ----------------------------------------------------------------
  const int SIDETONE = 800;                 // 800 Hz sidetone
  const int MULTIPLIER = 2085;
  const int CW_I2C_FREQ = SIDETONE * MULTIPLIER;  // I2C clock frequency
  // *1000 yields 483 Hz with sine32()
  // *2000 yields 779 Hz with sine32()
  // *2040 yields 795 Hz with sine32()
  // *2050 yields 796 Hz with sine32()
  // *2080 yields 793 Hz with sine32()      1,664 kHz x 2 = 3,328 kHz clock
  // *2075 yields 793 Hz with sine32()      1,660 kHz x 2 = 3,320 kHz clock
  // *2087 yields 795 Hz with sine32()      1,668 kHz x 2 = 3,336 kHz clock
  // *2100 yields 812 Hz with sine32()      1,680 kHz x 2 = 3,360 kHz clock
  // *2125 yields 820 Hz with sine32()      1,700 kHz x 2 = 3,400 kHz clock
  Serial.print("Target sidetone is ");
  Serial.println(SIDETONE);
  Serial.print("Multiplier for sidetone is ");
  Serial.println(MULTIPLIER);
  Serial.print("Commanded I2C clock frequency is ");
  Serial.println(CW_I2C_FREQ);
  Serial.print("Actual I2C clock frequency is ");
  Serial.println(CW_I2C_FREQ * 2);

  while (true) {
    sine32(CW_I2C_FREQ);
  }
}

//=========== main work loop ===================================
void loop() {
}

// ---------- waveform generators
void sine8(int i2c_freq) {
  for (int ii = 0; ii < nSine8; ii++) {
    dac.setVoltage(Sine8[ii], false, i2c_freq);
  }
}

void sine16(int i2c_freq) {
  for (int ii = 0; ii < nSine16; ii++) {
    dac.setVoltage(Sine16[ii], false, i2c_freq);
  }
}

void sine32(int i2c_freq) {
  for (int ii = 0; ii < nSine32; ii++) {
    dac.setVoltage(Sine32[ii], false, i2c_freq);
  }
}

void sine64(int i2c_freq) {
  for (int ii = 0; ii < nSine64; ii++) {
    dac.setVoltage(Sine64[ii], false, i2c_freq);
  }
}

void square8(int i2c_freq) {
  for (int ii = 0; ii < nSquare8; ii++) {
    dac.setVoltage(Square8[ii], false, i2c_freq);
  }
}
