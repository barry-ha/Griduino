#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     moodel_crypto.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Griduino v14+ contains an Atmel CryptoAuthentication ATSHA204A chip.
            We use this module to report a Griduino serial number, and to
            report PCB revision level.

  Library:  https://github.com/sparkfun/SparkFun_ATSHA204_Arduino_Library
            https://github.com/mengguang/atsha204_i2c

  Example:  https://github.com/mengguang/atsha204_i2c/blob/master/examples/atsha204_simple_example/atsha204_simple_example.ino

  Spec:     https://ww1.microchip.com/downloads/en/DeviceDoc/ATSHA204A-Data-Sheet-40002025A.pdf
            Attention Please: we need to set BUFFER_LENGTH to at least 64 in Wire.h
*/

#include <Arduino.h>
#include <Wire.h>
#include <sha204_i2c.h>
// #include "sha204_lib_return_codes.h"

// ========== extern ===========================================
extern Logger logger;   // Griduino.ino

// ========== class BatteryVoltage ==================================
class CryptoAuthentication {
private:
  // Attention: we need to set BUFFER_LENGTH to at least 64 in Wire.h
  atsha204Class sha204;

public:
  // Griduino v7+ uses Analog input pin to measure 3v coin battery
  CryptoAuthentication() {}   // ctor

  bool canReadBattery = false;   // assume PCB rev.4
  uint8_t serialNumber[6];
  int pcbRevision = 0;   // todo

  void begin() {
    logger.log(CONFIG, INFO, "Initialize Atmel CryptoAuthentication ATSHA204A chip...");

    // Remember to set BUFFER_LENGTH to at 64 in Wire.h
    Wire.begin();
    // Serial.begin(115200);

    logger.log(CONFIG, DEBUG, "Wake up SHA204 chip");
    int rc = sha204.simpleWakeup();
    if (rc == SWI_FUNCTION_RETCODE_SUCCESS) {   // = SHA204_SUCCESS = 0

      logger.log(CONFIG, DEBUG, "Get SHA204 id");
      rc = sha204.simpleGetSerialNumber(serialNumber);

      if (rc == SWI_FUNCTION_RETCODE_SUCCESS) {   // = SHA204_SUCCESS = 0
        logger.log(CONFIG, INFO, "Serial number:");
        hexDump(serialNumber, sizeof(serialNumber));   // debug: also report chip serial number in hex

        canReadBattery = true;
      } else {
        logger.log(CONFIG, ERROR, "AGSHA204A simpleGetSerialNumber() failed, error %d", rc);
      }
    } else {
      logger.log(CONFIG, ERROR, "ATSHA204A wakeup failed, error %d", rc);
    }

    logger.log(CONFIG, DEBUG, "Put SHA204 chip to sleep");
    sha204.simpleSleep();
  }

  void hexDump(uint8_t *data, uint32_t length) {
    char buffer[3];
    Serial.print("0x");
    for (uint32_t i = 0; i < length; i++) {
      snprintf(buffer, sizeof(buffer), "%02X", data[i]);
      Serial.print(buffer);
    }
    Serial.println();
  }

};   // end class CryptoAuthentication
