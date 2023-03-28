// Please format this file with clang before check-in to GitHub
/*
  I2C_Scan -- poll addresses on the I2C bus

  Version history:
      2023-03-26 adapted from Adafruit example for Griduino pcb v7
                 Note the Adafruit Feather rp2040 requires Wire1 (not Wire)

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Based on:   https://learn.adafruit.com/adafruit-qt-py-esp32-s3/i2c-scan-test
  Similar to: https://learn.adafruit.com/scanning-i2c-addresses/arduino

  This example code is in the public domain.
*/

#include <Adafruit_TestBed.h>
extern Adafruit_TestBed TB;

#define DEFAULT_I2C_PORT &Wire

#define SECONDARY_I2C_PORT &Wire1

void setup() {
  Serial.begin(115200);

  // Wait for Serial port to open
  while (!Serial) {
    delay(10);
  }
  delay(500);
  Serial.println(__FILE__);
  Serial.println("Adafruit I2C Scanner");
}

void loop() {
  Serial.println("");
  Serial.println("");

  Serial.print("Default port (Wire) ");
  TB.theWire = DEFAULT_I2C_PORT;
  TB.printI2CBusScan();

  Serial.print("Secondary port (Wire1) ");
  TB.theWire = SECONDARY_I2C_PORT;
  TB.printI2CBusScan();

  delay(5000);
}