// Please format this file with clang before check-in to GitHub
/*
  File:     save_restore.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This module saves configuration data to/from SDRAM.
            QSPI Flash chip on the Feather M4 Express breakout board has 2 MB capacity.

            This design serializes the C++ object by reading and writing a series
            of fixed-length buffers in the config file:
            Field 1: File name - stored inside file itself as a sanity check
            Field 2: File version - sanity check to ensure 'restore' matches 'save'
            Field 3: Data - base class stores an integer which is sufficient for
                     simple numeric settings like Volume or TimeZone settings

            There is one config file per C++ object; files are cheap so we don't
            share files for different types of settings. Every 'save' operation
            will erase and rewrite its associated data file.

            'SaveRestore' object is a base class. You can extend it by deriving
            another class, calling the base class methods first to read or write data,
            then adding any more data needed.

            See file system reference: https://www.arduino.cc/en/Reference/SD
*/

#include <Arduino.h>
#include "constants.h"           // Griduino constants and colors
#include "save_restore.h"        // class definition
#include "logger.h"              // conditional printing to Serial port
#include <SPI.h>                 // Serial Peripheral Interface
#include <SdFat.h>               // SDRAM File Allocation Table filesystem
#include <Adafruit_SPIFlash.h>   // for FAT filesystems on SPI flash chips.

// ------------ forward references in this same .cpp file
int openFlash();

// ========== globals =================================
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
// Adafruit Feather RP2040, https://www.adafruit.com/product/4884
Adafruit_FlashTransport_RP2040_CPY flashTransport;   // onboard RAM, compatible with CircuitPy
#else
// Adafruit Feather M4, https://www.adafruit.com/product/3857
Adafruit_FlashTransport_QSPI flashTransport;   // Quad SPI 2MB memory chip
#endif
Adafruit_SPIFlash gFlash(&flashTransport);
FatFileSystem gFatfs;   // file system object from SdFat
extern Logger logger;   // Griduino.ino

// ========== debug helper ============================
static void dumpHex(const char *text, char *buff, int len) {
  // debug helper to put data on console
  char out[128];
  snprintf(out, sizeof(out), ". %s [%d] : '%s' : ", text, len, buff);
  Serial.print(out);
  for (int i = 0; i < len; i++) {
    sprintf(out, "%02x ", buff[i]);
    Serial.print(out);
  }
  Serial.println("");
}

// ========== delete file =============================
int SaveRestore::deleteFile(const char *vFilename) {
  int result = 1;   // assume success
  logger.info("Delete file from SDRAM: ", vFilename);
  gFatfs.remove(fqFilename);   // delete file
  return result;
}

// ========== load configuration ======================
/*
 * Load our class data from SDRAM
 */
int SaveRestore::readConfig(byte *pData, const unsigned int sizeData) {
  // returns 1=success, 0=failure
  int result = 1;   // assume success
  logger.info("Reading config from SDRAM...");
  logger.info(". ", fqFilename);

  result = openFlash();   // open file system and report errors
  if (!result) {
    return 0;
  }

  // open config file
  File32 readFile = gFatfs.open(fqFilename, FILE_READ);
  if (!readFile) {
    logger.error("Error, failed to open config file for reading, ", fqFilename);
    return 0;
  }

  // Echo metadata about the file:
  logger.info(". Total file size (bytes): %d", readFile.size());
  // logger.info(". Current position in file: %d", readFile.position());
  // logger.info(". Available data remaining to read: %d", readFile.available());

  // read first field (filename) from config file...
  char temp[sizeof(fqFilename)];   // buffer size is as large as our largest member variable
  int count = readFile.read(temp, sizeof(fqFilename));
  // dumpHex("fqFilename", temp, sizeof(fqFilename));   // debug
  if (count == -1) {
    logger.error("Error, failed to read first field, ", fqFilename);
    return 0;
  }

  // verify first field (filename) stored inside file exactly matches expected
  if (strcmp(temp, this->fqFilename) != 0) {
    logger.error("Error, unexpected filename, ", temp);
    return 0;
  }

  // read second field (version string) from config file...
  count = readFile.read(temp, sizeof(sVersion));
  // dumpHex("sVersion", temp, sizeof(sVersion));   // debug
  if (count == -1) {
    logger.error("Error, failed to read version number, ", fqFilename);
    return 0;
  }
  // verify second field (version string) stored in file exactly matches expected
  if (strcmp(temp, this->sVersion) != 0) {
    Serial.print("Error, unexpected version, got (");
    Serial.print(temp);
    Serial.print(") and expected (");
    Serial.print(this->sVersion);
    Serial.println(")");
    return 0;
  }
  // data looks good, read third field (setting) and use its value
  count = readFile.read(pData, sizeData);
  // dumpHex("pData", (char *)pData, sizeData);   // debug
  if (count == -1) {
    logger.error("Error, failed to read integer value from ", fqFilename);
    return 0;
  }

  logger.info(". Data length: %d", sizeData);
  // logger.info(". Data value: %d", *pData);

  // close files and clean up
  readFile.close();
  // gFlash.end();             // this causes "undefined reference to 'Adafruit_SPIFlash::end()'

  return result;
}

