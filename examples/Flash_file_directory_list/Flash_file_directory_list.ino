/*
  File: Flash_file_directory_list.ino
  
  Lists the contents of Feather M4's onboard 2MB Quad-SPI Flash chip
  This example ignores the MicroSD Card slot on the ILI9341 TFT Display
  and ONLY examines the file system on the 2MB Flash memory.

  Author:   Barry K7BWH, barry@k7bwh.com, Seattle, WA

  Date:     2021-03-16 created, based on old unfinished Griduino_v9\examples\DAC_play_wav_from_SD_v2
            which, in turn, was based on Adafruit's full featured example
            at https://github.com/adafruit/Adafruit_SPIFlash/blob/master/examples/SdFat_full_usage/SdFat_full_usage.ino

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)
            Spec: https://www.adafruit.com/product/3857

*/

#include "SPI.h"                 // Serial Peripheral Interface
#include "Adafruit_ILI9341.h"    // TFT color display library
#include <SdFat.h>               // for FAT file systems on Flash and Micro SD cards
#include <Adafruit_SPIFlash.h>   // for FAT file systems on SPI flash chips

// ---------- Hardware Wiring ----------
// Same as Griduino

// SD card share the hardware SPI interface with TFT display, and have
// separate 'select' pins to identify the active device on the bus.
const int chipSelectPin = 7;
const int chipDetectPin = 8;

// ------- Identity for console
#define PROGRAM_TITLE   "Flash File Directory List"
#define PROGRAM_VERSION "v0.2"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   ""

// ------------ definitions
const int howLongToWait = 8;   // max number of seconds at startup waiting for Serial port to console

// ------------ global scope
Adafruit_FlashTransport_QSPI gFlashTransport;   // Quad-SPI 2MB memory chip
Adafruit_SPIFlash gFlash(&gFlashTransport);     //
FatFileSystem gFatfs;                           // file system object from SdFat

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for all this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong * 1000;
  while (millis() < targetTime) {
    if (Serial)
      break;
  }
}

// ----- Flash FS helper -----
int openFlash() {
  // returns 1=success, 0=failure

  // Initialize flash library and check its chip ID.
  if (!gFlash.begin()) {
    Serial.println("Error, failed to initialize onboard memory.");
    return 0;
  }
  Serial.print(". Flash chip JEDEC ID: 0x");
  Serial.println(gFlash.getJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!gFatfs.begin(&gFlash)) {
    Serial.println("Error, failed to mount filesystem!");
    Serial.println("Was the flash chip formatted with the SdFat_format example?");
    return 0;
  }
  Serial.println(". Mounted SPI flash filesystem");
  return 1;
}
// ----- file helper
//int getFileSize(File handle) {
//  
//}
// ----- console output formatter
void showFile(const char* indent, const int count, const char* filename, const int filesize, const char* comment) {
  char msg[256];
  snprintf(msg, sizeof(msg), "%s%d. %-14s  %d %s", 
                         indent, count, filename, filesize, comment);
  Serial.println(msg);
  delay(20);  // debug, in vcsadre 
}
// ----- iterate files in a folder
void listLevel2(const char* folder) {
  // Open a subdirectory to list all its children (files and folders)
  bool okay = true;   // assume success
  //Serial.print("   Listing children of "); Serial.println(folder);
  File mydir = gFatfs.open(folder);
  if (!mydir) {
    Serial.println("   Error, failed to open subfolder");
  }
  File kid2 = mydir.openNextFile();
  int count = 1;
  while (kid2 && okay) {
    char filename[64];
    kid2.getName(filename, sizeof(filename));
    if (strlen(filename) == 0) {
      Serial.println("   Empty filename, time to return");
      okay = false;
    } else {
      // Print the file name and mention if it's a directory.
      if (kid2.isDirectory()) {
        showFile("   ", count, filename, -1, "(directory)");
      } else {
        int filesize = kid2.size();
        showFile("   ", count, filename, filesize, "");
      }

      // increment loop counters
      kid2 = mydir.openNextFile();
      if (++count > 64) {
        Serial.println("And many more, but I'm stopping here.");
        okay = false;
      }
    }
  }
}
// ----- iterate files at root level
int listFiles() {
  // Open the root folder to list all the children (files and directories).
  int rc = 1;   // assume success
  File testDir = gFatfs.open("/");
  if (!testDir) {
    Serial.println("Error, failed to open root directory!");
    rc = 0;
  } 
  if (!testDir.isDirectory()) {
    Serial.println("Error, expected root to be a directory!");
    rc = 0;
  } else {

    Serial.println("Listing files in the root directory:");
    File child = testDir.openNextFile();
    int count  = 1;
    while (child && rc>0) {
      char filename[64];
      child.getName(filename, sizeof(filename));

      if (strlen(filename) == 0) {
        Serial.println("Empty filename, so time to return");
        rc = 0;
      } else {

        // Print the file name and mention if it's a directory.
        if (child.isDirectory()) {
          showFile("", count, filename, 0, "(directory)");
        } else {
          int filesize = child.size();
          showFile("", count, filename, filesize, "");
        }

        // If it was a directory, list its children
        if (child.isDirectory()) {
          listLevel2(filename);
        }

        // increment loop counters
        child = testDir.openNextFile();
        if (++count > 64) {
          Serial.println("And many more, but I'm stopping here.");
          rc = 0;
        }
      }
    }
  }
  return rc;
}

