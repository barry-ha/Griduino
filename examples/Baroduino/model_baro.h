#pragma once
/*
  File:     model_baro.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

         The interface to this class provides:
         1. Read barometer for ongoing display        baro.getBaroData();
         2. Read-and-save barometer for data logger   baro.logPressure( rightnow );
         3. Load history from NVR                     baro.loadHistory();
         4. Save history to NVR                       baro.saveHistory();
         5. A few random functions as needed for unit testing

         Data logger reads BMP388 hardware for pressure, and gets time-of-day
         from the caller. We don't read the realtime clock in here.

  Barometric Sensor:
         Adafruit BMP388 Barometric Pressure             https://www.adafruit.com/product/3966

  Pressure History:
         This class is basically a data logger for barometric pressure.
         Q: How many data points should we store?
         A: 288 samples
         . Assume we want 288-pixel wide graph which leaves room for graph labels on the 320-pixel TFT display
         . Assume we want one pixel for each sample, and yes this makes a pretty dense graph
         . Assume we want a 3-day display, which means 288/3 = 96 pixels (samples) per day
         . Then 24 hours / 96 pixels = 4 samples/hour = 15 minutes per sample

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

  Units of Time:
         This relies on "TimeLib.h" which uses "time_t" to represent time.
         The basic unit of time (time_t) is the number of seconds since Jan 1, 1970, 
         a compact 4-byte integer.
         https://github.com/PaulStoffregen/Time
*/

#include <Arduino.h>
#include "constants.h"                // Griduino constants, colors, typedefs

// ========== extern ===========================================
extern char* dateToString(char* msg, int len, time_t datetime);  // Griduino/Baroduino.ino

// ------------ definitions
#define MILLIBARS_PER_INCHES_MERCURY (0.02953)
#define BARS_PER_INCHES_MERCURY      (0.0338639)
#define PASCALS_PER_INCHES_MERCURY   (3386.39)
#define INCHES_MERCURY_PER_PASCAL    (0.0002953)

// ========== class BarometerModel ======================
class BarometerModel {
  public:
    // Class member variables
    Adafruit_BMP3XX* baro;            // pointer to the hardware-managing class 
    float inchesHg;
    float gPressure;
    float hPa;
    float feet;

    #define maxReadings 288           // 288 = (4 readings/hour)*(24 hours/day)*(3 days)
    #define lastIndex (maxReadings - 1)  // index to the last element in pressure array
    BaroReading pressureStack[maxReadings] = {};  // array to hold pressure data, init filled with zeros

    //float elevCorr = 4241;          // elevation correction in Pa, 
    // use difference between altimeter setting and station pressure: https://www.weather.gov/epz/wxcalc_altimetersetting
    float elevCorr = 0;               // todo: unused for now, review and change if needed

    // Constructor - create and initialize member variables
    BarometerModel(Adafruit_BMP3XX* vbaro) {
      baro = vbaro;
    }