// ========== save configuration ======================
/*
 * Save our class data to SDRAM
 */
int SaveRestore::writeConfig(const byte *pData, const unsigned int sizeData) {
  // initialize configuration file in file system, called by setup() if needed
  // assumes this is Feather M4 Express with 2 MB Quad-SPI flash memory
  // returns 1=success, 0=failure
  logger.info("Writing config data to flash");

  int result = openFlash();   // open file system and report errors
  if (!result) {
    return 0;
  }

  // replace an existing config file
  File32 writeFile = gFatfs.open(fqFilename, O_RDWR | O_CREAT | O_TRUNC);
  if (!writeFile) {
    logger.error("Error, failed to open config file for writing, ", fqFilename);
    return 0;
  }

  logger.info(". fqFilename ", fqFilename);
  logger.info(". sVersion ", sVersion);
  logger.info(". Data length %d", sizeData);

  // write config data to file...
  int count;
  count = writeFile.write(fqFilename, sizeof(fqFilename));
  if (count == -1) {
    logger.error("Error, failed to write filename into ", fqFilename);
    return 0;
  }
  count = writeFile.write(sVersion, sizeof(sVersion));
  if (count == -1) {
    logger.error("Error, failed to write version number into ", fqFilename);
    return 0;
  }
  count = writeFile.write(pData, sizeData);
  if (count == -1) {
    logger.error("Error, failed to write setting into ", fqFilename);
    return 0;
  }

  writeFile.close();
  return 1;   // success
}

// ========== line-by-line string functions ============
/*
 * For null-terminated strings to SDRAM
 * Returns 1=success, 0=failure
 */
