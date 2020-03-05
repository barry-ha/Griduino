/***************************************************************************
  This is a library for the BMP3XX temperature & pressure sensor
  

  Designed specifically to work with the Adafruit BMP388 Breakout
  ----> http://www.adafruit.com/products/3966

  This example has been updated to use SPI pins as defined for Griduino.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
  
  From:    https://raw.githubusercontent.com/adafruit/Adafruit_BMP3XX/master/examples/bmp3xx_simpletest/bmp3xx_simpletest.ino
  Updated: Barry Hansen, K7BWH, barry@k7bwh.com
  Date:    March 5, 2020
  Tested:  Arduino Feather M4 Express (120 MHz SAMD51)
  Spec:    https://www.adafruit.com/product/3857

  Pressure Conversions:
  https://www.weather.gov/media/epz/wxcalc/pressureConversion.pdf
  
 ***************************************************************************/

#include <Wire.h>
#include "SPI.h"                  // Serial Peripheral Interface
#include <Adafruit_Sensor.h>      // Adafruit sensor library
#include "Adafruit_BMP3XX.h"      // Precision barometric sensor

// ------- Identity for console
#define PROGRAM_TITLE   "BMP388 Simple Test"
#define PROGRAM_VERSION "v0.1"

// ---------- Hardware Wiring ----------
#define BMP_CS   13            // BMP388 sensor, chip select

// create an instance of the barometric sensor
Adafruit_BMP3XX bmp(BMP_CS);   // hardware SPI

// ------------ definitions
#define SEALEVELPRESSURE_HPA (1038.0)

//=========== setup ============================================
void setup() {

  // ----- init serial monitor
  Serial.begin(115200);
  while (!Serial);

  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " __DATE__ " " __TIME__);  // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  if (!bmp.begin()) {
    Serial.println("Error, unable to initialize BMP388, check your wiring");
    while (1);
  }

  // Set up BMP388 oversampling and filter initialization
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  //bmp.setOutputDataRate(BMP3_ODR_50_HZ);
}

//=========== main work loop ===================================
void loop() {
  if (! bmp.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
  Serial.print("Temperature = ");
  Serial.print(bmp.temperature);
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(bmp.pressure / 100.0);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bmp.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.println();
  delay(2000);
}
