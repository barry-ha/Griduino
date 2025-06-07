#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     model_baro.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

         The interface to this class provides:
         1. Constructor                               BarometerModel baroModel();
         2. Init ourself and our hardware             begin();
         3. Read barometer for ongoing display        baro.getBaroPressure();
         4. Read-and-save barometer for data logger   baro.logPressure( rightnow );
         5. Load history from NVR                     baro.loadHistory();
         6. Save history to NVR                       baro.saveHistory();
         7. A few minor functions for unit tests

         Data logger reads BMP280 or BMP388 or BMP390 hardware for pressure.
         We get time-of-day from the caller. We don't read the realtime clock.

  Barometric Sensor:
         Goal is to hide hardware from the caller via conditional compile.
         Adafruit BMP280 Barometric Pressure          https://www.adafruit.com/product/2651
         Adafruit BMP388 Barometric Pressure          https://www.adafruit.com/product/3966
         Adafruit BMP390                              https://www.adafruit.com/product/4816

         Bosch BMP280 datasheet:    https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf
         Bosch BMP388 datasheet:    https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf

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

#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
#include <Wire.h>
#include <Adafruit_BMP280.h>   // Precision barometric and temperature sensor
#include <Adafruit_Sensor.h>
#else
#include <Adafruit_BMP3XX.h>   // Precision barometric and temperature sensor
#endif
#include "constants.h"     // Griduino constants, colors, typedefs
#include "logger.h"        // conditional printing to Serial port
#include "date_helper.h"   // date/time conversions

// ========== extern ===========================================
extern Logger logger;   // Griduino.ino
extern Dates date;      // for "datetimeToString()", Griduino.ino

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
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  // BMP280 Constructor
  Adafruit_BMP280 baro;   // has-a hardware-managing class object, use I2C interface
  BarometerModel() : baro(&Wire1) {}
#else
  // BMP388 and BMP390 Constructor
  Adafruit_BMP3XX baro;   // has-a hardware-managing class object, use SPI or I2C interface
  BarometerModel(int vChipSelect = BMP_CS) {
    bmp_cs = vChipSelect;
  }
#endif
  int bmp_cs;        // Chip Select for BMP388 / BMP390 hardware
  float gPressure;   // pressure in Pascals
  float inchesHg;    // same pressure in inHg
  float celsius;     // internal case temperature

