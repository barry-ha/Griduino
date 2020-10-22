#pragma once
/*
  File:     model_baro.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA


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

#include <Arduino.h>
#include "constants.h"                // Griduino constants, colors, typedefs
//#include "save_restore.h"           // Configuration data in nonvolatile RAM

// ========== extern ===========================================

// ------------ typedef's
class Reading {
  public:
    float pressure;             // in millibars, from BMP388 sensor
    time_t time;                // in GMT, from realtime clock 
};

// ------------ definitions
#define MILLIBARS_PER_INCHES_MERCURY (0.02953)
#define BARS_PER_INCHES_MERCURY      (0.0338639)
#define PASCALS_PER_INCHES_MERCURY   (3386.39)

// ========== class BarometerModel ======================
class BarometerModel {
  public:
    // Class member variables
    void process() {
      getBaroData();

    }

    // init BMP388 hardware 
    int begin(void) {
      int rc = 1;                     // assume success
      if (baro.begin()) {
        // Set up BMP388 oversampling and filter initialization
        baro.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);
        baro.setPressureOversampling(BMP3_OVERSAMPLING_32X);
        baro.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_127);
        // baro.setOutputDataRate(BMP3_ODR_50_HZ);

        // Get and discard the first data point (repeated because first reading is always bad)
        for (int ii=0; ii<4; ii++) {
          baro.performReading();
          delay(50);
        }

      } else {
        Serial.println("Error, unable to initialize BMP388, check the wiring");
        rc = 0;                       // return failure
      }
      return rc;
    }

  protected:
    void getBaroData() {
      // returns: gPressure (global var)
      //          hPa       (global var)
      //          inchesHg  (global var)
      if (!baro.performReading()) {
        Serial.println("Error, failed to read barometer");
      }
      // continue anyway, for demo
      gPressure = baro.pressure + elevCorr;   // Pressure is returned in SI units of Pascals. 100 Pascals = 1 hPa = 1 millibar
      hPa = gPressure / 100;
      inchesHg = 0.0002953 * gPressure;
      Serial.print("Barometer ");
      Serial.print(gPressure);
      Serial.print(" Pa [");
      Serial.print(__LINE__);
      Serial.println("]");
    }

    void rememberPressure( float pressure, time_t time ) {
      // push the barometer reading onto the stack 
      // shift existing stack to the left
      for (int ii = 0; ii < lastIndex; ii++) {
        pressureStack[ii] = pressureStack[ii + 1];
      }
  
      // put the latest pressure onto the stack
      pressureStack[lastIndex].pressure = pressure;
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


  public:
    // Constructor - create and initialize member variables
    BarometerModel() { }

    // Setters

    // ========== load/save barometer pressure readings ============
    // Filenames MUST match between Griduino and Baroduino example program
    // To erase and rewrite a new data file, change the version string below.
    const char PRESSURE_HISTORY_FILE[25] = CONFIG_FOLDER "/barometr.dat";
    const char PRESSURE_HISTORY_VERSION[15] = "Pressure v01";
    int loadPressureHistory() {
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

    void savePressureHistory() {
      SaveRestore history(PRESSURE_HISTORY_FILE, PRESSURE_HISTORY_VERSION);
      history.writeConfig( (byte*) &pressureStack, sizeof(pressureStack) );
      Serial.print("Saved the pressure history to non-volatile memory [line "); Serial.print(__LINE__); Serial.println("]");
    }

    // the Model will update its internal state on a schedule determined by the Controller
    void processBarometer() {
      // todo 
    }

    void remember(PointGPS vLoc, int vHour, int vMinute, int vSecond) {
      // todo 
    }
    void dumpHistory() {
      // todo, or delete 
    }

};  // end class BarometerModel 
