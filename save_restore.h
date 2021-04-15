#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     save_restore.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This module saves configuration data to/from SDRAM.
            QSPI Flash chip on the Feather M4 breakout board has 2 MB capacity.
*/

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

  /*
   * Save our class data to SDRAM
   */
  int writeConfig(const byte *pBuffer, const int sizeBuffer);

  /*
   * Load our class data from SDRAM
   */
  int readConfig(byte *pBuffer, const int sizeBuffer);

  /*
   * Delete file
   */
  int remove(const char *vFilename) {
    // todo
    return 0;
  }

protected:
  int openFlash();   // helper
};
