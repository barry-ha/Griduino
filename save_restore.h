/* File: save_restore.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  This module saves configuration data to/from SDRAM.
  QSPI Flash chip on the Feather M4 breakout board has 2 MB capacity.
*/
#pragma once

class SaveRestore {
  public:
    char fqFilename[64];      // fully qualified filename, e.g. "/Settings/volume.cfg" (strictly 8.3 names)
    char sVersion[16];        // ID string, detects if settings are actually written

  public:
    /**
     * Constructor
     */
    SaveRestore(const char* vFilename, const char* vVersion) {
      // vFilename MUST follow 8.3 naming conventions
      strcpy(fqFilename, vFilename);
      strcpy(sVersion, vVersion);
    }
    ~SaveRestore() {}
  
    /**
     * Save our class data to SDRAM
     */
    int writeConfig(const byte* pBuffer, const int sizeBuffer);

    /**
     * Load our class data from SDRAM
     */
    int readConfig(byte* pBuffer, const int sizeBuffer);

    /**
     * Delete file
     */
    int remove(const char* vFilename) {
      /* todo */ 
      return 0;
    }

  protected:
    int openFlash();          // helper
};
