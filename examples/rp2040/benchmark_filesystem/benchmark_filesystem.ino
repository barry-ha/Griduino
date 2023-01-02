// Please format this file with clang before check-in to GitHub

// #define SDFAT_RP2040      // Pick one, recompile
#define LITTLEFS_RP2040   // Pick one, recompile
/*Context:  We are evaluating performance of two different flash file systems:
            1. SdFat for Feather RP2040     #define SDFAT_RP2040
            2. LittleFS for Feather RP2040  #define LITTLEFS_RP2040
            This example program can be compiled two ways.
            The "#if" statements are there to highlight coding differences.
*/
/*
  File: benchmark_filesystem.ino

  Version history:
            2023-01-01 created

  Author:   Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA

  Purpose:  We want to compare performance of writing files for SdFat and LittleFS
            on the Adafruit Feather RP2040.
            - SdFat would be compatible with CircuitPy file handling
            - LittleFS might be more robust, but is it faster?

  From:     File > Examples > SdFat - Adafruit_Fork > ExamplesV1 > PrintBenchmark
            examples\rp2040\Flash_file_directory_list\Flash_file_directory_list.ino

  File system docs:
            SdFat - https://www.arduino.cc/en/Reference/SD
            LittleFS - https://www.arduino.cc/reference/en/libraries/littlefs_mbed_rp2040/

  Tested with:
         1. Adafruit Feather RP2040       https://www.adafruit.com/product/3857

  Results                                 KB      KB/sec   usec  usec   usec
                 Function tested         Size    Write     Max   Min    Avg
      SdFat: --------------------       -----   ------   ------  ---   ---------
          0. println(uint16_t)           16.8    40.41   53,666    8     68 usec
          1. println(uint32_t)           30.0    49.42   53,886   22    150 usec
          2. println(float)              20.0    37.31   53,902   32    125 usec
          3. GPS breadcrumb trail       125.9    42.85   2.94 sec  .      .      O_RDWR
          4. GPS breadcrumb trail       125.9    43.40   2.90 sec  .      .      O_WRONLY
      LittleFS:
          0. println(uint16_t)           16.9    75.74   23,873    9     73 usec
          1. println(uint32_t)           30.0    85.96   18,112   11    115 usec
          2. println(float)              20.0    58.65   18,515   36    112 usec
          3. GPS breadcrumb trail       126.2    48.77   2.59 sec  .      .
      Conclusion:
          LittleFS is faster than SdFat by only 11%
          This does not justify switching file systems.
          To improve performance, redesign breadcrumb trail to save incremental
          additions instead of rewriting entire GPS history file.
*/

// clang-format off
#if defined(SDFAT_M4)
  #include <SdFat.h>               // for FAT file systems on Flash and Micro SD cards

  SdFat sd;
  FatFileSystem fatfs;                           // file system object from SdFat
  Adafruit_FlashTransport_QSPI flashTransport;   // Quad SPI 2MB memory chip

#elif defined(SDFAT_RP2040)
  #include <SdFat.h>               // for FAT file systems on Flash and Micro SD cards
  #include <Adafruit_SPIFlash.h>   // for FAT file systems on SPI flash chips

  SdFat sd;
  FatFileSystem fatfs;                        // file system object from SdFat
  // Un-comment either of the following lines:
  //  Adafruit_FlashTransport_RP2040  flashTransport(Adafruit_FlashTransport_RP2040::CPY_START_ADDR,
  //                                                 Adafruit_FlashTransport_RP2040::CPY_SIZE);
  Adafruit_FlashTransport_RP2040_CPY flashTransport;
  SdFile file;

