// Please format this file with clang before check-in to GitHub
/*
  This is an example program for the BMP3XX temperature & pressure sensor
  updated for Griduino hardware.

  Designed specifically for:
  * BMP380   Tested with Bosch BMP380 chip, surface mounted by JLCPCB.
  * BMP388   http://www.adafruit.com/products/3966   +/- 8 Pascals (0.5 meters altitude)
  * BMP390L  https://www.adafruit.com/product/4816   +/- 3 Pascals (0.25 meters altitude)
          ^- the "L" is "long product support life"
  * Arduino Feather M4 Express (120 MHz SAMD51)     https://www.adafruit.com/product/3857

  This sketch detects and automatically uses either connection type:
  * PCB v4 has BMP388 using SPI
  * PCB v18 has BMP390 using I2C

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!

  Originally written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, preamble source code text above must be included in any redistribution

  From:    https://github.com/adafruit/Adafruit_BMP3XX, see: examples/bmp3xx_simpletest
  Updated: Barry Hansen, K7BWH, barry@k7bwh.com

  Version history:
          2026-11-03 - code review by Claude
          2025-05-10 - updated for PCB rev 14+
          2022-12-28 - support I2C for Feather rp2040 and also SPI for Feather M4
          2021-01-30 - updated for BMP390
          2020-03-05 - adapted for Griduino with BMP388

  Tested with:

          2. Adafruit BMP388 - Precision Barometric Pressure https://www.adafruit.com/product/3966
             Adafruit BMP390                                 https://www.adafruit.com/product/4816

*/

#include <SPI.h>               // Serial Peripheral Interface
#include <Wire.h>              // I2C
#if defined(SAMD_SERIES)
  #warning ----- Compiling for Arduino Feather M4 Express (pcb v4) -----
  #define TARGET_GRIDUINO "PCB v4"
#elif defined(ARDUINO_ARCH_ESP32)
  #warning ----- Compiling for ESP32 S3 (pcb v18) -----
  #define TARGET_GRIDUINO "PCB v17+"
#else
  #error Hardware platform unknown.
#endif

#include "Adafruit_BMP3XX.h"   // Barometric and temperature sensor: https://github.com/adafruit/Adafruit_BMP3XX

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE    "BMP380 / BMP388 /BMP390 Test"
#define PROGRAM_VERSION  "v1.15"
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ---------- Hardware Wiring ----------
#define BMP_CS 13   // BMP388 sensor, chip select

// ------------ definitions
const int howLongToWait = 10;  // max number of seconds before using Serial port to console

#define INCHES_MERCURY_PER_MILLIBAR (0.02953)
#define FEET_PER_METER              (3.28084)
#define SEA_LEVEL_PRESSURE_HPA      (1013.25)

// Developer: Look up your current local sea level pressure on google
//    In Seattle, get current barometric readings from
//    https://forecast.weather.gov/data/obhistory/KSEA.html
//    At my home, it results in altimeter readings of 16.9 - 18.1 feet ASL over 5-minute interval

// ---------- Barometric and Temperature Sensor
Adafruit_BMP3XX baro;

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

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);          // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);  // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION " " TARGET_GRIDUINO);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

  // ----- init BMP380, BMP388 or BMP390 barometer
  bool initialized = baro.begin_SPI(BMP_CS);
  if (initialized) {
    // success - SPI
    Serial.println("--> Successfully initialized barometric sensor with SPI <--");

  } else {
    initialized = baro.begin_I2C(BMP3XX_DEFAULT_ADDRESS, &Wire);
    if (initialized) {
      // success - I2C
      Serial.println("==> Successfully initialized barometric pressure sensor with I2C <==");

    } else {
      // failed to initialize hardware
      while (1)
        Serial.println("Error, unable to initialize BMP380-BMP388-BMP390, check your wiring");
        delay(5000);
    }
  }

  // Set up oversampling and filter initialization
  // https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf
  // Section 3.5 Filter Selection, page 17
  baro.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  baro.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  baro.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  // baro.setOutputDataRate(BMP3_ODR_50_HZ);
}

//=========== main work loop ===================================

void loop() {
  // Read all sensors in the BMP3XX in blocking mode
  if (!baro.performReading()) {
    Serial.println("Error, failed reading the barometric sensor");
    delay(1000);  // prevent a bad sensor from flooding the console
    return;
  }
  float temperature = baro.temperature;                            // celsius
  float pressure    = baro.pressure / 100.0;                       // hectoPascal
  float altitude    = baro.readAltitude(SEA_LEVEL_PRESSURE_HPA);   // meters

  // ---- Print Readings: Console ----------------
  Serial.println();
  Serial.print("Temperature = ");
  Serial.print(temperature, 1);
  Serial.print(" C = ");
  Serial.print(temperature * 9 / 5 + 32, 1);
  Serial.println(" F");

  Serial.print("Pressure = ");
  Serial.print(pressure);
  Serial.print(" hPa = ");
  Serial.print(pressure * INCHES_MERCURY_PER_MILLIBAR);
  Serial.println(" inHg");

  Serial.print("Apprx Altitude = ");
  Serial.print(altitude);
  Serial.print(" m = ");
  Serial.print(altitude * FEET_PER_METER, 1);
  Serial.println(" ft");

  delay(2500);
}