//=========== setup ============================================
void setup() {

  // ----- init serial monitor
  Serial.begin(115200);           // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " __DATE__ " " __TIME__);   // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

  // ----- look for memory card
  Serial.print("Detecting if FLASH memory is available, using pin ");
  Serial.print(chipDetectPin);
  Serial.println(" ... ");
  pinMode(chipDetectPin, INPUT_PULLUP);               // use internal pullup resistor
  bool isCardDetected = digitalRead(chipDetectPin);   // HIGH = no card; LOW = card detected
  if (isCardDetected) {
    Serial.println(". Success - found a memory chip");
  } else {
    Serial.println(". Failed - no memory chip found");
  }

  // ----- init SD-card on the TFT board
  //Serial.println("Initializing Micro-SD interface...");
  // TODO

  // ----- init FLASH memory chip on the Feather M4 board
  Serial.println("Initializing Flash memory interface...");

  int result = openFlash();   // open file system

  result = listFiles();   // debug - list all files recursively

  // ----- init audio player
  //if (AudioPlayer.begin(sampleRate, NUM_AUDIO_CHANNELS, YOUR_SD_CS) == -1) {
  //  Serial.println("Error: unable to init audio player!");
  //  while(true);
  //}
  //Serial.println(" done.");

  //#define FILE1  "R2D2.wav"
  //AudioPlayer.play(FILE1, 0);

  //Serial.println("Playing file " FILE1 " ...");

  Serial.println("All done.");
}

//=========== main work loop ===================================
void loop() {
  /*
  if (Serial.available()) {
    char c = Serial.read();

    if ( c == 'q') {
      AudioPlayer.play("LaserFire1.wav", 0);
    }
    else if ( c == 'w') {
      AudioPlayer.play("LaserFire1.wav", 1);
    } else if ( c == 'e') {
      AudioPlayer.play("LaserFire2.wav", 2);
    } else if ( c == 'r') {
      AudioPlayer.play("LaserFire2.wav", 3);
    } else if ( c == 't') {
      AudioPlayer.play("R2D2.wav", 3);
    } else if ( c == '1') {
      AudioPlayer.stopChannel(0);
      Serial.println("ch0 off!");
    } else if ( c == '2') {
      AudioPlayer.stopChannel(1);
      Serial.println("ch1 off!");
    } else if ( c == '3') {
      AudioPlayer.stopChannel(2);
      Serial.println("ch2 off!");
    } else if ( c == '4') {
      AudioPlayer.stopChannel(3);
      Serial.println("ch3 off!");
    }
  }
  */
}
