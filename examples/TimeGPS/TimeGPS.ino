/*
  TimeGPS -- demonstrate reading time-of-day from GPS
             and usage of TimeLib for date operations.
             This will become the basis for graphing
             barometric pressure, which relies heavily on
             date calculations.

  Date:     2019-08-30 created from example BorisNeubert / Time

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

*/

#include <Adafruit_GPS.h>   // Ultimate GPS library
#include "constants.h"      // Griduino constants, colors, typedefs
#include <TimeLib.h>

// ------- Identity for splash screen and console --------
#define TIMEGPS_TITLE "TimeGPS"

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see constants.h

// Hardware serial port for GPS
Adafruit_GPS GPS(&Serial1);

// ------------ definitions
const int howLongToWait = 8;   // max number of seconds at startup waiting for Serial port to console

// Offset hours from gps time (UTC)
// const int offset = 1;   // Central European Time
// const int offset = -5;  // Eastern Standard Time (USA)
// const int offset = -4;  // Eastern Daylight Time (USA)
// const int offset = -8;  // Pacific Standard Time (USA)
const int offset = -7;   // Pacific Daylight Time (USA)

// Ideally, it should be possible to learn the time zone
// based on the GPS position data.  However, that would
// require a complex library, probably incorporating some
// sort of database using Eric Muller's time zone shape
// maps, at http://efele.net/maps/tz/

time_t prevDisplay = 0;   // when the digital clock was displayed

//=========== time helpers =================================
// Did the GPS report a valid date?
bool isDateValid(int yy, int mm, int dd) {
  if (yy < 2020) {
    return false;
  }
  if (mm < 1 || mm > 12) {
    return false;
  }
  if (dd < 1 || dd > 31) {
    return false;
  }
  return true;
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10) {
    Serial.print('0');
  }
  Serial.print(digits);
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(month());
  Serial.print("-");
  Serial.print(day());
  Serial.print("-");
  Serial.print(year());
  Serial.print("  ");

  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println(" ");
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong * 1000;
  while (millis() < targetTime) {
    if (Serial)
      break;
  }
}

//=========== setup ============================================
void setup() {

  // ----- init serial monitor
  Serial.begin(115200);           // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(TIMEGPS_TITLE " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name

  // ----- init GPS
  GPS.begin(9600);                        // 9600 NMEA is the default baud rate for Adafruit MTK GPS's
  delay(200);                             // is delay really needed?
  GPS.sendCommand(PMTK_SET_BAUD_57600);   // set baud rate to 57600
  delay(200);
  GPS.begin(57600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);   // turn on RMC (recommended minimum) and GGA (fix data) including altitude

  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  // GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ); // Once every 5 seconds update

  Serial.println("Waiting for GPS time ... ");
}

//=========== main work loop ===================================

void loop() {

  GPS.read();   // if you can, read the GPS serial port every millisecond in an interrupt
  if (GPS.newNMEAreceived()) {
    // sentence received -- verify checksum, parse it
    // GPS parsing: https://learn.adafruit.com/adafruit-ultimate-gps/parsed-data-output
    // In this "TimeGPS" program, all we need is the RTC date and time, which is sent
    // from the GPS module to the Arduino as NMEA sentences.
    if (GPS.parse(GPS.lastNMEA())) {
      // when TinyGPS reports new data...
      unsigned long age;
      int Year;
      // byte Month, Day, Hour, Minute, Second;
      // gps.crack_datetime(&Year, &Month, &Day, &Hour, &Minute, &Second, NULL, &age);
      if (true) /*(age < 500)*/ {
        // set the Time to the latest GPS reading
        setTime(GPS.hour, GPS.minute, GPS.seconds, GPS.day, GPS.month, GPS.year);
        adjustTime(offset * SECS_PER_HOUR);   // update to our internal selected time zone
      }
    } else {
      // parsing failed -- restart main loop to wait for another sentence
      // this also sets the newNMEAreceived() flag to false
      return;
    }
  }

  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) {                    // update the display only if the time has changed
      if (isDateValid(year(), month(), day())) {   // update the display only if the time is valid
        digitalClockDisplay();
      } else {
        Serial.print("Still waiting because date = ");
        Serial.print(month());
        Serial.print("-");
        Serial.print(day());
        Serial.print("-");
        Serial.println(year());
      }
      prevDisplay = now();
    }
  }
}
