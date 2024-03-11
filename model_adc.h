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

// ========== class BatteryVoltage ==================================
class BatteryVoltage {
private:
  int inputPin;
  int coinBattery;                               // measured coin battery ADC sample (0..1024)
  const float voltsPerSample = (3.3 / 1023.0);   // PCB v6+ circuit's reference voltage is Vcc = 3.3 volts

public:
  // Griduino v7 uses Analog input pin A1 to measure 3v coin battery
  BatteryVoltage(int inputPin = A1) {}

  void setup() {
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
    const float analogRef     = 3.3;    // analog reference voltage
    const uint16_t analogBits = 1024;   // ADC resolution is 10 bits = 2^10 = 1024

    int coin_adc       = analogRead(BATTERY_ADC);
    float coin_voltage = (float)coin_adc * analogRef / analogBits;
    // return GOOD_BATTERY_MINIMUM - 0.1;     // debug - show yellow icon
    return WARNING_BATTERY_MINIMUM - 0.1;   // debug - show red icon
    return coin_voltage;                    // production release
  }

};   // end class BatteryVoltage
