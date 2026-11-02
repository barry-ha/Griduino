// Please format this file with clang before check-in to GitHub
/*
  Example program to verify basic functionality

  Version history:
            2026-11-01 code reviewed by Claude
            2025-12-20 copied from driver library
*/

#include <Arduino.h>
#include <Wire.h>
#include <sha204_i2c.h>  // MengGuang ATSHA204, github.com/mengguang/atsha204_i2c

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE "\nATSHA Basic Example"
#define PROGRAM_VERSION "v1.15"
#define PROGRAM_LINE1 "Barry K7BWH"
#define PROGRAM_LINE2 "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ------------ definitions
const int howLongToWait = 4;  // max number of seconds before using Serial port to console

uint8_t randomExample();
uint8_t serialNumberExample();
uint8_t macChallengeExample();

void hexDump(uint8_t *data, uint32_t length) {
  char buffer[3];
  Serial.print("0x");
  for (uint32_t i = 0; i < length; i++) {
    snprintf(buffer, sizeof(buffer), "%02X", data[i]);
    Serial.print(buffer);
  }
  Serial.println();
}

// The driver library's README says to set BUFFER_LENGTH to at least 64 in Wire.h.
// That is not needed on Griduino: the Adafruit SAMD core's Wire buffers are already 256 bytes.
atsha204Class sha204;

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
  Wire.begin();

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);          // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);  // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println(PROGRAM_LINE1 ", " PROGRAM_LINE2);     // Report the authors
  Serial.println("Compiled " PROGRAM_COMPILED);         // Report our compiled date
  Serial.println(__FILE__);                             // Report our source code file name
}

bool wakeupDevice() {
  uint8_t ret_code = sha204.simpleWakeup();
  if (ret_code != SHA204_SUCCESS) {
    Serial.print("simpleWakeup failed, is the ATSHA204 connected? Error 0x");
    Serial.println(ret_code, HEX);
    return false;
  }
  return true;
}

void sleepDevice() {
  Serial.println("Put device to sleep mode.");
  uint8_t ret_code = sha204.simpleSleep();
  if (ret_code != SHA204_SUCCESS) {
    Serial.print("simpleSleep failed. Error 0x");
    Serial.println(ret_code, HEX);
  }
}

void loop() {
  uint8_t rc;

  Serial.println("\n1. Wake up device for serial number.");
  // If wakeup fails, skip sleep too: the chip's watchdog returns it to sleep by itself
  if (wakeupDevice()) {
    Serial.print("Get Serial Number:");
    rc = serialNumberExample();
    sleepDevice();
  }
  delay(3000);

  Serial.println("2. Wake up device for random number.");
  if (wakeupDevice()) {
    Serial.println("Call random:");
    rc = randomExample();
    sleepDevice();
  }
  delay(3000);

  Serial.println("3. Wake up device for MAC challenge.");
  if (wakeupDevice()) {
    Serial.println("Sending a MAC Challenge.");
    rc = macChallengeExample();
    sleepDevice();
  }
  delay(3000);
}

uint8_t randomExample() {
  uint8_t response[32];
  uint8_t returnValue;
  returnValue = sha204.simpleGetRandom(response);
  if (returnValue != SHA204_SUCCESS) {
    Serial.print("simpleGetRandom failed. Error 0x");
    Serial.println(returnValue, HEX);
    return returnValue;
  }
  hexDump(response, sizeof(response));
  return returnValue;
}

uint8_t serialNumberExample() {
  uint8_t serialNumber[6];
  uint8_t returnValue;

  returnValue = sha204.simpleGetSerialNumber(serialNumber);
  if (returnValue != SHA204_SUCCESS) {
    Serial.print("simpleGetSerialNumber failed. Error 0x");
    Serial.println(returnValue, HEX);
    return returnValue;
  }
  hexDump(serialNumber, sizeof(serialNumber));

  return returnValue;
}

uint8_t macChallengeExample() {
  static uint32_t n = 0;
  uint8_t mac[32];
  uint8_t challenge[MAC_CHALLENGE_SIZE] = { 0 };
  snprintf((char *)challenge, sizeof(challenge), "Are you OK? %lu", (unsigned long)n++);
  Serial.print("Challenge: ");
  Serial.println((char *)challenge);

  uint8_t ret_code = sha204.simpleMac(challenge, mac);
  if (ret_code != SHA204_SUCCESS) {
    Serial.print("simpleMac failed. Error 0x");
    Serial.println(ret_code, HEX);
    return ret_code;
  }
  Serial.print("MAC:\n");
  hexDump(mac, sizeof(mac));

  // Change your key here. It must match the secret key already programmed into the chip's
  // slot 0, otherwise every MAC challenge reports a mismatch. A new, unconfigured chip will
  // not match this demo key, so a mismatch does not necessarily mean the hardware is bad.
  // Storing the secret key in the sketch like this is for demonstration only. Never put a
  // real key in source code that is pushed to GitHub, or anyone can forge MAC responses.
  static uint8_t key[32] = {
    0xA9, 0xCD, 0x7F, 0x1B, 0x66, 0x88, 0x15, 0x9B,
    0x54, 0xBB, 0xE8, 0x62, 0xF6, 0x38, 0xFF, 0x9D,
    0x29, 0xE0, 0xFA, 0x5F, 0x87, 0xC6, 0x9D, 0x27,
    0xBF, 0xCD, 0x00, 0x78, 0x14, 0xBA, 0x69, 0xC9
  };
  uint8_t mac_offline[32];
  ret_code = sha204.simpleMacOffline(challenge, mac_offline, key);
  if (ret_code != SHA204_SUCCESS) {
    Serial.print("simpleMacOffline failed. Error 0x");
    Serial.println(ret_code, HEX);
    return ret_code;
  }
  Serial.print("MAC Offline:\n");
  hexDump(mac_offline, sizeof(mac_offline));

  if (memcmp(mac, mac_offline, sizeof(mac)) != 0) {
    Serial.println("MAC mismatch: device key does not match, authentication FAILED.");
    Serial.println("(Check that key[] matches the key programmed into the chip's slot 0.)");
    return SHA204_FUNC_FAIL;
  }
  Serial.println("MAC match: device authenticated.");
  return ret_code;
}
