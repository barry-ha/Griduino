// Please format this file with clang before check-in to GitHub
/*
  File:     LittleFS_basic.ino

  Version history:
            2022-12-27 created

  Purpose:  Simplest possible example of basic LittleFS functions on RP2040.
            This listens for serial console commands to read, write and delete
            files in flash memory of Adafruit Feather RP2040, and echo results.

  LittleFS docs:
            https://github.com/earlephilhower/arduino-pico-littlefs-plugin
            https://github.com/littlefs-project/littlefs/blob/master/SPEC.md
            https://github.com/littlefs-project/littlefs/blob/master/DESIGN.md

  Inspired by:
            https://www.hackster.io/Neutrino-1/littlefs-read-write-delete-using-esp8266-and-arduino-ide-867180

  License:  GNU General Public License
*/

#include <Wire.h>
#include "LittleFS.h"

// constants
#define PROGRAM_TITLE    "Little FS Demo"
#define PROGRAM_VERSION  "v1.12 rp2040"
#define PROGRAM_COMPILED __DATE__ " " __TIME__
#define PROGRAM_FILE     __FILE__
#define DATA_FILE        "/SavedFile.txt"

// ============ file helpers ===================================

void readData() {
  File file = LittleFS.open(DATA_FILE, "r");
  if (!file) {
    Serial.println("No Saved Data!");
    return;
  }

  Serial.print(DATA_FILE);
  Serial.print(" contains: ");
  while (file.available()) {
    int c = file.read();
    Serial.print(char(c));
  }
  file.close();
  Serial.println();
}

void writeData(String data) {
  // Open the file for writing, replacing contents
  File file = LittleFS.open(DATA_FILE, "w");
  file.print(data);
  file.close();
  delay(1);
  Serial.println("Write successful");
  Serial.print("Data Saved: ");
  Serial.println(data);
}

void deleteData() {
  LittleFS.remove(DATA_FILE);
  Serial.println("Data Deleted");
}

// ----- Flash FS helper -----
int openFlash() {
  // returns 1=success, 0=failure

  if (!LittleFS.begin()) {   // Start LittleFS
    Serial.println("An Error has occurred while mounting LittleFS");
    Serial.println("Mounting Error");
    delay(500);
    return 0;  // indicate error
  }
  Serial.println(". Mounted flash filesystem");
  return 1;
  }
}

//=========== setup ============================================
void setup() {
  Serial.begin(115200);   // init for debugging in the Arduino IDE
  while (!Serial) {       // Wait for console in the Arduino IDE
    yield();
  }

  Serial.println(PROGRAM_TITLE);
  Serial.println(PROGRAM_COMPILED);
  Serial.println(PROGRAM_FILE);

  delay(1000);

  openflash();

  Serial.print(DATA_FILE);
  if (LittleFS.exists(DATA_FILE)) {
    Serial.println(" already exists in the file system at startup");
  } else {
    Serial.println(" was not found in the file system at startup");
  }

  readData();   // Read and show saved data from file
}

//=========== main work loop ===================================
void loop() {
  // Take input from user on serial monitor
  if (Serial.available()) {
    String data = Serial.readString();
    Serial.print("You typed: ");
    Serial.println(data);

    if (data == "D") {   // Delete file
      deleteData();
      Serial.println("File deleted!");
    } else if (data == "R") {   // Read file
      readData();
    } else {   // Anything else: Write to file
      Serial.println("Writing Data...");
      writeData(data);
      Serial.println("done Writing Data!");
    }
  }
}
