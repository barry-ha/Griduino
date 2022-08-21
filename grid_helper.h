#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     grid_helper.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This contains lat/long conversion utilities:
            grid names, distances, lat/long conversion, etc

            The goal is to collect these helpers together in one place,
            because these techniques are the most likely to be reused or
            rewritten into other programming languages.
*/

#include "constants.h"   // Griduino constants and colors
#include "logger.h"      // conditional printing to Serial port

// ========== class Grids =========================================
class Grids {
public:
  void calcLocator(char *result, double lat, double lon, int precision) {
    // Converts from lat/long to Maidenhead Grid Locator
    // From: https://ham.stackexchange.com/questions/221/how-can-one-convert-from-lat-long-to-grid-square
    // And:  10-digit calculator, fmaidenhead.php[158], function getGridName($lat,$long)
    /**************************** 6-digit **************************/
    // Input: char result[7];
    int o1, o2, o3;
    int a1, a2, a3;
    double remainder;

    // longitude
    remainder = lon + 180.0;
    o1        = (int)(remainder / 20.0);
    remainder = remainder - (double)o1 * 20.0;
    o2        = (int)(remainder / 2.0);
    remainder = remainder - 2.0 * (double)o2;
    o3        = (int)(12.0 * remainder);

    // latitude
    remainder = lat + 90.0;
    a1        = (int)(remainder / 10.0);
    remainder = remainder - (double)a1 * 10.0;
    a2        = (int)(remainder);
    remainder = remainder - (double)a2;
    a3        = (int)(24.0 * remainder);
    /**************************** 8-digit *************************
    // Input: char result[9];
    int o1, o2, o3, o4;
    int a1, a2, a3, a4;
    double remainder;

    // longitude
    remainder = lon + 180.0;
    o1 = (int)(remainder / 20.0);     // longitude first  letter is 360-degree globe in 18 parts (20 degree each) -> A..R
    remainder = remainder - (double)o1 / 2.0;
    o2 = (int)(10.0 * remainder);     // longitude second number is 20-degree swath in 10 parts (2 degree each)   -> 0..9
    remainder = remainder - (double)o2;
    o3 = (int)(24.0 * remainder);     // longitude third  letter is 2-degree swath in 24 parts (1/12 degree each) -> a..x
    remainder = remainder - (double)o3;
    o4 = (int)(10.0 * remainder);     // longitude fourth number is 1/12 degrees in 10 parts (1/120 degree each)  -> 0..9

    // latitude
    remainder = lat + 90.0;
    a1 = (int)(remainder / 10.0);     // latitude first letter is 180-degrees pole to pole in (10-degree each)    -> A..R
    remainder = remainder - (double)a1;
    a2 = (int)(10.0 * remainder);     // latitude second number is 10-degree swath in 10 parts (1 degree each)    -> 0..9
    remainder = remainder - (double)a2;
    a3 = (int)(24.0 * remainder);     // latitude third letter is 1 degree swath in 24 parts                      -> a..x
    remainder = remainder - (double)a3;
    a4 = (int)(10.0 * remainder);     // latitude fourth number is 1/24 degree swath in 10 parts                  -> 0..9

    char msg[64];
    snprintf(msg, sizeof(msg), "Longitude remainders: %d, %d, %d, %d", o1, o2, o3, o4); // debug
    Serial.println(msg);
    snprintf(msg, sizeof(msg), "Latitude remainders:  %d, %d, %d, %d", a1, a2, a3, a4); // debug
    Serial.println(msg);
    ***********************************/

    result[0] = (char)o1 + 'A';
    result[1] = (char)a1 + 'A';
    result[2] = (char)o2 + '0';
    result[3] = (char)a2 + '0';
    result[4] = (char)0;
    if (precision > 4) {
      result[4] = (char)o3 + 'a';
      result[5] = (char)a3 + 'a';
      result[6] = (char)0;
    }
    /*****
    if (precision > 6) {
      result[6] = (char)o4 + '0';
      result[7] = (char)a4 + '0';
      result[8] = (char)0;
    }
    *****/
    return;
  }

  //=========== distance helpers =============================
  double calcDistance(double fromLat, double fromLong, double toLat, double toLong, bool isMetric) {
    // Note: accurate for short distances, since it ignores curvature of earth
    double latDist  = calcDistanceLat(fromLat, toLat, isMetric);
    double longDist = calcDistanceLong(fromLat, fromLong, toLong, isMetric);
    double total    = sqrt(latDist * latDist + longDist * longDist);
    return total;
  }

  double calcDistanceLat(double fromLat, double toLat, bool isMetric) {
    // calculate distance in N-S direction (miles or km)
    // input:   latitudes in degrees
    //          metric true=km, false=miles
    // returns: 'double' in either English or Metric

    double R = 3958.8;   // average Earth radius (miles)
    if (isMetric) {
      R = 6371.0;   // average Earth radius (kilometers)
    }
    double angleDegrees = fabs(fromLat - toLat);
    double angleRadians = angleDegrees / degreesPerRadian;
    double distance     = angleRadians * R;
    return distance;
  }

  double calcDistanceLong(double lat, double fromLong, double toLong, bool isMetric) {
    // calculate distance in E-W direction (miles or km)
    // input:   latitudes in degrees
    //          metric true=km, false=miles
    // returns: 'double' in either English or Metric

    double R = 3958.8;   // average Earth radius (miles)
    if (isMetric) {
      R = 6371.0;   // average Earth radius (kilometers)
    }
    double scaleFactor  = fabs(cos(lat / degreesPerRadian));   // grids are narrower as you move from equator to north/south pole
    double angleDegrees = fabs(fromLong - toLong);
    double angleRadians = angleDegrees / degreesPerRadian * scaleFactor;
    double distance     = angleRadians * R;
    return distance;
  }

  // ============== grid helpers =================================
  // ----- north
  float nextGridLineNorth(float latitudeDegrees) {
    return ceil(latitudeDegrees);
  }
  float nextGrid6North(float latitudeDegrees) {
    // six-digit grid every 2.5 minutes latitude (2.5/60 = 0.041666 degrees)
    return ceil(latitudeDegrees * 60.0 / 2.5) / (60.0 / 2.5);
  }

  // ----- south
  float nextGridLineSouth(float latitudeDegrees) {
    return floor(latitudeDegrees);
  }
  float nextGrid6South(float latitudeDegrees) {
    // six-digit grid every 2.5 minutes latitude (2.5/60 = 0.041666 degrees)
    return floor(latitudeDegrees * 60.0 / 2.5) / (60.0 / 2.5);
  }

  // ----- east
  // given a position, find the longitude of the next grid line
  // this is always an even integer number since grids are 2-degrees wide
  float nextGridLineEast(float longitudeDegrees) {
    return ceil(longitudeDegrees / 2) * 2;
  }
  float nextGrid6East(float longitudeDegrees) {
    // six-digit grid every 5 minutes longitude (5/60 = 0.08333 degrees)
    return floor(longitudeDegrees * 60.0 / 5.0) / (60.0 / 5.0);
  }

  // ----- west
  float nextGridLineWest(float longitudeDegrees) {
    return floor(longitudeDegrees / 2) * 2;
  }
  float nextGrid6West(float longitudeDegrees) {
    // six-digit grid every 5 minutes longitude (5/60 = 0.08333 degrees)
    return ceil(longitudeDegrees * 60.0 / 5.0) / (60.0 / 5.0);
  }

};   // end class Grids