#define maxReadings 384                          // 384 = (4 readings/hour)*(24 hours/day)*(4 days)
#define lastIndex   (maxReadings - 1)            // index to the last element in pressure array
  BaroReading pressureStack[maxReadings] = {};   // array to hold pressure data, init filled with zeros

  // float elevCorr = 4241;          // elevation correction in Pascals
  //  use difference between altimeter setting and station pressure: https://www.weather.gov/epz/wxcalc_altimetersetting
  float elevCorr = 0;   // todo: unused for now, review and change if needed

  // ----- init BMP380, BMP388 or BMP390 barometer
  int begin(void) {
    int rc = 1;   // assume success

    bool initialized = baro.begin_SPI(bmp_cs);   // Griduino v4 pcb, SPI
    if (initialized) {
      // success - SPI
      logger.log(BARO, INFO, "successfully initialized barometric sensor with SPI");

    } else {
      initialized = baro.begin_I2C(BMP3XX_DEFAULT_ADDRESS, &Wire);   // Griduino v7+ pcb, I2C
      if (initialized) {
        // success - I2C
        logger.log(BARO, INFO, "successfully initialized barometric sensor with I2C");
      } else {
        // failed
        logger.log(BARO, ERROR, "unable to initialize Bosch pressure sensor");
      }
    }

    if (initialized) {
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
      /*
      baro->setTemperatureOversampling(BMP3_NO_OVERSAMPLING);
      baro->setPressureOversampling(BMP3_OVERSAMPLING_4X);
      baro->setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_7);   // was 3, too busy
      baro->setOutputDataRate(BMP3_ODR_50_HZ);
      // logger.fencepost("model_baro.h", __LINE__);
        *****/

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
        baro.performReading();   // read hardware
        delay(25);
      }

    } else {
      uint8_t id = baro.chipID();
      switch (id) {
      case 0x00:
        logger.log(BARO, ERROR, "ID = 0x00 = no response from pressure sensor hardware");
        break;
      case 0xFF:
        logger.log(BARO, ERROR, "ID = 0xFF = a bad address, a BMP 180 or BMP 085");
        break;
      case 0x56:
      case 0x57:
      case 0x58:
        logger.log(BARO, ERROR, "ID = 0x56-0x58 = a BMP 280");
        break;
      case 0x60:
        logger.log(BARO, ERROR, "ID = 0x60 = a BME 280");
        break;
      case 0x61:
        logger.log(BARO, ERROR, "ID = 0x61 = a BME 680");
        break;
      default:
        logger.log(BARO, ERROR, "ID = %d = not a recognized pressure sensor", id);
        break;
      }
      rc = 0;   // return failure
    }
    return rc;
  }

  float getAltitude(float sealevelPa) {
    // input: sea level air pressure, Pascals
    // returns: altitude, meters
    return baro.readAltitude(sealevelPa / 100.0);
  }

  float getTemperature() {
    // updates: celsius (public class var)
    // query temperature from the C++ object, Celsius, not the hardware
    celsius = baro.readTemperature();
    return celsius;
  }

  float getBaroPressure() {
    // returns: float Pascals
    // updates: gPressure (public class var)
    //          inchesHg  (public class var)
    // continue anyway, for demo
    gPressure = baro.readPressure();   // Pressure is returned in SI units of Pascals. 100 Pascals = 1 hPa = 1 millibar
    inchesHg  = gPressure * INCHES_MERCURY_PER_PASCAL;
    return gPressure;
  }

  // the schedule is determined by the Controller
  // controller should call this every 15 minutes
  void logPressure(time_t rightnow) {
    float pressure = getBaroPressure();                       // read
    rememberPressure(pressure, rightnow);                     // push onto stack
    logger.log(BARO, INFO, "logPressure(%s)", pressure, 1);   // debug
    saveHistory();                                            // write stack to NVR
  }

  // ========== load/save barometer pressure history =============
  // Filenames MUST match between Griduino and Baroduino example program
  // To erase and rewrite a new data file, change the version string below.
  const char PRESSURE_HISTORY_FILE[25]    = CONFIG_FOLDER "/barometr.dat";
  const char PRESSURE_HISTORY_VERSION[15] = "Pressure v02";
  int loadHistory() {
    logger.log(BARO, ERROR, "model_baro.h: loadHistory() is disabled");
    return true;   // debug!! todo!!! help!!!!
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

      logger.log(BARO, INFO, ". Loaded barometric pressure history file, %d readings", numNonZero);
    }
    dumpPressureHistory();   // debug
    return result;
  }

  void saveHistory() {
    SaveRestore history(PRESSURE_HISTORY_FILE, PRESSURE_HISTORY_VERSION);
    history.writeConfig((byte *)&pressureStack, sizeof(pressureStack));
    logger.log(BARO, INFO, "Saved pressure history to non-volatile memory");
  }

  void testRememberPressure(float pascals, time_t time) {
    rememberPressure(pascals, time);
  }

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
    //  format the barometric pressure array and write it to the Serial console log
    //  entire subroutine is for debug purposes
    for (int ii = 0; ii < maxReadings; ii++) {
      BaroReading item = pressureStack[ii];
      if (item.pressure > 0) {
        char sDate[24], sPressure[24], out[128];

        Serial.println(date.datetimeToString(sDate, sizeof(sDate), item.time));
        floatToCharArray(sPressure, sizeof(sPressure), item.pressure, 4);
        snprintf(out, sizeof(out), "Stack[%d] = %s  %s", ii, sPressure, sDate);

        logger.log(BARO, DEBUG, out);
      }
    }
    return;
  }

};   // end class BarometerModel
