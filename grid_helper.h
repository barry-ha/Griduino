#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     grid_helper.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This contains lat/long conversion utilities:
            grid names, distances, lat/long conversion, etc

            The goal is to collect these helpers together in one place,
            because these techniques are likely to be reused or rewritten
            into other programming languages.
*/

#include "constants.h"   // Griduino constants and colors
#include "logger.h"      // conditional printing to Serial port
#include <cstdlib>

// ----- globals
extern Logger logger;   // Griduino.ino

// ========== class Grids =========================================
class Grids {
public:
  void calcLocator(char *result, double lat, double lon, int precision) {
    // Converts from lat/long to Maidenhead Grid Locator
    // From: https://ham.stackexchange.com/questions/221/how-can-one-convert-from-lat-long-to-grid-square
    // And:  10-digit calculator, fmaidenhead.php[158], function getGridName($lat,$long)
    /**************************** 6-digit **************************/
    // Input: char result[7];
    int o1, o2, o3, o4;   // l_o_ngitude
    int a1, a2, a3, a4;   // l_a_titude
    // double remainder;   // (unused)

    // longitude
    double r = (lon + 180.0) / 20.0 + 1e-7;
    o1       = (int)r;
    r        = 10.0 * (r - floor(r));
    o2       = (int)r;
    r        = 24.0 * (r - floor(r));
    o3       = (int)r;
    r        = 10.0 * (r - floor(r));
    o4       = (int)r;

    // latitude
    double t = (lat + 90.0) / 10.0 + 1e-7;
    a1       = (int)t;
    t        = 10.0 * (t - floor(t));
    a2       = (int)t;
    t        = 24.0 * (t - floor(t));
    a3       = (int)t;
    t        = 10.0 * (t - floor(t));
    a4       = (int)t;

    /* ... debug ...
    char msg[64];                                                                             // debug
    if (o1 < 0 || o2 < 0 || o3 < 0 || o4 < 0) {                                               // debug
      snprintf(msg, sizeof(msg), ". Longitude remainders: %d, %d, %d, %d", o1, o2, o3, o4);   // debug
      logger.log(DEBUG, msg);                                                                 // debug
    }                                                                                         // debug
    if (a1 < 0 || a2 < 0 || a3 < 0 || a4 < 0) {                                               // debug
      snprintf(msg, sizeof(msg), ". Latitude remainders:  %d, %d, %d, %d", a1, a2, a3, a4);   // debug
      logger.log(DEBUG, msg);                                                                 // debug
    }
    ... */

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
    if (precision > 6) {
      result[6] = (char)o4 + '0';
      result[7] = (char)a4 + '0';
      result[8] = (char)0;
    }
    return;
  }

  //=========== distance helpers =============================
  bool isVisibleDistance(const PointGPS from, const PointGPS to) {
    // has the vehicle moved some minimum amount, enough to be visible?

    if (abs(from.lat - to.lat) >= minLat) {
      return true;
    } else if (abs(from.lng - to.lng) >= minLong) {
      return true;
    } else {
      return false;
    }
  }