int SaveRestoreStrings::open(const char *fqFilename, const char *mode) {   // https://cplusplus.com/reference/cstdio/fopen/
  // returns 1=success, 0=failure
  if (*mode == 'w') {
    logger.info("Writing to file system: ", fqFilename);
  } else {
    logger.info("Reading from file system: ", fqFilename);
  }

  int result = openFlash();   // open file system and report errors
  if (!result) {
    return 0;
  }
  switch (mode[0]) {
  case 'r':
    if (!gFatfs.exists(fqFilename)) {
      logger.error("File does not exist, ", fqFilename);
      return 0;
    }

    handle = gFatfs.open(fqFilename, FILE_READ);
    if (!handle) {
      // failed
      logger.error("Error, failed to open string file for reading");
      return 0;
    }

    // success
    logger.info(". Total file size (bytes): %d", handle.size());
    break;
  case 'w':
    gFatfs.remove(fqFilename);   // delete previous file (or else it appends data to the end)
    handle = gFatfs.open(fqFilename, FILE_WRITE);
    if (!handle) {
      logger.error("Error, failed to open string file for writing");
      return 0;
    }
    break;
  default:
    logger.error("Error, unknown file mode, ", mode);
    break;
  }

  return 1;   // success
};
int SaveRestoreStrings::writeLine(const char *pBuffer) {   // https://cplusplus.com/reference/cstdio/snprintf/
  // we chose to always append EOL to encourage reducing the number of writes
  // QuadSPI ram is slow; you should reduce writes to maximize speed
  char msg[256];
  int len = strlen(pBuffer);
  if (len > 256) {
    snprintf(msg, sizeof(msg), "Warning, string length (%d characters) is very long");
    logger.warning(msg);
  }
  int count = handle.print(pBuffer);
  handle.print("\n");   // write eol
  if (count == -1) {
    snprintf(msg, sizeof(msg), "Error, failed to write line into (%s)", fqFilename);
    logger.error(msg);
    return 0;   // failure
  }
  return count;   // success
}
int SaveRestoreStrings::readLine(char *pBuffer, int bufflen) {   // https://cplusplus.com/reference/cstdio/gets/
  // returns count = number of bytes read
  //         count = 0 = EOF
  //         count = -1 = error, ref: SDFat_-_Adafruit_fork / src / FatLib / FatFile.h
  int count = handle.fgets(pBuffer, bufflen);

  // remove LF from the string
  // (the CR has already been removed but we'll check anyway)
  char key[] = "\r\n";
  char *pch  = strpbrk(pBuffer, key);
  while (pch != NULL) {
    *pch = ' ';
    pch  = strpbrk(pch + 1, key);
  }
  return count;
}
void SaveRestoreStrings::close() {
  // Echo metadata about the file:
  logger.info(". Total file size (bytes): %d", handle.size());
  handle.close();
}

// ========== file management ======================
// Very basic file management is provided to report on the files
// stored in SDRAM. This provides an easier way to see where and how
// the SDRAM file system is being used via USB port, compared to the
// awkward process of restarting in CircuitPy mode.
// Example output:
/*
 * Directory of /
 *          0 .Trashes
 *         22 code.py
 *            lib  <dir>
 *        117 boo_out.txt
 *         70 data.txt
 *            Griduino  <dir>
 *          0 main.py
 *            __pycache__  <dir>
 *       1240 Griduinoten_mile.cfg
 *            audio  <dir>
 *          8 Files 7032 bytes
 *
 *  Directory of /lib
 *        117 boot_out.txt
 *          1 Files 11 bytes
 *
 *  Directory of /Griduino
 *        100 screen.cfg
 *        100 volume.cfg
 *         97 announce.cfg
 *       6240 barometr.dat
 *      19416 gpsmodel.cfg
 *       1240 ten_mile.cfg
 *          6 Files 2240 bytes
 *
 *  Directory of /audio
 *      20552 0.wav
 *      15860 1.wav
 *      15874 2.wav
 *      16108 3.wav
 *      27208 a.wav
 *      22058 b.wav
 *      23862 c.wav
 *      23514 d.wav
 *         36 Files 890402 bytes
 *
 *  Total files listed:
 *    1031247 bytes
 *         49 Files
 *          6 Directories
 *
 */
int SaveRestore::listFiles(const char *dirname) {
  // Open the root folder to list top-level children (files and directories).
  int rc         = 1;   // assume success
  File32 testDir = gFatfs.open(dirname);
  if (!testDir) {
    logger.error("Error, failed to open directory ", dirname);
    rc = 0;
  }
  if (!testDir.isDirectory()) {
    logger.error("Error, expected ", dirname, " to be a directory");
    rc = 0;
  } else {

    int count = 1;
    logger.info("\r\nDirectory of ", dirname);   // announce start of directory listing

    int fileCount = 0;
    int byteCount = 0;
    File32 child  = testDir.openNextFile();
    while (child && rc > 0) {
      char filename[64];
      child.getName(filename, sizeof(filename));

      if (strlen(filename) == 0) {
        logger.info("Empty filename, so time to return");   // is this ever triggered?
        rc = 0;
      } else {

        // Print the file name and mention if it's a directory.
        if (child.isDirectory()) {
          showDirectory(filename);
        } else {
          int filesize = child.size();
          showFile("", count, filename, filesize);

          fileCount++;
          byteCount += filesize;
        }

        // increment loop counters
        child = testDir.openNextFile();
        if (++count > 64) {
          logger.warning("And many more, but I'm stopping here.");
          rc = 0;
        }
      }
    }

    char msg[256];   // report summary for the directory
    snprintf(msg, sizeof(msg), "%12d Files, %d bytes", fileCount, byteCount);
    logger.info(msg);

    // Now, having listed the FILES, let's loop through again for DIRECTORIES
    testDir = gFatfs.open(dirname);
    child   = testDir.openNextFile();
    while (child && rc > 0) {
      char filename[64];
      child.getName(filename, sizeof(filename));

      if (strlen(filename) == 0) {
        logger.info(__LINE__, "Empty filename, so time to return");   // is this ever triggered?
        rc = 0;
      } else {
        // Descend into the directory and list its files
        if (child.isDirectory()) {
          listFiles(filename);   // RECURSION
        }
      }
      child = testDir.openNextFile();   // advance to next file
    }
  }
  return rc;
}

