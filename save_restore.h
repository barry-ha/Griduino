#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     save_restore.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This module saves configuration data to/from SDRAM.
            QSPI Flash chip on the Feather M4 Express breakout board has 2 MB capacity.

  Typical Save C++ Object:
            void saveConfig() {
              SaveRestore config(CONFIG_FILE_NAME, CONFIG_VERSION_STRING);
              config.writeConfig((byte *)&buffer, sizeof(buffer));
            }
  Typical Restore C++ Object:
            void restoreConfig() {
              SaveRestore config(CONFIG_FILE_NAME, CONFIG_VERSION_STRING);
              int tempValue;
              int result = config.readConfig((byte *)&tempValue, sizeof(tempValue));
              if (result) {
                Serial.print("Loaded my value from NVR: ");
                Serial.println(myValue);
              } else {
                Serial.println("Failed to load settings, re-initializing config file");
                saveConfig();
              }
            }
*/
#include <SdFat.h>    // SDRAM File Allocation Table filesystem
#include "logger.h"   // conditional printing to Serial port

// ========== extern ===========================================
extern Logger logger;   // Griduino.ino

// ========== class SaveRestore =========================
class SaveRestore {
public:
  char fqFilename[64];   // fully qualified filename, e.g. "/Settings/volume.cfg" (strictly 8.3 names)
  char sVersion[32];     // ID string, detects if settings are actually written

public:
  /*
   * Constructor
   */
  SaveRestore(const char *vFilename, const char *vVersion) {
    // vFilename MUST follow 8.3 naming conventions
    if (strlen(vFilename) < sizeof(fqFilename)) {
      strcpy(fqFilename, vFilename);
    } else {
      Serial.println("Error! Filename exceeds buffer length");
    }

    if (strlen(vVersion) < sizeof(sVersion)) {
      strcpy(sVersion, vVersion);
    } else {
      Serial.println("Error! Version string exceeds buffer length");
    }
  }
  ~SaveRestore() {}

  // ========== load configuration ======================
  /*
   * Load our class data from SDRAM
   */
  int readConfig(byte *pBuffer, const unsigned int sizeBuffer);

  // ========== save configuration ======================
  /*
   * Save our class data to SDRAM
   */
  int writeConfig(const byte *pBuffer, const unsigned int sizeBuffer);

  // ========== file management ======================
  // Very basic file management is provided to report on the files
  // stored in SDRAM. This provides an easier way to see where and how
  // the SDRAM file system is being used via USB port, compared to the
  // awkward process of restarting in CircuitPy mode.
  /*
   * List files in SDRAM
   */
  int listFiles(const char *dirname);

  /*
   * Delete file
   */
  int deleteFile(const char *vFilename);

protected:
  int openFlash();   // helper
  void showFile(const char *indent, const int count, const char *filename, const int filesize);
  void showDirectory(const char *dirname);
};

// ========== line-by-line string functions ============
/*
 * For null-terminated strings to SDRAM
 * Returns 1=success, 0=failure
 */
class SaveRestoreStrings : public SaveRestore {
public:
  /*
   * Constructor
   */
  SaveRestoreStrings(const char *vFilename, const char *vVersion)
      : SaveRestore{vFilename, vVersion} {
  }
  ~SaveRestoreStrings() {}

  int open(const char *filename, const char *mode);   // https://cplusplus.com/reference/cstdio/fopen/
  int writeLine(const char *pBuffer);                 // https://cplusplus.com/reference/cstdio/snprintf/
  int readLine(char *pBuffer, int bufflen);           // https://cplusplus.com/reference/cstdio/gets/
  uint8_t getError() {
    return handle.getError();
  }
  void close();

protected:
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  // Adafruit Feather RP2040, https://www.adafruit.com/product/4884
  File32 handle;
#else
  // Adafruit Feather M4, https://www.adafruit.com/product/3857
  File32 handle;   // contains result of gFatfs.open()
#endif
};
