#pragma once   // Please format this file with clang before check-in to GitHub

// ---------- Hardware Wiring ----------
/*                                Arduino       Adafruit
  ___Label__Description______________Mega_______Feather M4__________Resource____
TFT Power:
   GND  - Ground                  - ground      - J2 Pin 13
   VIN  - VCC                     - 5v          - Pin 10 J5 Vusb
TFT SPI:
   SCK  - SPI Serial Clock        - Digital 52  - SCK (J2 Pin 6)  - uses hardw SPI
   MISO - SPI Master In Slave Out - Digital 50  - MI  (J2 Pin 4)  - uses hardw SPI
   MOSI - SPI Master Out Slave In - Digital 51  - MO  (J2 Pin 5)  - uses hardw SPI
   CS   - SPI Chip Select         - Digital 10  - D5  (Pin 3 J5)
   D/C  - SPI Data/Command        - Digital  9  - D12 (Pin 8 J5)
TFT MicroSD:
   CD   - SD Card Detection       - Digital  8  - D10 (Pin 6 J5)
   CCS  - SD Card Chip Select     - Digital  7  - D11 (Pin 7 J5)
TFT Backlight:
   BL   - Backlight               - Digital 12  - D4  (J2 Pin 1)  - uses hardw PWM
TFT Resistive touch:
   X+   - Touch Horizontal axis   - Digital  4  - A3  (Pin 4 J5)
   X-   - Touch Horizontal        - Analog  A3  - A4  (J2 Pin 8)  - uses analog A/D
   Y+   - Touch Vertical axis     - Analog  A2  - A5  (J2 Pin 7)  - uses analog A/D
   Y-   - Touch Vertical          - Digital  5  - D9  (Pin 5 J5)
TFT No connection:
   3.3  - 3.3v output             - n/c         - n/c
   RST  - Reset                   - n/c         - n/c
   IM0/3- Interface Control Pins  - n/c         - n/c
GPS:
   VIN  - VCC                     - 5v          - Vin
   GND  - Ground                  - ground      - ground
   >RX  - data into GPS           - TX1 pin 18  - TX  (J2 Pin 2)  - uses hardware UART
   <TX  - data out of GPS         - RX1 pin 19  - RX  (J2 Pin 3)  - uses hardware UART
Audio Out:
   DAC0 - audio signal            - n/a         - A0  (J2 Pin 12) - uses onboard digital-analog converter
Digital potentiometer:
   VINC - volume increment        - n/a         - D6
   VUD  - volume up/down          - n/a         - A2
   CS   - volume chip select      - n/a         - A1
On-board lights:
   LED  - red activity led        - Digital 13  - D13             - reserved for onboard LED
   NP   - NeoPixel                - n/a         - D8              - reserved for onboard NeoPixel
*/

/* "Ultimate GPS" pin wiring is connected to a dedicated hardware serial port
    available on an Arduino Mega, Arduino Feather and others.

    The GPS' LED indicates status:
        1-sec blink = searching for satellites
        15-sec blink = position fix found
*/

// TFT display and SD card share the hardware SPI interface, and have
// separate 'select' pins to identify the active device on the bus.
#if defined(SAMD_SERIES)
// Adafruit Feather M4 Express pin definitions
// To compile for Feather M0/M4, install "additional boards manager"
// https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup

#define TFT_BL 4    // TFT backlight
#define TFT_CS 5    // TFT chip select pin
#define TFT_DC 12   // TFT display/command pin
#define BMP_CS 13   // BMP388 sensor, chip select

#define SD_CD  10   // SD card detect pin - Feather
#define SD_CCS 11   // SD card select pin - Feather

#elif defined(ARDUINO_AVR_MEGA2560)
#define TFT_BL 6    // TFT backlight
#define SD_CCS 7    // SD card select pin - Mega
#define SD_CD  8    // SD card detect pin - Mega
#define TFT_DC 9    // TFT display/command pin
#define TFT_CS 10   // TFT chip select pin
#define BMP_CS 13   // BMP388 sensor, chip select

#else
#warning You need to define pins for your hardware

#endif

// ---------- Touch Screen
#define TOUCHPRESSURE 200   // Minimum pressure threshhold considered an actual "press"
#define PIN_XP        A3    // Touchscreen X+ can be a digital pin
#define PIN_XM        A4    // Touchscreen X- must be an analog pin, use "An" notation
#define PIN_YP        A5    // Touchscreen Y+ must be an analog pin, use "An" notation
#define PIN_YM        9     // Touchscreen Y- can be a digital pin

// ------- TFT 4-Wire Resistive Touch Screen configuration parameters
// For touch point precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
#define TOUCHPRESSURE 200   // Minimum pressure threshhold considered an actual "press"
#define X_MIN_OHMS    240   // Expected range of measured X-axis readings
#define X_MAX_OHMS    800
#define Y_MIN_OHMS    320   // Expected range of measured Y-axis readings
#define Y_MAX_OHMS    760
#define XP_XM_OHMS    310   // Resistance in ohms between X+ and X- to calibrate touch pressure
                            // measure this with an ohmmeter while Griduino turned off

// ---------- Feather's onboard lights
// efine PIN_LED 13         // already defined in Feather's board variant.h
// efine PIN_NEOPIXEL 8     // already defined in Feather's board variant.h
#define NUMPIXELS 1   // Feather M4 has one NeoPixel on board
