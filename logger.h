#pragma once   // Please format this file with clang before check-in to GitHub
//------------------------------------------------------------------------------
//  File name: Logger.h
//
//  Description: "class Logger" prints to the USB port and allows the
//               listener at the other end to select what messages are sent.
//
//------------------------------------------------------------------------------
//  The MIT License (MIT)
//
//  Copyright (c) 2022 Barry Hansen K7BWH
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//------------------------------------------------------------------------------
/*
enum {
  NMEA = 1,
  DEBUG,
  INFO,
  WARNING,
  ERROR,
}
*/

class Logger {

public:
  bool print_nmea    = false;
  bool print_gmt     = false;
  bool print_debug   = true;
  bool print_info    = true;
  bool print_warning = true;
  bool print_error   = true;

  // NMEA messages, such as $GPRMC
  // this is noisy with 1-per-second so by default it's off
  void nmea(const char *pText) {
    if (print_nmea) {
      // 2022-01-02 for now, all we need is $GPRMC not $GPGGA
      //
      //    GPRMC data:       GPGGA data:
      //    hh:mm:ss          hh:mm:ss
      //    Sat status        Latitude
      //    Latitude          Longitude
      //    Longitude         Fix quality
      //    Ground speed      No. of sats
      //    Track angle       Ground speed
      //    DD:MM:YY          Altitude
      //
      if (strncmp("$GPRMC", pText, 6) == 0) {
        Serial.print(pText);
      }
    }
  }

  // GMT time reports
  // this is noisy with 1-per-second so by default it's off
  void gmt(const char *pText) {
    if (print_gmt) {
      Serial.print(pText);
    }
  }

  void info(const char *pText) {
    if (print_info) {
      Serial.print(pText);
    }
  }
  void debug(const char *pText) {
    if (print_debug) {
      Serial.print(pText);
    }
  }
  void warning(const char *pText) {
    if (print_warning) {
      Serial.print(pText);
    }
  }
  void error(const char *pText) {
    if (print_error) {
      Serial.print(pText);
    }
  }

protected:
  // nothing yet
  
};   // end class Logger
