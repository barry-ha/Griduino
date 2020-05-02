#ifndef _GRIDUINO_SAVE_RESTORE_H
#define _GRIDUINO_SAVE_RESTORE_H

/* File: save_restore.h

  This module saves configuration data to/from SDRAM.
  QSPI Flash chip on the Feather M4 breakout board has 2 MB capacity.
*/

class SaveRestore {
  public:
    char sFoldername[32];     // folder name, e.g. "/Settings" (must be 8.3 name)
    char fqFilename[64];      // fully qualified filename, e.g. "/Settings/volume.cfg" (must be 8.3 name)
    char sVersion[16];        // ID string, detects if settings are actually written
    int intSetting;           // general-purpose integer

  private:
    int openFlash();          // open file system
    int verifyFolder();       // if needed, create target folder

  public:
	/**
	 * Constructor
	 */
    SaveRestore(const char* vFoldername, const char* vFilename, const char* vVersion, const int vSetting=0) {
      // names MUST follow 8.3 naming conventions
      strcpy(sFoldername, vFoldername);
      strcpy(fqFilename, vFilename);
      strcpy(sVersion, vVersion);
      intSetting = vSetting;
    }
    ~SaveRestore() {}
  
    /**
     * Save our class data to SDRAM
     */
    int writeConfig();

    /**
     * Load our class data from SDRAM
     */
    int readConfig();
};

#endif // _GRIDUINO_SAVE_RESTORE_H
