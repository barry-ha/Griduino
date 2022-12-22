// Please format this file with clang before check-in to GitHub
/*
  File: PWM_rp2040_basic

  Example of basic PWM programmiong for Adafruit Feather RP2040.

  The RP2040 PWM block has 8 identical slices. Each slice can drive 
  two PWM output signals, or measure the frequency or duty cycle of 
  an input signal.  This gives a total of up to 16 controllable PWM 
  outputs. All 30 GPIO pins can be driven by the PWM block.

  Version history:
            2022-12-22 created
            Written by Dr. Benjamin Bird
            Adapted by Barry Hansen K7BWH

  Requires:
            Library by Khoi Hoang https://github.com/khoih-prog/RP2040_PWM
            (do not use MBED_PR2040_PWM library)
            Licensed under MIT license
*/

#include "RP2040_PWM.h"

// ============== constants ====================================
//efine LED_PIN 6   // Griduino TFT Backlight = GPIO6 = D4
#define LED_PIN LED_BUILTIN  // RP2040 onboard red LED, pin 13

const float frequency = 8;   // f > 7 Hz
const float dutyCycle = 20;  // 0..100

// ============== globals ======================================
// instantiate PWM object, save default settings (does not initialize hardware)
RP2040_PWM PWM_Instance = RP2040_PWM(LED_PIN, frequency, dutyCycle);

//=========== setup ============================================
void setup() {
  PWM_Instance.setPWM();  // init hardware according to ctor settings
}

//=========== main work loop ===================================
void loop() {
  PWM_Instance.setPWM(LED_PIN, 10000, dutyCycle);  //
  delay(5000);

  PWM_Instance.setPWM(LED_PIN, frequency, dutyCycle);
  delay(5000);
}
