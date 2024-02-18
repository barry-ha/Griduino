#line 1 "C:\\Users\\barry\\Documents\\Arduino\\Griduino\\examples\\Coin_Battery\\Coin_Battery.ino"
// Please format this file with clang before check-in to GitHub
/*
  Coin_Battery -- simple voltage measurement of coin battery

  Version history:
      2024-02-14 written for Griduino both pcb v4 and v12

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  This example code is in the public domain.
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>

const float analogRef     = 3.3;    // default analog reference voltage
const uint16_t analogBits = 1024;   // default ADC resolution bits
int pcbVersion            = 0;      // default to unknown PCB

#line 22 "C:\\Users\\barry\\Documents\\Arduino\\Griduino\\examples\\Coin_Battery\\Coin_Battery.ino"
bool detectDevice(int address);
#line 66 "C:\\Users\\barry\\Documents\\Arduino\\Griduino\\examples\\Coin_Battery\\Coin_Battery.ino"
int pcb_detect();
#line 80 "C:\\Users\\barry\\Documents\\Arduino\\Griduino\\examples\\Coin_Battery\\Coin_Battery.ino"
void setup();
#line 93 "C:\\Users\\barry\\Documents\\Arduino\\Griduino\\examples\\Coin_Battery\\Coin_Battery.ino"
void loop();
#line 22 "C:\\Users\\barry\\Documents\\Arduino\\Griduino\\examples\\Coin_Battery\\Coin_Battery.ino"
bool detectDevice(int address) {
  // https://github.com/adafruit/Adafruit_BusIO/blob/master/Adafruit_I2CDevice.h
  Adafruit_I2CDevice device(address);
  if (device.detected()) {
    Serial.print("I2C device found at address 0x");
    if (address < 16)
      Serial.print("0");
    Serial.println(address, HEX);

    delay(10);
    device.end();
    return true;
  } else {
    delay(10);
    device.end();
    return false;
  }

  /* *****
  // https://learn.adafruit.com/scanning-i2c-addresses/arduino
  byte error;

  // The i2c_scanner uses the return value of the Write.endTransmission
  // to see if a device did acknowledge the address.
  //WIRE.beginTransmission(address);
  //error = WIRE.endTransmission();

  if (error == 0) {
    Serial.print("I2C device found at address 0x");
    if (address < 16)
      Serial.print("0");
    Serial.println(address, HEX);
    return true;

  } else if (error == 4) {
    Serial.print("Unknown error at address 0x");
    if (address < 16)
      Serial.print("0");
    Serial.println(address, HEX);
  }
  return false;
  ***** */
}

int pcb_detect() {
  Serial.println("Scanning...");
  bool i00 = detectDevice(0x00);
  bool i01 = detectDevice(0x01);
  bool i3E = detectDevice(0x3E);
  bool i77 = detectDevice(0x77);

  if (!i3E || !i77) {
    return 4;
  } else {
    return 11;
  }
}

void setup() {
  Serial.begin(115200);

  // Wait for Serial port to open
  while (!Serial) {
    delay(10);
  }
  delay(500);
  Serial.println(__FILE__);
  Serial.println("Griduino Coin Battery Measurement");
  Serial.println("Sampled resolution is (3.3v)/1024 = 3 mV");
}

void loop() {
  Serial.println("");
  pcbVersion = pcb_detect();
  Serial.print("PCB version = ");
  Serial.println(pcbVersion);

  int coin_adc       = analogRead(A2);
  float coin_voltage = (float)coin_adc * analogRef / analogBits;

  Serial.print("Coin battery sampled = ");
  Serial.println(coin_adc);

  Serial.print("Coin battery voltage = ");
  Serial.print(coin_voltage);
  Serial.print(" volts");

  if (coin_voltage < 2.8) {
    Serial.println(" Warning! Low voltage!");
  } else {
    Serial.println("");
  }

  delay(4000);
}
