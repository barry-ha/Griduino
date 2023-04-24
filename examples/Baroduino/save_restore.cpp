/* File: save_restore.cpp

  This module saves configuration data to/from SDRAM.
  QSPI Flash chip on the Feather M4 breakout board has 2 MB capacity.

  This design serializes the C++ object by reading and writing a series
  of fixed-length buffers in the config file:
  Field 1: File name - stored inside file itself as a sanity check
  Field 2: File version - sanity check to ensure 'restore' matches 'save'
  Field 3: Data - base class stores an integer which is sufficient for
                  simple things like Volume or TimeZone settings

  There is one config file per C++ object; files are cheap so we don't 
  share files for different types of settings. Every 'save' operation 
  will erase and rewrite its associated data file.

  'SaveRestore' object is a base class. You can extend it by deriving
  another class, calling the base class methods first to read or write data,
  then adding any more data needed.

  See file system reference: https://www.arduino.cc/en/Reference/SD
*/

#include <Arduino.h>
#include "constants.h"            // Griduino constants and colors
#include "save_restore.h"         // class definition
#include "SPI.h"                  // Serial Peripheral Interface
#include <SdFat.h>                // SDRAM File Allocation Table filesystem
#include <Adafruit_SPIFlash.h>

// ------------ forward references in this same .cpp file
int openFlash();

// ========== globals =================================
Adafruit_FlashTransport_QSPI flashTransport;
Adafruit_SPIFlash gFlash(&flashTransport);
FatFileSystem gFatfs;          // file system object from SdFat

// ========== debug helper ============================
static void dumpHex(const char * text, char * buff, int len) {
  // debug helper to put data on console
  #ifdef RUN_UNIT_TESTS
    char sout[128];
    snprintf(sout, sizeof(sout), ". %s [%d] : '%s' : ", text, len, buff);
    Serial.print(sout);
    for (int i=0; i<len; i++) {
      sprintf(sout, "%02x ", buff[i]);
      Serial.print(sout);
    }
    Serial.println("");
  #endif
}

// ========== load configuration ======================
int SaveRestore::readConfig(byte* pData, const int sizeData) {
  // returns 1=success, 0=failure
  int result = 1;             // assume success
  Serial.println("Starting to read config from SDRAM...");
  Serial.print(". fqFilename ("); Serial.print(fqFilename); Serial.println(")");
    
  result = openFlash();       // open file system
  if (!result) {
    Serial.print("error, failed to open Flash file system");
    return 0;
  }

  // open config file
  File readFile = gFatfs.open(fqFilename, FILE_READ);
  if (!readFile) {
    Serial.print("Error, failed to open config file for reading ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }
  
  // Echo metadata about the file:
  Serial.print(". Total file size (bytes): "); Serial.println(readFile.size(), DEC);
  Serial.print(". Current position in file: "); Serial.println(readFile.position(), DEC);
  Serial.print(". Available data remaining to read: "); Serial.println(readFile.available(), DEC);

  // read first field (filename) from config file...
  char temp[sizeof(fqFilename)];     // buffer size is as large as our largest member variable
  int count = readFile.read(temp, sizeof(fqFilename));
  dumpHex("fqFilename", temp, sizeof(fqFilename));    // debug
  if (count == -1) {
    Serial.print("Error, failed to read first field from ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }
  // verify first field (filename) stored inside file exactly matches expected
  if (strcmp(temp, this->fqFilename) != 0) {
    Serial.print("Error, unexpected filename ("); Serial.print(temp); Serial.println(")");
    return 0;
  }

  // read second field (version string) from config file...
  count = readFile.read(temp, sizeof(sVersion));
  dumpHex("sVersion", temp, sizeof(sVersion));    // debug
  if (count == -1) {
    Serial.print("Error, failed to read version number from ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }
  // verify second field (version string) stored in file exactly matches expected
  if (strcmp(temp, this->sVersion) != 0) {
    Serial.print("Error, unexpected version ("); Serial.print(temp); Serial.println(")");
    return 0;
  }
  // data looks good, read third field (setting) and use its value
  count = readFile.read(pData, sizeData);
  dumpHex("pData", (char*)pData, sizeData);    // debug
  if (count == -1) {
    Serial.print("Error, failed to read integer value from ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }

  Serial.print(". Data length ("); Serial.print(sizeData); Serial.println(")");
  
  // close files and clean up
  readFile.close();
  //gFlash.end();             // this causes "undefined reference to 'Adafruit_SPIFlash::end()'

  return result;
}
// ========== save configuration ======================
int SaveRestore::writeConfig(const byte* pData, const int sizeData) {
  // initialize configuration file in file system, called by setup() if needed
  // assumes this is Feather M4 Express with 2 MB flash
  // returns 1=success, 0=failure
  int result = 1;             // assume success
  Serial.println("Starting to write config to SDRAM...");

  result = openFlash();       // open file system
  if (!result) { return 0; }

  // replace an existing config file
  gFatfs.remove(fqFilename);  // delete old file (or else it would append data to the end)
  File writeFile = gFatfs.open(fqFilename, FILE_WRITE); // 
  if (!writeFile) {
    Serial.print("Error, failed to open config file for writing ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }

  Serial.print(". fqFilename ("); Serial.print(fqFilename); Serial.println(")");
  Serial.print(". sVersion ("); Serial.print(sVersion); Serial.println(")");
  Serial.print(". Data length ("); Serial.print(sizeData); Serial.println(")");
  
  // write config data to file...
  int count;
  count = writeFile.write(fqFilename, sizeof(fqFilename));
  if (count == -1) {
    Serial.print("Error, failed to write filename into ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }
  count = writeFile.write(sVersion, sizeof(sVersion));
  if (count == -1) {
    Serial.print("Error, failed to write version number into ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }
  count = writeFile.write(pData, sizeData);
  if (count == -1) {
    Serial.print("Error, failed to write setting into ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }
  
  writeFile.close();
  return result;
}

// ----- protected helpers -----
int SaveRestore::openFlash() {
  // returns 1=success, 0=failure
  
  // Initialize flash library and check its chip ID.
  if (!gFlash.begin()) {
    Serial.println("Error, failed to initialize onboard memory.");
    return 0;
  }
  //Serial.print(". Flash chip JEDEC ID: 0x"); Serial.println(gFlash.getJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!gFatfs.begin(&gFlash)) {
    Serial.println("Error, failed to mount filesystem!");
    logger.error("Was the flash chip formatted with File > Examples > Adafruit SPIFlash > SdFat_format?");
    return 0;
  }
  //Serial.println(". Mounted SPI flash filesystem");

  // Check if our config data directory exists and create it if not there.
  // Note you should _not_ add a trailing slash (like '/test/') to directory names.
  // You can use the exists() function to check for the existence of a file.
  if (!gFatfs.exists(CONFIG_FOLDER)) {
    Serial.println(". Configuration directory not found, creating...");
    gFatfs.mkdir(CONFIG_FOLDER);     // Use mkdir to create directory (note you should _not_ have a trailing slash)
    
    if ( !gFatfs.exists(CONFIG_FOLDER) ) {
      Serial.print("Error, failed to create directory (");
      return 0;
    } else {
      Serial.print(". Created directory (");
    }
    Serial.print(CONFIG_FOLDER); Serial.println(")");
  }
  return 1;   // indicate success
}
