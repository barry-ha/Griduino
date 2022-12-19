// Please format this file with clang before check-in to GitHub
/*
  File: PWM_rp2040_test

  Use RP2040 state machine for PWM output
  Test the PWM function on all the pins of Raspberry Pi Pico
  This example code is in the public domain.

  Version history:
            2022-12-19 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA
  
  Based on: https://pastebin.com/0BgA5LmT

  Test results:
           + -----RP2040------+
      na   | SDA          D4  | fail
      na   | SCL          Tx  | na
      fail | 5            Rx  | na
      fail | 6            MI  | na
      PWM  | 9            MO  | na
      PWM  | 10           SCK | na
      PWM  | 11           25  | PWM
      PWM  | 12           24  | PWM
      PWM  | 13           A3  | PWM
      na   | USB          A2  | PWM
      na   | En           A1  | PWM
      na   | Bat          A0  | PWM
           | -            Gnd | na
           | -            3.3 | na
           | -            3.3 | na
           | -            Rst | na
           + -----------------+
  where:
    fail = no PWM waveform
    PWM  = working PWM output
    na   = not applicable
*/

#define PWM_PIN 13  // edit this line and recompile to repeat test \
                    // connect oscilloscope to PWM_PIN to see result \
                    // set horiz timebase to 10 usec

// ============== constants ====================================
#define DUTY_100_PCT 64000
#define DUTY_75_PCT 48000
#define DUTY_50_PCT 32767
#define DUTY_25_PCT 16000

// ============== globals ======================================
uint slice_num;  // PWM slice: 0..7
uint channel;    // PWM channel: 0 for A, 1 for B

// ============== helpers ======================================
#include "hardware/pwm.h"
int set_pwm_duty(uint slice, uint channel, uint top, uint duty) {
  uint cc = duty * (top + 1) / 65535;
  pwm_set_chan_level(slice, channel, cc);
  pwm_set_enabled(slice, true);
  return 0;
}

//=========== setup ============================================
void setup() {

  slice_num = pwm_gpio_to_slice_num(PWM_PIN);
  channel = pwm_gpio_to_channel(PWM_PIN);
  gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
  pwm_set_clkdiv(slice_num, 2.0f);  // Div the sys clock by 2 yields 53 kHz

  // Set the PWM counter wrap value to reset on
#define PWM_1KHZ 62500       // Wrap val for 1khz at SYSCLK / 2
  uint top = PWM_1KHZ / 50;  // Set for 50khz freq - Close to it anyway.
  pwm_set_wrap(slice_num, top);

  set_pwm_duty(slice_num, channel, top, DUTY_25_PCT);
}

//=========== main work loop ===================================
void loop() {
}
