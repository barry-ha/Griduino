#pragma once   // Please format this file with clang before check-in to GitHub

// ---------- Hardware Wiring ----------
/*                                Arduino       Adafruit
  ___Label__Description______________Mega_______Feather M4__________Resource____
TFT Power:
   GND  - Ground                  - ground      - Gnd  (J2 Pin 13)
   VIN  - VCC                     - 5v          - Vusb (J5 Pin 10)
TFT SPI:
   SCK  - SPI Serial Clock        - Digital 52  - SCK (J2 Pin 6)  - uses hardware SPI
   MISO - SPI Master In Slave Out - Digital 50  - MI  (J2 Pin 4)  - uses hardware SPI
   MOSI - SPI Master Out Slave In - Digital 51  - MO  (J2 Pin 5)  - uses hardware SPI
   CS   - SPI Chip Select         - Digital 10  - D5  (Pin 3 J5)
   D/C  - SPI Data/Command        - Digital  9  - D12 (Pin 8 J5)
TFT MicroSD:
   CD   - SD Card Detection       - Digital  8  - D10 (Pin 6 J5)
   CCS  - SD Card Chip Select     - Digital  7  - D11 (Pin 7 J5)
TFT Backlight:
   BL   - Backlight               - Digital 12  - D4  (J2 Pin 1)  - uses hardware PWM
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
BMP388/BMP390:
   CS   - barometer chip select   - Digital 13  - D13             - shared with onboard LED
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

    The "Ultimate GPS" LED indicates status:
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

#elif defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
// Ref: https://arduino-pico.readthedocs.io/en/latest/index.html
// Adafruit Feather_RP2040 pin definitions
// To compile for Feather_RP2040, install "additional boards manager"
// https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

#define TFT_BL 4    // TFT backlight
#define TFT_CS 5    // TFT chip select pin
#define TFT_DC 12   // TFT display/command pin
#define BMP_CS 13   // BMP388 sensor, chip select

#define SD_CD  10   // SD card detect pin - Feather
#define SD_CCS 11   // SD card select pin - Feather

#else
#warning You need to define pins for your SPI bus hardware
#endif

// ------- TFT 4-Wire Resistive Touch Screen configuration parameters
#define XP_XM_OHMS 0   // Set to zero to receive raw pressure measurements

#define START_TOUCH_PRESSURE 200   // Minimum pressure threshold considered start of "press"
#define END_TOUCH_PRESSURE   50    // Maximum pressure threshold required before end of "press"

#define X_MIN_OHMS 100   // Expected range on touchscreen's X-axis readings
#define X_MAX_OHMS 900
#define Y_MIN_OHMS 110   // Expected range on touchscreen's Y-axis readings
#define Y_MAX_OHMS 860

#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
// ---------- Touch Screen pins - Adafruit Feather RP2040
#define PIN_XM A2   // Touchscreen X- must be an analog pin, use "An" notation
#define PIN_YP A3   // Touchscreen Y+ must be an analog pin, use "An" notation
#define PIN_XP 24   // Touchscreen X+ can be a digital pin
#define PIN_YM 25   // Touchscreen Y- can be a digital pin

// ---------- Audio output pins
//  #define DAC_PIN     0   // do not use - RP2040 has no DAC
//  #define PIN_SPEAKER 0   // do not use - RP2040 has no DAC
#else
// ---------- Touch Screen pins - Feather M4
#define PIN_XP      A3     // Touchscreen X+ can be a digital pin
#define PIN_XM      A4     // Touchscreen X- must be an analog pin, use "An" notation
#define PIN_YP      A5     // Touchscreen Y+ must be an analog pin, use "An" notation
#define PIN_YM      9      // Touchscreen Y- can be a digital pin
// ---------- Audio output pins
#define DAC_PIN     DAC0   // onboard DAC0 == pin A0
#define PIN_SPEAKER DAC0   // uses DAC
#endif

// ---------- Battery voltage sensor
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
#define BATTERY_ADC A1
#endif

// ---------- Feather RP2040 onboard led
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
#define RED_LED 25   // diagnostics RED LED
#else
#define RED_LED 13   // diagnostics RED LED
#endif

// ---------- neopixel
#define NUMPIXELS 1   // Feather M4 has one NeoPixel on board
// define PIN_NEOPIXEL 8     // already defined in Feather's board variant.h

// ---------- Digital potentiometer
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
// todo - Griduino PCB v7 uses volume control on I2C
#else
// Griduino PCB v7 uses volume control DS1804 on SPI
#define PIN_VCS A1   // volume chip select
#define PIN_VUD A2   // volume up/down
#endif

// Adafruit ItsyBitsy M4 Express potentiometer wiring
#if defined(ADAFRUIT_ITSYBITSY_M4_EXPRESS)
#define PIN_VINC 2   // volume increment, ItsyBitsy M4 Express
#else
#define PIN_VINC 6   // volume increment, Feather M4 Express
#endif
