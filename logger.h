#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     logger.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  "class Logger" prints messages to the console (a USB port) and allows
            the listener at the other end to select what messages are sent.

            General design:   void subsystem(enum eLogLevel level, const char *msg);
            Example usage:    logger.log(SUBSYSTEM, INFO, "Hi mom");

            Because logging can change system timing, sometimes the logging
            needs to be turned off globally:
                void logGlobalOn();
                void logGlobalOff();

 ******************************************************************************
  Licensed under the GNU General Public License v3.0

  Permissions of this strong copyleft license are conditioned on making available
  complete source code of licensed works and modifications, which include larger
  works using a licensed work, under the same license. Copyright and license
  notices must be preserved. Contributors provide an express grant of patent rights.

  You may obtain a copy of the License at
      https://www.gnu.org/licenses/gpl-3.0.en.html

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

// ----- Severity Level
enum LogLevel {
  DEBUG = 0,   // verbose
  POST,        // fencepost debug
  INFO,        // non critical
  WARNING,     // important
  ERROR,       // critical
  CONSOLE,     // required output
  numLevels,   // array size
};

// ----- Subsystem
enum LogSystem {
  NMEA = 0,     //
  GMT,          //
  FENCE,        // fencepost debug
  COMMAND,      //
  GPS_SETUP,    //
  CONFIG,       // is used in all modules named "cfg_xxxxx"
  BARO,         // barometric pressure sensor
  AUDIO,        // morse code and speech output
  BATTERY,      // coin battery for GPS
  FILES,        // all file handling, breadcrumbs, save/restore
  TIME,         // RTC, dates, calendar, events
  SCREEN,       // TFT display
  numSystems,   // array size
};

#include <Arduino.h>   // for "strncpy" and others
#include "logger.h"    // conditional printing to Serial port

// extern
void floatToCharArray(char *result, int maxlen, double fValue, int decimalPlaces);   // Griduino.ino

struct systemDef {
  bool enabled;
  char name[5];
};

class Logger {

public:
  // master on/off switch
  bool log_enabled = true;

  // subsystems
  systemDef printSystem[numSystems] = {
      {true, "NMEA"},    // NMEA = 0,     // MUST be in same order as enum
      {false, "GMT "},   // GMT,
      {true, "FENC"},    // FENCE,        // fencepost debug
      {false, "CMD "},   // COMMAND,
      {true, "GPS "},    // GPS_SETUP,
      {false, "CFG "},   // CONFIG,       // is used in all modules named "cfg_xxxxx"
      {false, "BARO"},   // BARO,         // barometric pressure sensor
      {false, "AUD "},   // AUDIO,        // morse code and speech output
      {false, "BATT"},   // BATTERY,      // coin battery for GPS
      {true, "FILE"},    // FILES,        // all file handling, including save/restore
  };

  // severities
  bool printLevel[numLevels] = {
      true,   // DEBUG = 0,   // verbose
      true,   // FENCE,       // fencepost debug
      true,   // INFO,        // non critical
      true,   // WARNING,     // important
      true,   // ERROR,       // critical
      true,   // CONSOLE,     // required output
  };

  void setLevel(LogLevel lvl) {
    if (lvl >= numLevels) {
      log(COMMAND, ERROR, "Level %d is not allowed", lvl);
    }
    for (int ii = 0; ii < numLevels; ii++) {
      if (ii < lvl) {
        printLevel[ii] = false;
      } else {
        printLevel[ii] = true;
      }
    }
  }

