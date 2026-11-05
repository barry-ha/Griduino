// Please format this file with clang before check-in to GitHub
/*
  Example program to query and display Serial Number from ATSHA204 crypto chip

  The 6-byte chip serial buffer is correct: The library reads all 9 bytes and 
  returns only the 6 that differ between chips, because SN[0:1] is always 01 23 
  and SN[8] is always EE.

  Chip features used:
    contains a guaranteed unique 72-bit serial number (this is why we have the chip)
    1-sec watchdog timer
    always wake/sleep around any function calls (to reset watchdog)
  Chip features we MIGHT use someday:
    can generate a 32-byte random number and an internally stored nonce
    can generate a random number
    contains 512 bit OTP (One Time Programmable) bits for fixed information

  Version history:
            2026-11-04 bwh written

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

*/

#if defined(SAMD_SERIES)
#warning----- Compiling for Arduino Feather M4 Express -----
#elif defined(ARDUINO_ARCH_ESP32)
#warning----- Compiling for ESP32 S3 (pcb v16+) -----
#else
#error Hardware platform unknown.
#endif

#include <Arduino.h>
#include <Wire.h>
#include <sha204_i2c.h>         // MengGuang ATSHA204, github.com/mengguang/atsha204_i2c
#include <Adafruit_GFX.h>       // Core graphics display library
#include <Adafruit_ILI9341.h>   // TFT display, Adafruit: https://github.com/adafruit/Adafruit_ILI9341
#include "hardware.h"           // Griduino pin definitions

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE    "Display Serial Number"
#define PROGRAM_VERSION  "v1.15"
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ---------- TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ------------ definitions
const int howLongToWait = 8;   // max number of seconds before using Serial port to console

// ----- screen layout
// When using default system fonts, screen pixel coordinates will identify top left of character cell
const int xLabel = 8;      // indent labels, slight margin on left edge of screen
#define yRow1 6            // title: "Griduino Altimeter"
#define yRow2 yRow1 + 34   // more text

// ----- Griduino color scheme
// RGB 565 true color: https://chrishewett.com/blog/true-rgb565-colour-picker/
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND 0x00A          // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cTEXTCOLOR  ILI9341_CYAN   // 0, 255, 255
#define cLABEL      ILI9341_GREEN
#define cVALUE      ILI9341_YELLOW

void hexDump(uint8_t *data, uint32_t length) {
  char buffer[3];
  Serial.print("0x");
  for (uint32_t i = 0; i < length; i++) {
    snprintf(buffer, sizeof(buffer), "%02X", data[i]);
    Serial.print(buffer);
  }
  Serial.println();
}

int char2int(char input) {
  if (input >= '0' && input <= '9')
    return input - '0';
  if (input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if (input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  return 0;
}

// This function assumes src to be a zero terminated sanitized string with
// an even number of [0-9a-f] characters, and target to be sufficiently large
void hex2bin(const char *src, uint8_t *target) {
  while (*src && src[1]) {
    *(target++) = char2int(*src) * 16 + char2int(src[1]);
    src += 2;
  }
}

// Attention: we need to set BUFFER_LENGTH to at least 64 in Wire.h
atsha204Class sha204;

// ========== splash screen helpers ============================
void startSplashScreen() {
  clearScreen();   // clear screen

  tft.setTextSize(2);
  // clang-format off
  tft.setCursor(xLabel, yRow1);      tft.setTextColor(cTEXTCOLOR);  tft.print(PROGRAM_TITLE);
  tft.setCursor(xLabel, yRow2);      tft.setTextColor(cLABEL);      tft.print(PROGRAM_VERSION); tft.print("  "); tft.print(PROGRAM_LINE1);
  tft.setCursor(xLabel, yRow2 + 20);                                tft.print(PROGRAM_COMPILED);
  // clang-format on
}

void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connection to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong * 1000;
  while (millis() < targetTime) {
    if (Serial)
      break;
  }
}

//=========== setup ============================================
void setup() {

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);   // set backlight to full brightness

  // ----- init TFT display
  tft.begin();          // initialize TFT display
  tft.setRotation(1);   // 1=landscape (default is 0=portrait)
  clearScreen();        // note that "begin()" does not clear screen

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);           // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println(PROGRAM_LINE1 ", " PROGRAM_LINE2);    // Report the authors
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

  startSplashScreen();

  // Remember to set BUFFER_LENGTH to at 64 in Wire.h
  Wire.begin();
}

// input:  6 bytes uint
// output: updated message text
void getGriduinoSerno(uint8_t chip[6], char* msg) {
  strcpy(msg, "pcb v16 s/n 086");
}

//=========== main work loop ===================================
void loop() {
  uint8_t chipSerialNumber[6] = {0};   // stays all-zero if the read fails

  Serial.print("\nGet Chip Serial: ");
  uint8_t rc = serialNumberExample(chipSerialNumber);

  // format the real chip serial number as "0x" + 12 hex digits
  char hex[3 + 2 * 6];   // "0x" + 12 digits + NUL
  strcpy(hex, "0x");
  for (int i = 0; i < 6; i++) {
    snprintf(hex + 2 + 2 * i, 3, "%02X", chipSerialNumber[i]);
  }

  // two-argument setTextColor() paints the background too, so each redraw
  // erases the previous text; trailing spaces erase any longer old value
  tft.setCursor(xLabel, yRow2 + 60);
  tft.setTextColor(cLABEL, cBACKGROUND);
  tft.print("Chip s/n: ");
  if (rc == SHA204_SUCCESS) {
    tft.setTextColor(cVALUE, cBACKGROUND);
    tft.print(hex);
  } else {
    tft.setTextColor(ILI9341_RED, cBACKGROUND);
    tft.print("read error    ");
  }

  char msg[128] = "blank";
  if (rc == SHA204_SUCCESS) {
    Serial.print("Look up Griduino public serno: ");
    getGriduinoSerno(chipSerialNumber, msg);
  } else {
    strcpy(msg, "unknown        ");   // don't derive a serno from a failed read
  }
  Serial.println(msg);   // console
  tft.setCursor(xLabel, yRow2 + 80);
  tft.setTextColor(cLABEL, cBACKGROUND);
  tft.print("Griduino: ");
  tft.setTextColor(rc == SHA204_SUCCESS ? cVALUE : ILI9341_RED, cBACKGROUND);
  tft.print(msg);

  delay(4000);
}

// input:   pointer to uint8_t serialNumber[6]
// returns: 0=success
uint8_t serialNumberExample(uint8_t *serialNumber) {

  sha204.simpleWakeup();
  uint8_t rc = sha204.simpleGetSerialNumber(serialNumber);

  if (rc == SHA204_SUCCESS) {
    hexDump(serialNumber, 6);
  } else {
    Serial.print("sha204.simpleGetSerialNumber() failed, rc = ");
    Serial.println(rc);
  }

  sha204.simpleSleep();
  return rc;
}
