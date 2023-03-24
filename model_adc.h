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
  int coinBattery;   // measured coin battery ADC sample (0..1024)
  const float voltsPerSample = (3.3 / 1023.0);  // PCB v6+ circuit's reference voltage is Vcc = 3.3 volts

public:
  // Griduino v7 uses Analog input pin A1 to measure 3v coin battery
  BatteryVoltage(int inputPin = A1) {}

  void setup() {
  }

  float getCoinBatteryVoltage() {
    coinBattery       = analogRead(A1);
    float coinVoltage = coinBattery * voltsPerSample;
    return coinVoltage;
  }

};   // end class BatteryVoltage
