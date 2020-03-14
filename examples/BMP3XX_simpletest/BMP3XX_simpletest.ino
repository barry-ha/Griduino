/***************************************************************************
  This is an example program for the BMP3XX temperature & pressure sensor

  Designed specifically to work with the Adafruit BMP388 Breakout
  ----> http://www.adafruit.com/products/3966

  This example has been updated to use SPI pins as defined for Griduino.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products
  from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
  
  From:    https://github.com/adafruit/Adafruit_BMP3XX, see: examples/bmp3xx_simpletest
  Updated: Barry Hansen, K7BWH, barry@k7bwh.com
  Date:    March 5, 2020
  Tested:  Arduino Feather M4 Express (120 MHz SAMD51)
  Spec:    https://www.adafruit.com/product/3857
  Used in: https://github.com/barry-ha/Griduino

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
// In Seattle, get current barometric readings from
//    https://forecast.weather.gov/data/obhistory/KSEA.html
//    At my home, it results in altimeter readings of 16.9 - 18.1 feet ASL over 5-minute interval

#define SEALEVELPRESSURE_HPA (1016.5)   // at 6am PST Mar 6, 2020

//=========== setup ============================================
void setup() {

  // ----- init serial monitor
  Serial.begin(115200);           // init for debuggging in the Arduino IDE
  while (!Serial);

  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " __DATE__ " " __TIME__);  // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init barometer
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
  // Read all sensors in the BMP3XX in blocking mode
  if (! bmp.performReading()) {
    Serial.println("Error, failed to get reading from barometric sensor");
    return;
  }
  float temperature = bmp.temperature;  // celsius
  float pressure = bmp.pressure / 100.0;  // hectoPascal
  float altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);  // meters
  
  Serial.println();
  Serial.print("Temperature = ");
  Serial.print(temperature, 1);
  Serial.print(" C = ");
  Serial.print(temperature*9/5+32, 1);
  Serial.println(" F");

  Serial.print("Pressure = ");
  Serial.print(pressure / 100.0);
  Serial.println(" hPa");

  Serial.print("Apprx Altitude = ");
  Serial.print(altitude);
  Serial.print(" m = ");
  Serial.print(altitude*3.28084, 1);
  Serial.println(" ft");
  
  delay(2500);
}