    // init BMP388 hardware 
    int begin(void) {
      int rc = 1;                     // assume success
      if (baro->begin()) {
        // Bosch BMP388 datasheet:
        //      https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf
        // IIR: 
        //      An "infinite impulse response" filter intended to remove short-term 
        //      fluctuations in pressure, e.g. caused by slamming a door or wind blowing 
        //      on the sensor.
        // Oversampling: 
        //      Each oversampling step reduces noise and increases output resolution 
        //      by one bit.
        //

        // ----- Settings recommended by Bosch based on use case for "handheld device dynamic"
        // https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf
        // Section 3.5 Filter Selection, page 17
        baro->setTemperatureOversampling(BMP3_NO_OVERSAMPLING);
        baro->setPressureOversampling(BMP3_OVERSAMPLING_4X);
        baro->setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_7);     // was 3, too busy
        baro->setOutputDataRate(BMP3_ODR_50_HZ);

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
        for (int ii=0; ii<4; ii++) {
          baro->performReading();     // read hardware 
          delay(50);
        }

      } else {
        Serial.println("Error, unable to initialize BMP388, check the wiring");
        rc = 0;                       // return failure
      }
      return rc;
    }

    float getBaroData() {
      // returns: float Pascals 
      // updates: gPressure (class var)
      //          hPa       (class var)
      //          inchesHg  (class var)
      if (!baro->performReading()) {
        Serial.println("Error, failed to read barometer");
      }
      // continue anyway, for demo
      gPressure = baro->pressure + elevCorr;   // Pressure is returned in SI units of Pascals. 100 Pascals = 1 hPa = 1 millibar
      hPa = gPressure / 100;
      inchesHg = 0.0002953 * gPressure;
      Serial.print("Pressure "); Serial.print(gPressure); Serial.println(" Pa");
      return gPressure;
    }

    // the schedule is determined by the Controller
    // controller should call this every 15 minutes
    void logPressure(time_t rightnow) {
      float pressure = getBaroData();           // read
      rememberPressure( pressure, rightnow );   // push onto stack
      Serial.print("logPressure( "); Serial.print(pressure,1); Serial.println(" )");  // debug
      saveHistory();                    // write stack to NVR
    }
    
    // ========== load/save barometer pressure history =============
    // Filenames MUST match between Griduino and Baroduino example program
    // To erase and rewrite a new data file, change the version string below.
    const char PRESSURE_HISTORY_FILE[25] = CONFIG_FOLDER "/barometr.dat";
    const char PRESSURE_HISTORY_VERSION[15] = "Pressure v01";
    int loadHistory() {
      SaveRestore history(PRESSURE_HISTORY_FILE, PRESSURE_HISTORY_VERSION);
      BaroReading tempStack[maxReadings] = {};      // array to hold pressure data, fill with zeros
      int result = history.readConfig( (byte*) &tempStack, sizeof(tempStack) );
      if (result) {
        int numNonZero = 0;
        for (int ii=0; ii<maxReadings; ii++) {
          pressureStack[ii] = tempStack[ii];
          if (pressureStack[ii].pressure > 0) {
            numNonZero++;
          }
        }
    
        Serial.print(". Loaded barometric pressure history file, ");
        Serial.print(numNonZero);
        Serial.println(" readings found");
      }
      dumpPressureHistory();              // debug
      return result;
    }

    void saveHistory() {
      SaveRestore history(PRESSURE_HISTORY_FILE, PRESSURE_HISTORY_VERSION);
      history.writeConfig( (byte*) &pressureStack, sizeof(pressureStack) );
      Serial.print("Saved the pressure history to non-volatile memory [line "); Serial.print(__LINE__); Serial.println("]");
    }

#ifdef RUN_UNIT_TESTS
    void testRememberPressure( float pascals, time_t time ) {
      rememberPressure(pascals, time);
    }
#endif

  protected:
    void rememberPressure( float pascals, time_t time ) {
      // interface for unit test
      // push the given barometer reading onto the stack (without reading hardware)
      // shift existing stack to the left
      for (int ii = 0; ii < lastIndex; ii++) {
        pressureStack[ii] = pressureStack[ii + 1];
      }
  
      // put the latest pressure onto the stack
      pressureStack[lastIndex].pressure = pascals;
      pressureStack[lastIndex].time = time;
    }

    void dumpPressureHistory() {            // debug
      Serial.print("Pressure history stack, non-zero values [line "); Serial.print(__LINE__); Serial.println("]");
      for (int ii=0; ii<maxReadings; ii++) {
        BaroReading item = pressureStack[ii];
        if (item.pressure > 0) {
          Serial.print("Stack["); Serial.print(ii); Serial.print("] = ");
          Serial.print(item.pressure);
          Serial.print("  ");
          char msg[24];
          Serial.println( dateToString(msg, sizeof(msg), item.time) );                          // debug
        }
      }
      return;
    }

};  // end class BarometerModel 