// ----- console output formatter
void SaveRestore::showFile(const char *indent, const int count, const char *filename, const int filesize) {
  // Example: "        6240 barometr.dat"
  //          "1...5....0.. filename.ext"
  char msg[256];
  snprintf(msg, sizeof(msg), "%12d %s", filesize, filename);
  logger.info(msg);
}
void SaveRestore::showDirectory(const char *dirname) {
  // Example:  "1...5....0.. Griduino  <dir>"
  Serial.print("             ");
  Serial.print(dirname);
  Serial.println("  <dir>");

  /* --- this code hangs ---
  char msg[256];
  const int field = 20;   // total field width including string
  int w = strlen(dirname);
  int numBlanks = field - w;
  for (int ii=0; ii<numBlanks; ii++) {
    Serial.print(" ");
  }
  snprintf(msg, sizeof(msg),
           "\r\n%sDelectory of %s  <dir>", dirname);
  logger.info(msg);
  --- */
}
int SaveRestore::typeFile() {   // Echo file contents to console
  int rc   = 1;                 // assume success
  File32 f = gFatfs.open(fqFilename);
  if (!f) {
    logger.error("Error, failed to open file ", fqFilename);
    rc = 0;
  } else if (!f.isFile()) {
    logger.error("Error, ", fqFilename, " is not a file");
    rc = 0;
  } else {

    char buffer[128];
    int count = f.fgets(buffer, sizeof(buffer));
    while (count > 0) {
      Serial.print(buffer);   // print() not println(), since text files include their own CRLF
      count = f.fgets(buffer, sizeof(buffer));
    }
  }
  return rc;
}
// ----- protected helpers -----
int SaveRestore::openFlash() {
  // returns 1=success, 0=failure

  // Initialize flash library and check its chip ID.
  if (!gFlash.begin()) {
    logger.error("Error, unable to begin using Flash onboard memory");
    return 0;
  }
  // Serial.print(". Flash chip JEDEC ID: 0x"); Serial.println(gFlash.getJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!gFatfs.begin(&gFlash)) {
    logger.error("Error, failed to mount filesystem.");
    logger.error("Was the flash chip formatted with File > Examples > Adafruit SPIFlash > SdFat_format?");
    return 0;
  }

  // Check if our config data directory exists and create it if not there.
  // todo - add multilevel folder support, it currently assumes a single folder depth.
  // Note you should _not_ add a trailing slash (like '/test/') to directory names.
  // You can use the exists() function to check for the existence of a file.
  if (!gFatfs.exists(CONFIG_FOLDER)) {
    logger.info(". Configuration directory not found, creating...");
    gFatfs.mkdir(CONFIG_FOLDER);   // Use mkdir to create directory (note you should _not_ have a trailing slash)

    if (!gFatfs.exists(CONFIG_FOLDER)) {
      logger.error("Error, failed to create directory ", CONFIG_FOLDER);
      return 0;
    } else {
      logger.info(". Created directory ", CONFIG_FOLDER);
    }
  }
  return 1;   // indicate success
}
