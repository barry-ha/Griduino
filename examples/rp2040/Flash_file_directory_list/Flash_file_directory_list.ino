// Please format this file with clang before check-in to GitHub
/*
  File: Flash_file_directory_list.ino

  Version history:
            2022-12-27 migrated Feather M4 program into RP2040 + LittleFS
            2022-06-05 refactored pin definitions into hardware.h
            2021-03-16 created, based on old unfinished Griduino_v9\examples\DAC_play_wav_from_SD_v2
            which, in turn, was based on Adafruit's full featured example
            at https://github.com/adafruit/Adafruit_SPIFlash/blob/master/examples/SdFat_full_usage/SdFat_full_usage.ino

  Author:   Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA

  Purpose:  List the file system contents of Feather RP2040 onboard 8MB flash memory.
            Contrary to the Feather M4, the new RP2040 has a single 8MB memory
            space shared for both program and files. You must allocate and
            partition this memory (to 4MB each) in the Arduino IDE.
            This is simple program only reports at root and first folder levels.

  LittleFS docs:
            https://github.com/earlephilhower/arduino-pico-littlefs-plugin
            https://github.com/littlefs-project/littlefs/blob/master/SPEC.md
            https://github.com/littlefs-project/littlefs/blob/master/DESIGN.md
            C:\Users\barry\AppData\Local\Arduino15\packages\rp2040\hardware\rp2040\2.6.5\libraries\LittleFS\src\LittleFS.h
            C:\Users\barry\AppData\Local\Arduino15\packages\rp2040\hardware\rp2040\2.6.5\cores\rp2040\FSImpl.h
  SD file system docs:
            https://www.arduino.cc/en/Reference/SD

  Tested with:
         1. Arduino Feather RP2040 (133 MHz dual-core M0)
            Spec: https://www.adafruit.com/product/3857

*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include "LittleFS.h"           // LittleFS is declared
// #include <SdFat.h>               // for FAT file systems on Flash and Micro SD cards
// #include <Adafruit_SPIFlash.h>   // for FAT file systems on SPI flash chips
#include "hardware.h"   // Griduino pin definitions

// ------- Identity for splash screen and console --------
#define EXAMPLE_TITLE    "Flash File Directory List"
#define EXAMPLE_VERSION  "v1.11 rp2040"
#define PROGRAM_LINE1    "Barry K7BWH"
#define PROGRAM_LINE2    ""
#define PROGRAM_COMPILED __DATE__ " " __TIME__
#define PROGRAM_FILE     __FILE__

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// ---------- TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// SD card share the hardware SPI interface with TFT display, and have
// separate 'select' pins to identify the active device on the bus.
// const int chipSelectPin = 7; // no "chip select" used with rp2040 memory space
// const int chipDetectPin = 8;

// ------------ definitions
const int howLongToWait = 6;   // max number of seconds at startup waiting for Serial port to console

// ----- Griduino color scheme
// RGB 565 true color: https://chrishewett.com/blog/true-rgb565-colour-picker/
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND 0x00A            // 0,   0,  10 = darker than ILI9341_NAVY, but not black
#define cTEXTCOLOR  ILI9341_CYAN     // 0, 255, 255
#define cLABEL      ILI9341_GREEN    //
#define cVALUE      ILI9341_YELLOW   // 255, 255, 0
#define cHIGHLIGHT  ILI9341_WHITE    //
#define cWARN       0xF844           // brighter than ILI9341_RED but not pink

// ------------ global scope
// Adafruit_FlashTransport_QSPI gFlashTransport;   // Quad-SPI 2MB memory chip
// Adafruit_SPIFlash gFlash(&gFlashTransport);     //
// FatFileSystem gFatfs;                           // file system object from SdFat
// LittleFS fs;                                       // file system object from LittleFS is declared in "LittleFS.h"

// ========== splash screen ====================================
const int xLabel    = 8;   // indent labels, slight margin on left edge of screen
const int yRow1     = 8;   // title
const int rowHeight = 12;
int gCurrentY       = yRow1;

void startSplashScreen() {
  tft.setTextSize(1);

  tft.setCursor(xLabel, gCurrentY);
  tft.setTextColor(cTEXTCOLOR, cBACKGROUND);
  tft.print(EXAMPLE_TITLE);
  gCurrentY += rowHeight;

  tft.setCursor(xLabel, gCurrentY);
  tft.setTextColor(cLABEL, cBACKGROUND);
  tft.println("Compiled " PROGRAM_COMPILED);
  gCurrentY += rowHeight;

  tft.setCursor(xLabel, gCurrentY);
  tft.setTextColor(cLABEL, cBACKGROUND);   // continued on same line
  tft.println("Open serial console for listing");
  gCurrentY += rowHeight;
}

// ========== screen helpers ===================================
void clearScreen() {
  tft.fillScreen(cBACKGROUND);
}

// ----- console+display output formatter
void showFile(const char *indent, const int count, const char *filename, const int filesize) {
  char msg[256];
  snprintf(msg, sizeof(msg), "%s%d. %-14s  %d",
           indent, count, filename, filesize);
  Serial.println(msg);

  tft.setCursor(xLabel, gCurrentY);
  tft.setTextColor(cVALUE, cBACKGROUND);
  tft.print(msg);
  gCurrentY += rowHeight;
}

void showDirectory(const char *indent, const int count, const char *dirname) {
  char msg[256];
  snprintf(msg, sizeof(msg), "%s%d. %-14s  (dir)",
           indent, count, dirname);
  Serial.println(msg);

  tft.setCursor(xLabel, gCurrentY);
  tft.setTextColor(cHIGHLIGHT, cBACKGROUND);
  tft.print(msg);
  gCurrentY += rowHeight;
}

void showErrorMessage(const char *error) {
  Serial.println(error);

  tft.setCursor(xLabel, gCurrentY);
  tft.setTextColor(cWARN, cBACKGROUND);
  tft.print(error);
  gCurrentY += rowHeight;
  if (strlen(error) > 51) {   // at font size 1, a row can hold 51 chars
    // if a long line spilled onto next line, add vertical space
    gCurrentY += rowHeight;
  }
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong * 1000;
  while (millis() < targetTime) {
    if (Serial)
      break;
  }
}

// ----- Flash FS helper -----
int openFlash() {
  // returns 1=success, 0=failure

#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  if (!LittleFS.begin()) {   // Start LittleFS
    Serial.println("An Error has occurred while mounting LittleFS");
    delay(500);
    return 0;   // indicate error
  }
#else
  // Initialize flash library and check its chip ID.
  if (!gFlash.begin()) {
    showErrorMessage("Error, failed to initialize onboard memory.");
    return 0;
  }
  Serial.print(". Flash chip JEDEC ID: 0x");
  Serial.println(gFlash.getJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!gFatfs.begin(&gFlash)) {
    showErrorMessage("Error, failed to mount filesystem");
    showErrorMessage("Was the flash chip formatted with the SdFat_format example?");
    return 0;
  }
#endif
  Serial.println(". Mounted flash filesystem");
  return 1;   // success
}

// ----- iterate files in a folder
void listLevel2(const char *folder) {
  // Open a subdirectory to list all its children (files and folders)
  bool okay = true;   // assume success
  Serial.println(folder);
  //File mydir = gFatfs.open(folder);
  File mydir = LittleFS.open(folder, "r");
  if (!mydir) {
    showErrorMessage("Error, failed to open subfolder");
  }
  File kid2 = mydir.openNextFile();
  int count = 1;
  while (kid2 && okay) {
    char filename[64];
    //kid2.getName(filename, sizeof(filename));
    strncpy(filename, kid2.name(), sizeof(filename));
    if (strlen(filename) == 0) {
      Serial.println("   Empty filename, time to return");
      okay = false;
    } else {
      // Print the file name and mention if it's a directory.
      if (kid2.isDirectory()) {
        showDirectory("   ", count, filename);
      } else {
        int filesize = kid2.size();
        showFile("   ", count, filename, filesize);
      }

      // increment loop counters
      kid2 = mydir.openNextFile();
      if (++count > 128) {
        Serial.println("And many more, but I'm stopping here.");
        okay = false;
      }
      delay(20);   // delay in case of runaway listing to quiet things down slightly
    }
  }
}
// ----- iterate files at root level
int listFiles() {
  // Open the root folder to list top-level children (files and directories).
  int rc       = 1;              // assume success
  File testDir = LittleFS.open("/", "r");   //
  if (!testDir) {
    showErrorMessage("Error, failed to open root directory");
    rc = 0;
  }
  if (!testDir.isDirectory()) {
    showErrorMessage("Error, expected root to be a directory");
    rc = 0;
  } else {

    int count = 1;
    Serial.println("Listing files in the root directory:");

    File child = testDir.openNextFile();
    while (child && rc > 0) {
      char filename[64];
      //child.getName(filename, sizeof(filename));
      strncpy(filename, child.name(), sizeof(filename));

      if (strlen(filename) == 0) {
        Serial.println("Empty filename, so time to return");
        rc = 0;
      } else {

        // Print the file name and mention if it's a directory.
        if (child.isDirectory()) {
          showDirectory("", count, filename);
        } else {
          int filesize = child.size();
          showFile("", count, filename, filesize);
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

  // ----- init TFT display
  tft.begin();          // initialize TFT display
  tft.setRotation(0);   // 1=landscape (default is 0=portrait)
  clearScreen();        // note that "begin()" does not clear screen

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);   // start at full brightness

  // ----- announce ourselves
  startSplashScreen();

  // ----- init serial monitor
  Serial.begin(115200);           // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(EXAMPLE_TITLE " " EXAMPLE_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

  // ----- look for memory card
  /* ...
  Serial.print("Detecting if FLASH memory is available, using pin ");
  Serial.print(chipDetectPin);   // todo
  Serial.println(" ... ");
  pinMode(chipDetectPin, INPUT_PULLUP);               // use internal pullup resistor
  bool isCardDetected = digitalRead(chipDetectPin);   // HIGH = no card; LOW = card detected
  if (isCardDetected) {
    Serial.println(". Success - found a memory chip");
  } else {
    Serial.println(". Failed - no memory chip found");
  }
  ... */

  // ----- Do The Thing
  Serial.println("Initializing Flash memory interface...");
  int result = openFlash();   // open file system
  result     = listFiles();   // list all files in the file system
  Serial.println("All done.");
}

//=========== main work loop ===================================
void loop() {

  for (int ii = 30; ii--; ii <= 0) {
    Serial.print(ii);
    Serial.print(" ");
    delay(1000);
  }
  Serial.println(" ");
  clearScreen();   // note that "begin()" does not clear screen
  gCurrentY = yRow1;

  // ----- Do The Thing
  Serial.println("Initializing Flash memory interface...");
  int result = openFlash();   // open file system
  result     = listFiles();   // list all files in the file system
  Serial.println("All done.");
}
