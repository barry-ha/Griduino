#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     logger.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  "class Logger" prints to the USB port and allows the
            listener at the other end to select what messages are sent.

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

/*
enum {
  NMEA = 1,
  DEBUG,
  INFO,
  WARNING,
  ERROR,
}
*/

#include <Arduino.h>   // for "strncpy" and others
#include "logger.h"    // conditional printing to Serial port

class Logger {

public:
  bool print_nmea      = true;    // set TRUE for NmeaTime2 by www.visualgps.net
  bool print_gmt       = false;   // the time reports are frequent (1 per second) so by default it's off
  bool print_fencepost = true;
  bool print_debug     = true;
  bool print_info      = true;   // set FALSE for NmeaTime2 by www.visualgps.net
  bool print_warning   = true;
  bool print_error     = true;

  // NMEA messages, such as $GPRMC
  // this has frequent output messages (1 per second) so by default it's off
  void nmea(const char *pText) {
    if (print_nmea) {
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
      // this GPS info is required by NMEATime2 by www.visualgps
      const char haystack[] = "$GPRMC $GPGGA $GPGSA $GPGSV ";
      char needle[7];
      strncpy(needle, pText, 6);
      needle[6] = 0;   // null terminated
      Serial.print(pText);
      if (strstr("$GPRMC", needle)) {
        Serial.println();
      }
    }
  }

  // GMT time reports
  void gmt(const char *pText) {
    if (print_gmt) {
      Serial.print(pText);
    }
  }
  void fencepost(const char *pModule, const int lineno) {
    // example output: "Griduino.ino[123]"
    if (print_fencepost) {
      Serial.print(pModule);
      Serial.print("[");
      Serial.print(lineno);
      Serial.println("]");
    }
  }
  void fencepost(const char *pModule, const char *pSubroutine, const int lineno) {
    // This is used extensively in unittest.cpp
    // example input:  logger.fencepost("unittest.cpp", "subroutineName()", __LINE__);
    // example output: "----- subroutineName(), unit_test.cpp[123]"
    if (print_fencepost) {
      Serial.print("----- ");
      Serial.print(pSubroutine);
      Serial.print(", ");
      Serial.print(pModule);
      Serial.print("[");
      Serial.print(lineno);
      Serial.println("] ");
    }
  }
  void info(const char *pText) {   // one string arg
    if (print_info) {
      Serial.println(pText);
    }
  }
  void info(const char *pText, const int value) {   // one format string containing %d, one number
    if (print_info) {
      char msg[256];
      snprintf(msg, sizeof(msg), pText, value);
      Serial.println(msg);
    }
  }
  void info(const char *pText, const int value1, const int value2) {   // one format string, two numbers
    if (print_info) {
      char msg[256];
      snprintf(msg, sizeof(msg), pText, value1, value2);
      Serial.println(msg);
    }
  }
  void info(const char *pText1, const char *pText2) {   // two string args
    if (print_info) {
      Serial.print(pText1);
      Serial.println(pText2);
    }
  }
  void info(const int int1, const char *pText2) {   // linenumber, one string arg
    if (print_info) {
      Serial.print(int1);
      Serial.println(pText2);
    }
  }
  void info(const char *pText1, const char *pText2, const char *pText3) {   // three string args
    if (print_info) {
      Serial.print(pText1);
      Serial.print(pText2);
      Serial.println(pText3);
    }
  }
  void info(const int int1, const char *pText2, const char *pText3) {   // linenumber, two string args
    if (print_info) {
      Serial.print(int1);
      Serial.print(pText2);
      Serial.println(pText3);
    }
  }
  void debug(const char *pText) {
    if (print_debug) {
      Serial.println(pText);
    }
  }
  void warning(const char *pText) {
    if (print_warning) {
      Serial.println(pText);
    }
  }
  void error(const char *pText) {   // one string arg
    if (print_error) {
      Serial.println(pText);
    }
  }
  void error(const char *pText1, const char *pText2) {   // two string args
    if (print_error) {
      Serial.print(pText1);
      Serial.println(pText2);
    }
  }
  void error(const char *pFormatString, const int val1, const int val2 = 0) {   // format string, one or two numbers
    if (print_error) {
      char msg[256];
      snprintf(msg, sizeof(msg), pFormatString, val1, val2);
      Serial.println(msg);
    }
  }
  void error(const char *pText1, const char *pText2, const char *pText3) {   // three string args
    if (print_error) {
      Serial.print(pText1);
      Serial.print(pText2);
      Serial.println(pText3);
    }
  }

protected:
  // nothing yet

};   // end class Logger