  // ---------- categories ----------
  // NMEA messages, such as $GPRMC
  // this has frequent output messages (1 per second) so by default it's off
  // todo - delete, unused
  void nmea(LogLevel severity, const char *pText) {
    if (ok_to_log(NMEA, severity)) {
      //
      //    ---GPRMC---         ---GPGGA---
      //    hh:mm:ss              hh:mm:ss
      //    Sat status            Latitude
      //    Latitude              Longitude
      //    Longitude             Fix quality
      //    Ground speed (knots)  No. of sats in use
      //    Track angle           Altitude (m)
      //    DD:MM:YY
      //
      // these NEMA sentences are used by NMEATime2 by www.visualgps.com
      const char haystack[] = "$GPRMC $GPGGA $GPGSA $GPGSV ";
      char needle[7];
      strncpy(needle, pText, 6);
      needle[6] = 0;   // null terminated
      Serial.print(pText);
      if (strstr("$GPRMC", needle)) {
        Serial.println();   // insert blank line after $GPRMC
      }
    }
  }

  // ----- general log: text-only
  void log(LogSystem system, LogLevel severity, const char *pText) {
    if (ok_to_log(system, severity)) {

      printPrefix(system, severity);
      if (strlen(pText) < 4) {
        // allow caller to send up to this many newlines together
        Serial.print(pText);
      } else {
        if (pText[strlen(pText) - 1] == '\n') {
          // use the trailing newline from NEMA sentences from the GPS module
          Serial.print(pText);
        } else {
          // add our own newline
          Serial.println(pText);
        }
      }
    }
  }

  // ----- general log: text + text
  void log(LogSystem system, LogLevel severity, const char *pFormat, const char *pStr) {
    if (ok_to_log(system, severity)) {

      printPrefix(system, severity);
      char msg[256];
      snprintf(msg, sizeof(msg), pFormat, pStr);
      Serial.println(msg);
    }
  }

  // ----- general log: text + text + text
  void log(LogSystem system, LogLevel severity, const char *pFormat, const char *pStr1, const char *pStr2) {
    if (ok_to_log(system, severity)) {

      printPrefix(system, severity);
      char msg[256];
      snprintf(msg, sizeof(msg), pFormat, pStr1, pStr2);
      Serial.println(msg);
    }
  }

  // ----- general log: text + number
  void log(LogSystem system, LogLevel severity, const char *pFormat, const int value) {
    if (ok_to_log(system, severity)) {

      printPrefix(system, severity);
      char msg[256];
      snprintf(msg, sizeof(msg), pFormat, value);
      Serial.println(msg);
    }
  }

  // ----- general log: text + hexadecimal
  /*
  void logHex(LogSystem system, LogLevel severity, const char *pFormat, const int value) {
    if (ok_to_log(system, severity)) {

      printPrefix(system, severity);

      char msg[256];
      snprintf(msg, sizeof(msg), pFormat, value);
      Serial.println(msg);
    }
  }
  */

  bool ok_to_log(LogSystem system, LogLevel severity) {
    if (severity == CONSOLE) {
      return true;
    }

    if (log_enabled) {
      // check that this severity level AND system component is enabled
      if (printLevel[severity]) {
        if (printSystem[system].enabled) {
          return true;
        }
      }
    }
    return false;
  }

  // ----- general log: text + int + int
  void log(LogSystem system, LogLevel severity, const char *pFormat, const int value1, const int value2) {
    if (ok_to_log(system, severity)) {

      printPrefix(system, severity);
      char msg[256];
      snprintf(msg, sizeof(msg), pFormat, value1, value2);
      Serial.println(msg);
    }
  }

  // ----- general log: text + float
  void logFloat(LogSystem system, LogLevel severity, const char *pFormat, const float value1, const int decimalPlaces) {
    if (ok_to_log(system, severity)) {

      // print 'error' and 'warning' if needed
      printPrefix(system, severity);

      // print the given text and value
      char msg[256];
      char sFloat[8];
      floatToCharArray(sFloat, sizeof(sFloat), value1, decimalPlaces);
      snprintf(msg, sizeof(msg), pFormat, sFloat);
      Serial.println(msg);
    }
  }

