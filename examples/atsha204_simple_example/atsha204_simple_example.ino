#include <Arduino.h>
#include <Wire.h>
#include <sha204_i2c.h>

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

void loadKeyFromHex(const char *hex_key, uint8_t *key) {
  hex2bin(hex_key, key);
}

/* Attention: we need to set BUFFER_LENGTH to at least 64 in Wire.h */
atsha204Class sha204;

void setup() {
  //Remember to set BUFFER_LENGTH to at 64 in Wire.h
  Wire.begin();
  Serial.begin(115200);
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
  Serial.println("\n1. Wake up device.");
  if (wakeupDevice()) {
    Serial.print("Get Serial Number:");
    serialNumberExample();
  }
  sleepDevice();
  delay(3000);

  Serial.println("2. Wake up device.");
  if (wakeupDevice()) {
    Serial.println("Call random:");
    randomExample();
  }
  sleepDevice();
  delay(3000);

  Serial.println("3. Wake up device.");
  if (wakeupDevice()) {
    Serial.println("Sending a MAC Challenge.");
    macChallengeExample();
  }
  sleepDevice();
  delay(3000);
}

uint8_t randomExample() {
  uint8_t response[32];
  uint8_t returnValue;
  returnValue = sha204.simpleGetRandom(response);
  if (returnValue != SHA204_SUCCESS) {
    Serial.println("simpleGetRandom failed.");
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
    Serial.println("simpleGetSerialNumber failed.");
    return returnValue;
  }
  hexDump(serialNumber, sizeof(serialNumber));

  return returnValue;
}

uint8_t macChallengeExample() {
  static uint32_t n = 0;
  uint8_t mac[32];
  uint8_t challenge[MAC_CHALLENGE_SIZE] = { 0 };
  sprintf((char *)challenge, "Are you OK? %lu", n++);
  Serial.print("Challenge: ");
  Serial.println((char *)challenge);

  uint8_t ret_code = sha204.simpleMac(challenge, mac);
  if (ret_code != SHA204_SUCCESS) {
    Serial.println("simpleMac failed.");
    return ret_code;
  }
  Serial.print("MAC:\n");
  hexDump(mac, sizeof(mac));

  uint8_t key[32];
  //Change your key here.
  const char *hex_key = "A9CD7F1B6688159B54BBE862F638FF9D29E0FA5F87C69D27BFCD007814BA69C9";
  loadKeyFromHex(hex_key, key);
  uint8_t mac_offline[32];
  ret_code = sha204.simpleMacOffline(challenge, mac_offline, key);
  if (ret_code != SHA204_SUCCESS) {
    Serial.println("simpleMacOffline failed.");
    return ret_code;
  }
  Serial.print("MAC Offline:\n");
  hexDump(mac_offline, sizeof(mac_offline));

  if (memcmp(mac, mac_offline, sizeof(mac)) != 0) {
    Serial.println("MAC mismatch: device key does not match, authentication FAILED.");
    return SHA204_FUNC_FAIL;
  }
  Serial.println("MAC match: device authenticated.");
  return ret_code;
}
