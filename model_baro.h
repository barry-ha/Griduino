#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     model_baro.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

         The interface to this class provides:
         1. Read barometer for ongoing display        baro.getBaroPressure();
         2. Read-and-save barometer for data logger   baro.logPressure( rightnow );
         3. Load history from NVR                     baro.loadHistory();
         4. Save history to NVR                       baro.saveHistory();
         5. A few random functions as needed for unit testing

         Data logger reads BMP388 or BMP390 hardware for pressure, and gets time-of-day
         from the caller. We don't read the realtime clock in here.

  Barometric Sensor:
         Adafruit BMP388 Barometric Pressure             https://www.adafruit.com/product/3966
         Adafruit BMP390                                 https://www.adafruit.com/product/4816

  Pressure History:
         This class is basically a data logger for barometric pressure.
         Q: How many data points should we store?
         A: 288 samples
         . Assume we want 288-pixel wide graph which leaves room for graph labels on the 320-pixel TFT display
         . Assume we want one pixel for each sample, and yes this makes a pretty dense graph
         . Assume we want a 3-day display, which means 288/3 = 96 pixels (samples) per day
         . Then 24 hours / 96 pixels = 4 samples/hour = 15 minutes per sample

  Units of Time:
         This relies on "TimeLib.h" which uses "time_t" to represent time.
         The basic unit of time (time_t) is the number of seconds since Jan 1, 1970,
         a compact 4-byte integer.
         https://github.com/PaulStoffregen/Time

  Units of Pressure:
         hPa is the abbreviated name for hectopascal (100 x 1 pascal) pressure
         units which are exactly equal to millibar pressure unit (mb or mbar):

         100 Pascals = 1 hPa = 1 millibar.

         The hectopascal or millibar is the preferred unit for reporting barometric
         or atmospheric pressure in European and many other countries.
         The Adafruit BMP388 Precision Barometric Pressure sensor reports pressure
         in 'float' values of Pascals.

         In the USA and other backward countries that failed to adopt SI units,
         barometric pressure is reported as inches-mercury (inHg).

         1 pascal = 0.000295333727 inches of mercury, or
         1 inch Hg = 3386.39 Pascal
         So if you take the Pascal value of say 100734 and divide by 3386.39 you'll get 29.72 inHg.

         The BMP388 sensor has a relative accuracy of 8 Pascals, which translates to
         about +/- 0.5 meter of altitude.
*/

#include <Adafruit_BMP3XX.h>   // Precision barometric and temperature sensor
//#include <Arduino.h>           //
#include "constants.h"         // Griduino constants, colors, typedefs
#include "logger.h"            // conditional printing to Serial port

// ========== extern ===========================================
extern char *dateToString(char *msg, int len, time_t datetime);   // Griduino/Baroduino.ino
extern Logger logger;                                             // Griduino.ino

// ------------ definitions
#define MILLIBARS_PER_INCHES_MERCURY (0.02953)
#define BARS_PER_INCHES_MERCURY      (0.0338639)
#define PASCALS_PER_INCHES_MERCURY   (3386.39)
#define PASCALS_PER_HPA              (100.0)
#define HPA_PER_PASCAL               (0.01)
#define HPA_PER_INCHES_MERCURY       (33.8639)
#define INCHES_MERCURY_PER_PASCAL    (0.0002953)

// ========== class BarometerModel ======================
class BarometerModel {
public:
  // Class member variables
  Adafruit_BMP3XX *baro;   // pointer to the hardware-managing class
  int bmp_cs;              // Chip Select for BMP388 / BMP390 hardware
  float gPressure;         // pressure in Pascals
  float inchesHg;          // same pressure in inHg
  float celsius;           // internal case temperature

#define maxReadings 384                          // 384 = (4 readings/hour)*(24 hours/day)*(4 days)
#define lastIndex   (maxReadings - 1)            // index to the last element in pressure array
  BaroReading pressureStack[maxReadings] = {};   // array to hold pressure data, init filled with zeros

  // float elevCorr = 4241;          // elevation correction in Pascals
  //  use difference between altimeter setting and station pressure: https://www.weather.gov/epz/wxcalc_altimetersetting
  float elevCorr = 0;   // todo: unused for now, review and change if needed

  // Constructor - create and initialize member variables
  BarometerModel(Adafruit_BMP3XX *vBarometerObject, int vChipSelect) {
    baro   = vBarometerObject;
    bmp_cs = vChipSelect;
  }