  // ----- general log: text + float
  void logTwoFloats(LogSystem system, LogLevel severity, const char *pFormat, const float value1, const int places1, const float value2, const int places2) {
    if (ok_to_log(system, severity)) {

      // print 'error' and 'warning' if needed
      printPrefix(system, severity);

      // print the given text and value
      char sFloat1[8], sFloat2[8];
      floatToCharArray(sFloat1, sizeof(sFloat1), value1, places1);
      floatToCharArray(sFloat2, sizeof(sFloat2), value2, places2);

      char msg[256];
      snprintf(msg, sizeof(msg), pFormat, sFloat1, sFloat2);
      Serial.println(msg);
    }
  }

  // ----- debug helper
  void dumpHex(LogSystem system, LogLevel severity, const char *text, char *buff, int len) {
    if (ok_to_log(system, severity)) {
      // debug helper to put hexadecimal data on console
      char out[128];
      snprintf(out, sizeof(out), ". %s [%d] : '%s' : ", text, len, buff);
      Serial.print(out);
      for (int i = 0; i < len; i++) {
        sprintf(out, "%02x ", buff[i]);
        Serial.print(out);
      }
      Serial.println("");
    }
  }

  // ----- debug statements to confirm a certain line of code is executed
  void fencepost(const char *pModule, const int lineno) {
    // const LogSystem sys = FENCE;
    // const LogLevel sev = POST;
    // if (ok_to_log(sys, sev)) {
    //   printPrefix(sys, sev);
    //  example output: "Griduino.ino[123]"
    char msg[128];
    snprintf(msg, sizeof(msg), "%s[%d]", pModule, lineno);
    log(FENCE, POST, msg);
    //}
  }
  void fencepost(const char *pModule, const char *pSubroutine, const int lineno) {
    // This is used extensively in unittest.cpp
    // example input:  logger.fencepost("unittest.cpp", "subroutineName()", __LINE__);
    // example output: "----- subroutineName(), unit_test.cpp[123]"
    char msg[128];
    snprintf(msg, sizeof(msg), "----- %s, %s[%d] ", pSubroutine, pModule, lineno);
    log(FENCE, POST, msg);
  }

  // ---------- print direct to console
  // logger.print() is used when text MUST be sent out to the console.
  // For example, when the console sends us a 'help' request
  // then we must reply by printing to the console, no matter what.
  // Note: This function does not send \n newline.
  //       logger.print() is a direct replacement for Serial.print()
  void print(const char *pText) {
    Serial.print(pText);
  }
  void print(int value) {
    Serial.print(value);
  }
  void print(long value) {
    Serial.print(value);
  }
  void print(float f, int places) {
    Serial.print(f, places);
  }
  void println() {
    Serial.println();
  }
  void println(long value) {
    Serial.println(value);
  }
  void println(const char *pText) {
    Serial.println(pText);
  }

protected:
  // helper: issue prefix for warnings and errors
  void printPrefix(LogSystem system, LogLevel severity) {
    char pfx[32];
    switch (severity) {
    case DEBUG:
      snprintf(pfx, sizeof(pfx), "(d, %s) ", printSystem[system].name);
      Serial.print(pfx);
      break;
    case POST:
      snprintf(pfx, sizeof(pfx), "(fencepost) ", printSystem[system].name);
      Serial.print(pfx);
      break;
    case INFO:
      snprintf(pfx, sizeof(pfx), "(i, %s) ", printSystem[system].name);
      Serial.print(pfx);
      break;
    case WARNING:
      snprintf(pfx, sizeof(pfx), "%s Warning, ", printSystem[system].name);
      Serial.print(pfx);
      break;
    case ERROR:
      snprintf(pfx, sizeof(pfx), "%s Error, ", printSystem[system].name);
      Serial.print(pfx);
      break;
    case CONSOLE:
      snprintf(pfx, sizeof(pfx), "(c, %s) ", printSystem[system].name);
      Serial.print(pfx);
      break;
    default:
      Serial.print("(severity?) ");
      break;
    }
  }

};   // end class Logger
