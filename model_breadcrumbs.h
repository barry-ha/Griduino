#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     model_breadcrumbs.h

            Contains everything about the breadcrumb history trail.

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

*/

// #include <Arduino.h>        //
// #include <Adafruit_GPS.h>   // "Ultimate GPS" library
// #include "constants.h"      // Griduino constants, colors, typedefs
// #include "hardware.h"       //
// #include "logger.h"         // conditional printing to Serial port
// #include "grid_helper.h"    // lat/long conversion routines
// #include "date_helper.h"    // date/time conversions
// #include "save_restore.h"   // Configuration data in nonvolatile RAM

// ========== extern ===========================================
// extern Adafruit_GPS GPS;       // Griduino.ino
// extern Logger logger;          // Griduino.ino
// extern Grids grid;             // grid_helper.h
// extern Dates date;             // date_helper.h

// void floatToCharArray(char *result, int maxlen, double fValue, int decimalPlaces);   // Griduino.ino
// bool isVisibleDistance(const PointGPS from, const PointGPS to);                      // view_grid.cpp

// Size of GPS breadcrumb trail:
// Our goal is to keep track of at least one long day's travel, 500 miles or more.
// If 180 pixels horiz = 100 miles, then we need (500*180/100) = 900 entries.
// If 160 pixels vert = 70 miles, then we need (500*160/70) = 1,140 entries.
// In reality, with a drunken-sailor route around the Olympic Peninsula,
// we need at least 800 entries to capture the whole out-and-back 500-mile loop.
Location history[3000];               // remember a list of GPS coordinates and stuff
const int numHistory = sizeof(history) / sizeof(Location);

// ========== class History ======================
class Breadcrumbs {
public:
  // Class member variables

protected:
public:
  // Constructor - create and initialize member variables
  Breadcrumbs() {}

};   // end class History
