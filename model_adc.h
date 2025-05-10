#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     morse_adc.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Measure the voltage on the coin battery that powers
            the Quectel GPS. If the battery falls below 1.6v then
            the GPS can no longer acquire satellites.

  Note:     PCB v7+ has wiring to measure battery voltage.
            PCB v4 has no such sensor.

*/
#include "model_crypto.h"

// ========== extern ===========================================
extern CryptoAuthentication pcb;   // Griduino.ino / model_crypto.h
extern Logger logger;              // Griduino.ino

// ========== class BatteryVoltage ==================================
class BatteryVoltage {
private:
  const int inputPin         = A2;
  const float analogRef      = 3.3;      // ADC reference voltage = Vcc = 3.3 volts
  const float analogBits     = 1024.0;   // ADC resolution is 10 bits = 2^10 = 1024
  const float voltsPerSample = (analogRef / analogBits);

public:
  // Griduino v7 uses Analog input pin As to measure 3v coin battery
  BatteryVoltage() {}   // ctor

  void begin() {
    // figure out if this hardware can measure battery voltage
    // (PCB v4 returns 0.0v, PCB v7+ returns 1.0-3.3v)
    if (pcb.canReadBattery) {
      float coin_voltage = readCoinBatteryVoltage();
      logger.logFloat(BATTERY, INFO, "Coin battery = %s volts", coin_voltage, 3);
    }
  }

  uint16_t getBatteryColor(float v) {
    if (v >= GOOD_BATTERY_MINIMUM) {
      return ILI9341_GREEN;
    } else if (v >= WARNING_BATTERY_MINIMUM) {
      return ILI9341_YELLOW;
    } else {
      return ILI9341_RED;
    }
  }

  float readCoinBatteryVoltage() {

    int coin_adc       = analogRead(BATTERY_ADC);
    float coin_voltage = (float)coin_adc * voltsPerSample;
    logger.log(BATTERY, DEBUG, "Coin battery ADC sample = %d", coin_adc);
    return coin_voltage;   // production release
    // return GOOD_BATTERY_MINIMUM - 0.1;     // debug - show yellow icon
    // return WARNING_BATTERY_MINIMUM - 0.1;   // debug - show red icon
  }

};   // end class BatteryVoltage