  double calcDistance(double fromLat, double fromLong, double toLat, double toLong, bool isMetric) {
    // Note: accurate for short distances, since it ignores curvature of earth
    double latDist  = calcDistanceLat(fromLat, toLat, isMetric);
    double longDist = calcDistanceLong(fromLat, fromLong, toLong, isMetric);
    double total    = sqrt(latDist * latDist + longDist * longDist);
    return total;   // miles or km, depending on 'isMetric'
  }

// Calculate compass heading from simulated-GPS starting and ending coordinates
// Not needed when reading GPS hardware because NMEA includes compass heading directly
// Calculations are done on the unit circle, then converted to compass headings.
//
//        UNIT CIRCLE
//   aka SCREEN DEGREES                COMPASS                   GPS
//            +90°                        0                  coordinates
//             ▲                          ▲                +-------------+ 48°
//         Q2  |  Q1                      |                |             |
//  180°◄------+------► 0°     270°◄------+------► 90°     |             |
//         Q3  |  Q4                      |                |             |
//             ▼                          ▼                +-------------+ 47° latitude
//            -90°                       180°            -122°         -120°   longitude
//      +ve angles = CCW           +ve angles = CW
//
  float calcHeading(double fromLat, double fromLong, double toLat, double toLong) {
    // direction of travel, returns degrees from true north
    double latAngleRadians = (toLat - fromLat) * radiansPerDegree;   // + north, - south
    double latDistance     = latAngleRadians * earthRadiusKM;        // km

    // double longDist = calcDistanceLong(fromLat, fromLong, toLong, true);
    double scaleFactor      = cos(fromLat * radiansPerDegree);   // grids are narrower as you move from equator to north/south pole
    double longAngleRadians = (toLong - fromLong) * radiansPerDegree * scaleFactor;
    double longDistance     = longAngleRadians * earthRadiusKM;   // + east, - west

    // avoid dividing by near-zero
    float ratio = (abs(latDistance) < 0.001) ? (latDistance * 1000.0) : (latDistance / longDistance);

    // "arctan" returns values in the range of -π/2 (south) to 0 (east) to +π/2 (north)
    float screenDegrees = atan(ratio) * degreesPerRadian;

    // westward movement requires heading adjustment into left-hand side of unit circle
    if (longDistance < 0.0) {
      screenDegrees -= 180.0;
    }

    // convert unit circle headings to compass headings, and use positive 0..360
    int resultHeading = 90 - round(screenDegrees);
    if (resultHeading >= 360) {
      resultHeading -= 360;
    }
    if (resultHeading < 0) {
      resultHeading += 360;
    }

    // clang-format off
    char msg[256];
    char sToLat[12], sToLong[12], sLatDistance[12], sLongDistance[12], sScaleFactor[12], sRatio[12], sScreenDegrees[12];
    floatToCharArray(sToLat, sizeof(sToLat), toLat, 4);
    floatToCharArray(sToLong, sizeof(sToLong), toLong, 4);
    floatToCharArray(sLatDistance, sizeof(sLatDistance), latDistance, 2);
    floatToCharArray(sLongDistance, sizeof(sLongDistance), longDistance, 2);
    floatToCharArray(sRatio, sizeof(sRatio), ratio, 2);
    floatToCharArray(sScaleFactor, sizeof(sScaleFactor), scaleFactor, 3);
    floatToCharArray(sScreenDegrees, sizeof(sScreenDegrees), screenDegrees, 1);
    snprintf(msg, sizeof(msg), "toLat(%s), toLong(%s), latDistance(%s), longDistance(%s), scaleFactor(%s), ratio(%s), screenDegrees(%s), resultHeading(%d)",
                                sToLat,    sToLong,    sLatDistance,    sLongDistance,    sScaleFactor,    sRatio,    sScreenDegrees,    resultHeading);
    logger.log(GPS_SETUP, DEBUG, msg);
    // clang-format on

    return resultHeading;
  }

  double calcDistanceLat(double fromLat, double toLat, bool isMetric) {
    // calculate distance in N-S direction (miles or km)
    // input:   latitudes in degrees
    //          metric true=km, false=miles
    // returns: positive distance, in either English or Metric

    double R            = isMetric ? earthRadiusKM : earthRadiusMiles;
    double angleDegrees = fabs(fromLat - toLat);
    double angleRadians = angleDegrees / degreesPerRadian;
    double distance     = angleRadians * R;
    return distance;
  }

  double calcDistanceLong(double lat, double fromLong, double toLong, bool isMetric) {
    // calculate distance in E-W direction (miles or km)
    // input:   latitudes in degrees
    //          metric true=km, false=miles
    // returns: positive distance, in either English or Metric

    double R            = isMetric ? earthRadiusKM : earthRadiusMiles;
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