#elif defined(LITTLEFS_RP2040)
  #include "LittleFS.h"            // LittleFS is declared
  #include <Adafruit_SPIFlash.h>   // for FAT file systems on SPI flash chips

  // RP2040 use same flash device that store code for file system. Therefore we
  // only need to specify start address and size (no need SPI or SS).
  //
  // By default (start=0, size=0), values that match file system setting in
  // 'Tools->Flash Size' menu selection will be used.
  // Adafruit_FlashTransport_RP2040 flashTransport;

  // To be compatible with CircuitPython partition scheme (start_address = 1 MB,
  // size = total flash - 1 MB) use const value (CPY_START_ADDR, CPY_SIZE) or
  // subclass Adafruit_FlashTransport_RP2040_CPY. 
  //
  // Un-comment either of the following lines:
  //  Adafruit_FlashTransport_RP2040  flashTransport(Adafruit_FlashTransport_RP2040::CPY_START_ADDR,
  //                                                 Adafruit_FlashTransport_RP2040::CPY_SIZE);
  Adafruit_FlashTransport_RP2040_CPY flashTransport;
  File file;

#else
  #error Choose your #define test case. 
#endif
// clang-format on

#include "sdios.h"
#include "FreeStack.h"

// ------- Identity for splash screen and console --------
#define EXAMPLE_TITLE   "Flash File Performance Test"
#define EXAMPLE_VERSION "v1.11 rp2040"

// ---------- SD chip
// SD card share the hardware SPI interface with TFT display, and have
// separate 'select' pins to identify the active device on the bus.
const int chipSelect    = SS;
const int chipDetectPin = 8;

// ------------ Flash File System
Adafruit_SPIFlash flash(&flashTransport);   //

// Serial output stream
ArduinoOutStream cout(Serial);

// ============== helpers ======================================
// copied from Griduino.ino
void floatToCharArray(char *result, int maxlen, double fValue, int decimalPlaces) {
  String temp = String(fValue, decimalPlaces);
  temp.toCharArray(result, maxlen);
}

// ============== helpers ======================================
// instead of #include "grid_helper.h", here's a mock replacement
// because this benchmark wants to test file performance, not this subroutine's performance
void calcLocator(char *result, double lat, double lon, int precision) {
  strcpy(result, "JJ00aa");
}

// ============== history array functions from Griduino =========
#include "constants.h"     // for SECONDS_PER_5MIN
#include "date_helper.h"   // for Class Dates
Dates date = Dates();

Location history[3000];   // remember a list of GPS coordinates
const int numHistory = sizeof(history) / sizeof(Location);

#define HISTORY_FILE "bench3.csv"
const char HISTORY_VERSION[25] = "GPS Breadcrumb Trail v1";   // <-- always change version when changing data format

// ----- save GPS history[] to non-volatile memory as CSV file ----- (from model_gps.h)
int saveGPSBreadcrumbTrail() {   // returns 1=success, 0=failure
  // Internal breadcrumb trail is CSV format -- you can open this Arduino file directly in a spreadsheet
  // dumpHistoryGPS();   // debug

  // delete old file and open new file
  // SaveRestoreStrings config(HISTORY_FILE, HISTORY_VERSION);
  // config.open(HISTORY_FILE, "w");

  if (!file) {
    Serial.println("Error, file object is null");
  }

  // line 1,2,3,4: filename, data format, version, compiled
  char msg[256];
  snprintf(msg, sizeof(msg), "File:,%s\nData format:,%s\nGriduino:,%s\nCompiled:,%s",
           HISTORY_FILE, HISTORY_VERSION, PROGRAM_VERSION, PROGRAM_COMPILED);
  // config.writeLine(msg);
  if (file.println(msg)) {
    Serial.println("Okay, wrote Lines 1-4 to file");
  } else {
    Serial.println("Error, failed to write Lines 1-4 to file");
  }

  // line 5: column headings
  // config.writeLine("GMT Date, GMT Time, Grid, Latitude, Longitude");
  if (file.println("GMT Date, GMT Time, Grid, Latitude, Longitude")) {
    Serial.println("Okay, wrote Line 5 to file");
  } else {
    Serial.println("Error, failed to write Line 5 to file");
  }

  // line 6..x: date-time, grid6, latitude, longitude
  int count = 0;
  for (uint ii = 0; ii < numHistory; ii++) {
    // if (!history[ii].isEmpty()) {
    if (true) {   // for sake of performance test, we save all breadcrumbs even if empty
      count++;

      char sDate[12];   // sizeof("2022-11-10") = 10
      date.dateToString(sDate, sizeof(sDate), history[ii].timestamp);

      char sTime[12];   // sizeof("12:34:56") = 8
      date.timeToString(sTime, sizeof(sTime), history[ii].timestamp);

      char sGrid6[7];
      // grid.calcLocator(sGrid6, history[ii].loc.lat, history[ii].loc.lng, 6);
      calcLocator(sGrid6, history[ii].loc.lat, history[ii].loc.lng, 6);

      char sLat[12], sLng[12];
      floatToCharArray(sLat, sizeof(sLat), history[ii].loc.lat, 5);
      floatToCharArray(sLng, sizeof(sLng), history[ii].loc.lng, 5);

      snprintf(msg, sizeof(msg), "%s,%s,%s,%s,%s", sDate, sTime, sGrid6, sLat, sLng);
      // config.writeLine(msg);
      if (!file.println(msg)) {
        char errmsg[128];
        snprintf(errmsg, sizeof(errmsg), "Error writing element %d [loc %d]: %s",
                 ii, __LINE__, msg);
        Serial.println(errmsg);
        break;
      }
    }
  }
  Serial.print(". Wrote ");
  Serial.print(count);
  Serial.println(" entries to GPS log");

  // close file
  // file.close();

  return 1;   // success
}