  // init BMP388 or BMP390 barometer
  int begin(void) {
    int rc = 1;   // assume success
    // logger.fencepost("model_baro.h", __LINE__);
    if (baro->begin_SPI(bmp_cs)) {
      // logger.fencepost("model_baro.h", __LINE__);
      //  Bosch BMP388 datasheet:
      //       https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf
      //  IIR:
      //       An "infinite impulse response" filter intended to remove short-term
      //       fluctuations in pressure, e.g. caused by slamming a door or wind blowing
      //       on the sensor.
      //  Oversampling:
      //       Each oversampling step reduces noise and increases output resolution
      //       by one bit.
      //

      // ----- Settings recommended by Bosch based on use case for "handheld device dynamic"
      // https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf
      // Section 3.5 Filter Selection, page 17
      baro->setTemperatureOversampling(BMP3_NO_OVERSAMPLING);
      baro->setPressureOversampling(BMP3_OVERSAMPLING_4X);
      baro->setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_7);   // was 3, too busy
      baro->setOutputDataRate(BMP3_ODR_50_HZ);
      // logger.fencepost("model_baro.h", __LINE__);

      /*****
        // ----- Settings from Adafruit example
        // https://github.com/adafruit/Adafruit_BMP3XX/blob/master/examples/bmp3xx_simpletest/bmp3xx_simpletest.ino
        // Set up oversampling and filter initialization
        baro->setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
        baro->setPressureOversampling(BMP3_OVERSAMPLING_4X);
        baro->setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
        baro->setOutputDataRate(BMP3_ODR_50_HZ);
        *****/

      /*****
        // ----- Settings from original Barograph example
        // Set up BMP388 oversampling and filter initialization
        baro->setTemperatureOversampling(BMP3_OVERSAMPLING_2X);
        baro->setPressureOversampling(BMP3_OVERSAMPLING_32X);
        baro->setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_127);
        // baro->setOutputDataRate(BMP3_ODR_50_HZ);
        *****/

      // Get and discard the first data point
      // Repeated because first reading is always bad, until iir oversampling buffers are populated
      for (int ii = 0; ii < 4; ii++) {
        baro->performReading();   // read hardware
        delay(50);
      }

    } else {
      logger.error("Error, unable to initialize BMP388 / BMP390, check the wiring");
      rc = 0;   // return failure
    }
    // logger.fencepost("model_baro.h", __LINE__);
    return rc;
  }

  float getAltitude(float sealevelPa) {
    // input: sea level air pressure, Pascals
    // returns: altitude, meters
    return baro->readAltitude(sealevelPa / 100.0);
  }

  float getTemperature() {
    // updates: celsius (public class var)
    // query temperature from the C++ object, Celsius, not the hardware
    // note: the C++ object is updated only by "performReading()"
    celsius = baro->readTemperature();
    return celsius;
  }

  float getBaroPressure() {
    // returns: float Pascals
    // updates: gPressure (public class var)
    //          inchesHg  (public class var)
    if (!baro->performReading()) {
      logger.error("Error, failed to read barometer hardware");
    }
    // continue anyway, for demo
    gPressure = baro->pressure + elevCorr;   // Pressure is returned in SI units of Pascals. 100 Pascals = 1 hPa = 1 millibar
    // hPa = gPressure / 100;
    inchesHg = gPressure * INCHES_MERCURY_PER_PASCAL;
    // Serial.print("Barometer: "); Serial.print(gPressure); Serial.print(" Pa [model_baro.h "); Serial.print(__LINE__); Serial.println("]");  // debug
    return gPressure;
  }

  // the schedule is determined by the Controller
  // controller should call this every 15 minutes
  void logPressure(time_t rightnow) {
    if (logger.print_info) {
      float pressure = getBaroPressure();     // read
      rememberPressure(pressure, rightnow);   // push onto stack
      Serial.print("logPressure( ");          // debug
      Serial.print(pressure, 1);              // debug
      Serial.println(" )");                   // debug
    }
    saveHistory();   // write stack to NVR
  }

  // ========== load/save barometer pressure history =============
  // Filenames MUST match between Griduino and Baroduino example program
  // To erase and rewrite a new data file, change the version string below.
  const char PRESSURE_HISTORY_FILE[25]    = CONFIG_FOLDER "/barometr.dat";
  const char PRESSURE_HISTORY_VERSION[15] = "Pressure v02";
  int loadHistory() {
    return true;   // debug!!
    SaveRestore history(PRESSURE_HISTORY_FILE, PRESSURE_HISTORY_VERSION);
    BaroReading tempStack[maxReadings] = {};   // array to hold pressure data, fill with zeros
    int result                         = history.readConfig((byte *)&tempStack, sizeof(tempStack));
    if (result) {
      int numNonZero = 0;
      for (int ii = 0; ii < maxReadings; ii++) {
        // pressureStack[ii] = tempStack[ii];  // debug!
        if (pressureStack[ii].pressure > 0) {
          numNonZero++;
        }
      }

      logger.info(". Loaded barometric pressure history file, %d readings", numNonZero);
    }
    dumpPressureHistory();   // debug
    return result;
  }

  void saveHistory() {
    SaveRestore history(PRESSURE_HISTORY_FILE, PRESSURE_HISTORY_VERSION);
    history.writeConfig((byte *)&pressureStack, sizeof(pressureStack));
    logger.info("Saved pressure history to non-volatile memory");
  }

#ifdef RUN_UNIT_TESTS
  void testRememberPressure(float pascals, time_t time) {
    rememberPressure(pascals, time);
  }
#endif

protected:
  void rememberPressure(float pascals, time_t time) {
    // interface for unit test
    // push the given barometer reading onto the stack (without reading hardware)
    // shift existing stack to the left
    for (int ii = 0; ii < lastIndex; ii++) {
      pressureStack[ii] = pressureStack[ii + 1];
    }

    // put the latest pressure onto the stack
    pressureStack[lastIndex].pressure = pascals;
    pressureStack[lastIndex].time     = time;
  }

  void dumpPressureHistory() {   // debug
    // return;                      // debug debug
    //  format the barometric pressure array and write it to the Serial console log
    //  entire subroutine is for debug purposes
    logger.info("Pressure history stack, non-zero values [line %d]", __LINE__);
    if (logger.print_info) {
      for (int ii = 0; ii < maxReadings; ii++) {
        BaroReading item = pressureStack[ii];
        if (item.pressure > 0) {
          Serial.print("Stack[");
          Serial.print(ii);
          Serial.print("] = ");
          Serial.print(item.pressure);
          Serial.print("  ");
          char msg[24];
          Serial.println(dateToString(msg, sizeof(msg), item.time));   // debug
        }
      }
    }
    return;
  }

};   // end class BarometerModel
