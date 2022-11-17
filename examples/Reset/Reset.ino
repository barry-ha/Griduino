// Please format this file with clang before check-in to GitHub
/*
  Reset
  Blinks the onboard red LED repeatedly.
  After a few blinks, it resets the microcontroller.
  You should watch the serial monitor for status messages.

  Source:
  - Original example blink code is in the public domain.
  - good: https://forum.arduino.cc/t/soft-reset-and-arduino/367284/5
  - fail: https://www.instructables.com/two-ways-to-reset-arduino-in-software/
  - fail: https://www.theengineeringprojects.com/2015/11/reset-arduino-programmatically.html
 */

// Pin 13 has an LED connected on most Arduino boards, including Feather M4.
// Pin 11 has the LED on Teensy 2.0
// Pin 6  has the LED on Teensy++ 2.0
// Pin 13 has the LED on Teensy 3.0
int led = 13;


// the setup routine runs once when you press reset:
void setup() {
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);

  // allow time for Windows to recognize USB connection
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println();
  Serial.println("Software Reset Demo");
  Serial.println("Compiled " __DATE__ " " __TIME__);
  Serial.println(__FILE__);
}

void loop() {
  // flash slowly a few times
  for (int ii = 25; ii > 0; ii--) {
    Serial.print(ii);
    Serial.print("  ");
    digitalWrite(13, !digitalRead(13));
    delay(500);
  }

  Serial.println();
  Serial.println("Now we are Resetting Arduino Programmatically");

  reboot_wdt();
}

// ----- success: both Feather M4 and RP2040 will reset as desired
// use library manager to install Adafruit_SleepyDog
#include <Adafruit_SleepyDog.h>  // https://github.com/adafruit/Adafruit_SleepyDog
void reboot_wdt() {
  Watchdog.enable(15);  // milliseconds timeout
  while (1) {}
}

// ----- fail: the Feather M4 does not reconnect to USB
void reboot_function() {
  void (*resetFunction)(void) = 0;  //declare reset function at address 0
  resetFunction();                  // does not return
}