//------------------------------------------------------------------------------
#if defined(SDFAT_RP2040)
// store error strings in flash to save RAM
#define error(s) sd.errorHalt(F(s))
#elif defined(LITTLEFS_RP2040)
#define error(s) Serial.println(F(s))
#endif

// ----- Flash FS helper -----
int openFlash() {
  // returns 1=success, 0=failure

  // Initialize flash library
  Serial.println("Initializing Flash memory interface...");
#if defined(SDFAT_M4)
  if (!flash.begin()) {
    showErrorMessage("Error, failed to initialize onboard flash memory chip!");
    return 0;
  }
  if (!fatfs.begin(&flash)) {
    showErrorMessage("Error, failed to mount SdFat filesystem");
    showErrorMessage("  Was the flash chip formatted with the SdFat_format example?");
    showErrorMessage("  Was CircuitPython installed at least once?");
    return 0;   // indicate error
  }

#elif defined(SDFAT_RP2040)
  if (!flash.begin()) {
    Serial.println("Error, failed to initialize onboard flash memory chip!");
    return 0;
  }
  if (!fatfs.begin(&flash)) {
    Serial.println("Error, failed to mount SdFat filesystem");
    Serial.println("  Was the flash chip formatted with the SdFat_format example?");
    Serial.println("  Was CircuitPython installed at least once to create the filesystem?");
    return 0;   // indicate error
  }

#else   // defined(LITTLEFS_RP2040)
  if (!LittleFS.begin()) {
    Serial.println("Error, failed to mount LittleFS filesystem");
    Serial.println("  Was memory space allocated in the IDE?");
    delay(500);
    return 0;   // indicate error
  }
#endif

  // Check flash chip ID
  Serial.print(". Flash chip JEDEC ID: 0x");
  Serial.println(flash.getJEDECID(), HEX);   // typ: 0xC84015 on Feather M4

  Serial.println(". Mounted flash filesystem");
  return 1;   // success
}

