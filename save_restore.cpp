/* File: save_restore.cpp

  This module saves configuration data to/from SDRAM.
  QSPI Flash chip on the Feather M4 breakout board has 2 MB capacity.

  This design serializes the C++ object by reading and writing a series
  of fixed-length buffers in the config file. There is one config file
  per C++ object; files are cheap so we don't share files for different
  types of settings.

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
Adafruit_FlashTransport_QSPI gFlashTransport;
Adafruit_SPIFlash gFlash(&gFlashTransport);
FatFileSystem gFatfs;          // file system object from SdFat

// ========== helpers =================================
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
int SaveRestore::readConfig() {
  // returns 1=success, 0=failure
  int result = 1;             // assume success
  Serial.println("Starting to read config from SDRAM...");
    
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
  
  // You can get the current position, remaining data, and total size of the file:
  Serial.print(". Total file size (bytes): "); Serial.println(readFile.size(), DEC);
  //Serial.print(". Current position in file: "); Serial.println(readFile.position(), DEC);
  //Serial.print(". Available data remaining to read: "); Serial.println(readFile.available(), DEC);

  // read first field (filename) from config file...
  char temp[sizeof(fqFilename)];     // buffer size is as large as our largest member variable
  int count = readFile.read(temp, sizeof(fqFilename));
  dumpHex("fqFilename", temp, sizeof(fqFilename));
  if (count == -1) {
    Serial.print("Error, failed to read first field from ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }
  // verify filename stored inside file exactly matches expected
  if (strcmp(temp, this->fqFilename) != 0) {
    Serial.print("Error, unexpected filename ("); Serial.print(temp); Serial.println(")");
    return 0;
  }

  // read second field (version string) from config file...
  count = readFile.read(temp, sizeof(sVersion));
  dumpHex("sVersion", temp, sizeof(sVersion));
  if (count == -1) {
    Serial.print("Error, failed to read version number from ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }
  // verify version stored in file exactly matches expected
  if (strcmp(temp, this->sVersion) != 0) {
    Serial.print("Error, unexpected version ("); Serial.print(temp); Serial.println(")");
    return 0;
  }
  // data looks good, read third field (setting) and use its value
  count = readFile.read(temp, sizeof(intSetting));
  dumpHex("intSetting", temp, sizeof(intSetting));
  if (count == -1) {
    Serial.print("Error, failed to read setting value from ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }
  memcpy((void*)&intSetting, temp, sizeof(intSetting));
  Serial.print(". Setting value: "); Serial.println(intSetting);
  
  // close files and clean up
  readFile.close();           // TODO - don't close the file (derived classes need to append data)
  //gFlash.end();             // this causes "undefined reference to 'Adafruit_SPIFlash::end()'

  return result;
}
// ========== save configuration ======================
int SaveRestore::writeConfig() {
  // initialize configuration file in file system, called by setup() if needed
  // assumes this is Feather M4 Express with 2 MB flash
  // returns 1=success, 0=failure
  int result = 1;             // assume success
  Serial.println("Starting to write config to SDRAM...");

  result = openFlash();       // open file system
  if (!result) { return 0; }

  // replace an existing config file
  gFatfs.remove(fqFilename);  // delete old file, or else it appends data to the end
  File writeFile = gFatfs.open(fqFilename, FILE_WRITE); // 
  if (!writeFile) {
    Serial.print("Error, failed to open config file for writing ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }

  Serial.print(". fqFilename ("); Serial.print(fqFilename); Serial.println(")");
  Serial.print(". sVersion ("); Serial.print(sVersion); Serial.println(")");
  Serial.print(". intSetting ("); Serial.print(intSetting); Serial.println(")");
  
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
  count = writeFile.write((char*) &intSetting, sizeof(intSetting));
  if (count == -1) {
    Serial.print("Error, failed to write setting into ("); Serial.print(fqFilename); Serial.println(")");
    return 0;
  }
  
  writeFile.close();           // TODO - don't close the file (derived classes need to append data)
                               // until then we MUST close the file to flush the buffer
  return result;
}

// ----- private helper -----
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
    Serial.println("Was the flash chip formatted with the SdFat_format example?");
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
  return 1;
}
