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

class SaveRestore {
public:
  char sFoldername[32];   // folder name, e.g. "/Settings" (must be 8.3 name)
  char fqFilename[64];    // fully qualified filename, e.g. "/Settings/volume.cfg" (strictly 8.3 names)
  char sVersion[32];      // ID string, detects if settings are actually written
  int intSetting;         // general-purpose integer

private:
  int openFlash();      // open file system
  int verifyFolder();   // if needed, create target folder

public:
  /*
   * Constructor
   */
  SaveRestore(const char *vFoldername, const char *vFilename, const char *vVersion, const int vSetting = 0) {
    // names MUST follow 8.3 naming conventions
    strcpy(sFoldername, vFoldername);
    strcpy(fqFilename, vFilename);
    intSetting = vSetting;

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
  int readConfig();

  // ========== save configuration ======================
  /*
   * Save our class data to SDRAM
   */
  int writeConfig();

  // ========== file management ======================
  /*
   * Delete file
   */
  int remove(const char *vFilename) {
    // todo
    return 0;
  }
};