//=========== setup ============================================
void setup() {
  Serial.begin(115200);   // init for debugging in the Arduino IDE
  while (!Serial) {       // Wait for console in the Arduino IDE
    yield();
  }

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(EXAMPLE_TITLE " " EXAMPLE_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

  // init gps history array
  for (int ii = 0; ii < numHistory; ii++) {
    history[ii].reset();
  }
}

//=========== main work loop ===================================
void loop() {
  uint32_t maxLatency;
  uint32_t minLatency;
  uint32_t totalLatency;

  // Read any existing Serial data.
  do {
    delay(10);
  } while (Serial.available() && Serial.read() >= 0);

  // F stores strings in flash to save RAM
  cout << F("Type any character to start\n");
  while (!Serial.available()) {
    yield();
  }
  delay(400);   // catch Due reset problem

  cout << F("FreeStack: ") << FreeStack() << endl;

  // Initialize at the highest speed supported by the board that is
  // not over 50 MHz. Try a lower speed if SPI errors occur.
  // if (!sd.begin(chipSelect, SD_SCK_MHZ(50))) {
  //  sd.initErrorHalt();
  //}
  openFlash();   // open file system

  cout << F("Starting speed test.  Please wait.\n\n");

  // do write test
  for (int test = 0; test < 3; test++) {
    char fileName[13] = "bench0.txt";
    fileName[5]       = '0' + test;

    // open or create file - truncate existing file.
#if defined(SDFAT_RP2040)
    if (!file.open(fileName, O_RDWR | O_CREAT | O_TRUNC)) {
      error("open failed");
    }
#elif defined(LITTLEFS_RP2040)
    file     = LittleFS.open(fileName, "w");
#endif
    maxLatency   = 0;
    minLatency   = 999999;
    totalLatency = 0;
    switch (test) {
    case 0:
      cout << test << F(". Test of println(uint16_t)\n");
      break;

    case 1:
      cout << test << F(". Test of println(uint32_t)\n");
      break;

    case 2:
      cout << test << F(". Test of println(float)\n");
      break;
    }

    uint32_t t             = millis();   // elapsed time counter
    const uint16_t N_PRINT = 3000;       // number of lines to write to file
    for (uint16_t i = 0; i < N_PRINT; i++) {
      uint32_t m = micros();

      switch (test) {
      case 0:
        file.println(i);
        break;

      case 1:
        file.println(12345678UL + i);
        break;

      case 2:
        file.println((float)0.01 * i);
        break;
      }
      if (file.getWriteError()) {
        error("write failed");
      }
      m = micros() - m;
      if (maxLatency < m) {
        maxLatency = m;
      }
      if (minLatency > m) {
        minLatency = m;
      }
      totalLatency += m;
    }
    t = millis() - t;
#if defined(SDFAT_RP2040)
    double s = file.fileSize();
#elif defined(LITTLEFS_RP2040)
    double s = file.size();   // size of this one file
#endif
    file.close();
    cout << F("Time ") << 0.001 * t << F(" sec\n");
    cout << F("File size ") << 0.001 * s << F(" KB\n");
    cout << F("Write ") << s / t << F(" KB/sec\n");
    cout << F("Maximum latency: ") << maxLatency;
    cout << F(" usec, Minimum Latency: ") << minLatency;
    cout << F(" usec, Avg Latency: ");
    cout << totalLatency / N_PRINT << F(" usec\n\n");
  }   // end for-loop

  // ---------- Run the GPS test only once
  int test = 3;
  cout << test << F(". Test saving GPS breadcrumb trail\n");
  char fileName[13] = HISTORY_FILE;
  fileName[5]       = '0' + test;

  // open or create file - truncate existing file.
#if defined(SDFAT_RP2040)
  if (!file.open(fileName, O_WRONLY | O_CREAT | O_TRUNC)) {
    error("open failed");
  }
#elif defined(LITTLEFS_RP2040)
  file     = LittleFS.open(fileName, "w");
#endif

  uint32_t t = millis();   // init elapsed time counter

  saveGPSBreadcrumbTrail();   // Do The Thing

  t = millis() - t;   // calc elapsed time
#if defined(SDFAT_RP2040)
  double s = file.fileSize();
#elif defined(LITTLEFS_RP2040)
  double s = file.size();   // size of this one file
#endif
  file.close();
  cout << F("Time ") << 0.001 * t << F(" sec\n");
  cout << F("File size ") << 0.001 * s << F(" KB\n");
  cout << F("Write ") << s / t << F(" KB/sec\n");

  cout << F("Done!\n\n");
}
