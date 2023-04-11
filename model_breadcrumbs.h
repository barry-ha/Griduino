#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     model_breadcrumbs.h

            Contains everything about the breadcrumb history trail.
            This is a circular buffer in memory, which periodically written to a CSV file.
            Buffer is never full; we can always add more records and overwrite the oldest.

  Breadcrumb trail strategy:
            This is a "model" of the Model-View-Controller design pattern.
            As such, it contains the data for holding all the breadcrumbs  and is a
            low-level component with minimal side effects and few dependencies.

            This "Breadcrumbs" object will just only manage itself.
            It does not schedule events or request actions from other objects.
            The caller is responsible for asking us to write to a file, output to console, 
            produce a formatted KML file, and etc when it's needed.
            We don't reach into the "model" from here and tell it what to do.
            When the caller tells us to save to file, don't tell the "model" to do anything.

  Todo:     1. refactor "class Location" into this file
            2. refactor "makeLocation()" into ctor (or public member) of "class Location"
            3. replace remember(Location) with remember(a,b,c,d,e,f)
            4. add rememberPDN(), rememberTOD()

  API Design:
      Initialization:
            We need to initialize the circular buffer container with a buffer and size
            We need to reset the circular buffer container
      State tracking:
            We need to know whether the buffer is full or empty
            We need to know the current number of elements in the buffer
            We need to know the max capacity of the buffer
      Add/read records:
            We need to be able to add data to the buffer
            We need to be able to read values by iterating through the buffer
            We don't need to remove elements from the buffer
            We don't need thread safety
      I/O:
            We need to send various neatly-formatted reports to the console
            We need to save and restore the buffer to/from a file
            We want to hide implementation details from other modules

  Full and Empty:
            The "full" and "empty" cases look the same: head and tail pointer are equal.
            There are two approaches to differentiating between full and empty:
            1. Waste a slot in the buffer:
                  Full state is head + 1 == tail
                  Empty state is head == tail
            2. Use a bool flag and additional logic:    <-- Griduino chooses this approach
                  Full state has "bool full"
                  Empty state is (head == tail) && !full
            Griduino wants efficient memory usage and doesn't need thread safety.

  Size of GPS breadcrumb trail:
            Our goal is to keep track of at least one long day's travel, 500 miles or more.
            If 180 pixels horiz = 100 miles, then we need (500*180/100) = 900 entries.
            If 160 pixels vert = 70 miles, then we need (500*160/70) = 1,140 entries.
            For example, if I drive a drunken-sailor route around the Olympic Peninsula,
            we need at least 800 entries to capture the whole out-and-back 500-mile loop.

            array  bytes        total          2023-04-08
            size   per entry    memory         comment
            2500     48 bytes  120,000 bytes   ok
            2950     48 bytes  141,600 bytes   ok
            2980     48 bytes  143,040 bytes   ok
            3000     48 bytes  144,000 bytes   crash on "list files" command

  Inspiration:
            https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

*/

#include "logger.h"         // conditional printing to Serial port
#include "grid_helper.h"    // lat/long conversion routines
#include "date_helper.h"    // date/time conversions
#include "save_restore.h"   // Configuration data in nonvolatile RAM

// ========== extern ===========================================
extern Logger logger;   // Griduino.ino
extern Grids grid;      // grid_helper.h
extern Dates date;      // date_helper.h

bool isVisibleDistance(const PointGPS from, const PointGPS to);   // view_grid.cpp

// ========== class History ======================
class Breadcrumbs {
public:
  // Class member variables
  const int totalSize  = sizeof(history);          // bytes
  const int recordSize = sizeof(Location);         // bytes
  const int capacity   = totalSize / recordSize;   // max number of records
  int saveInterval     = 2;

private:
  Location history[2500];   // remember a list of GPS coordinates and stuff
  bool full   = false;      //
  int head    = 0;          // index of next item to write = nextHistoryItem
  int tail    = 0;          // index of oldest item
  int current = 0;          // index used for iterators begin(), next()

  const PointGPS noLocation{-1.0, -1.0};   // eye-catching value, and nonzero for "isEmpty()"
  const float noSpeed        = -1.0;
  const float noDirection    = -1.0;
  const float noAltitude     = -1.0;
  const uint8_t noSatellites = 0;

public:
  // ----- Initialization -----
  Breadcrumbs() {}   // Constructor - create and initialize member variables

  void clearHistory() {   // wipe clean the trail of breadcrumbs
    head = tail = 0;
    full        = false;
    for (uint ii = 0; ii < capacity; ii++) {
      history[ii].reset();
    }
    logger.info("Breadcrumb trail history[%d] has been erased", capacity);
  }

  // ----- State tracking
  const int getHistoryCount() {   // how many history slots currently contain breadcrumb data
    int count;
    if (full) {
      count = capacity;
    } else {
      if (head >= tail) {
        count = head - tail;
      } else {
        count = capacity + head - tail;
      }
    }
    return count;
  }

  const int isEmpty() {   // indicate empty
    return (head == tail);
  }

  const bool isFull() {   // buffer is never full; we can always add more records
    return false;
  }

  // ----- Add or read records
  void rememberPUP() {   // save "power-up" event in the history buffer
    Location pup{rPOWERUP, noLocation, now(), noSatellites, noSpeed, noDirection, noAltitude};
    remember(pup);
  }

  void rememberFirstValidTime(time_t vTime, uint8_t vSats) {   // save "first valid time received from GPS"
    // "first valid time" can happen _without_ a satellite fix,
    // so the only data stored is the GMT timestamp
    Location fvt{rVALIDTIME, noLocation, vTime, vSats, noSpeed, noDirection, noAltitude};
    remember(fvt);
  }

  void remember(Location vLoc) {   // save GPS location and timestamp in history buffer
    // so that we can display it as a breadcrumb trail
    history[head] = vLoc;
    advance_pointer();
  }

  void advance_pointer() {
    if (full) {
      tail = (tail + 1) % capacity;   // if buffer full, advance tail pointer
    }
    head = (head + 1) % capacity;   // always advance head pointer
    full = (head == tail);          // check if adding a record triggers the full condition
  }

  Location *begin() {   // returns pointer to tail of buffer, or null if buffer is empty
    current = tail;
    return &history[current];
  }

  Location *next() {   // returns pointer to next element, or null if buffer is empty
    Location *ptr = nullptr;
    if (current == head) {
      // nothing returned - end of data
    } else {
      current = (current + 1) % capacity;
      ptr     = &history[current];
    }
    return ptr;
  }

  // ----- I/O
  // our breadcrumb trail file is CSV format -- you can open this Arduino file directly in a spreadsheet
  int saveGPSBreadcrumbTrail();   // returns 1=success, 0=failure

  int restoreGPSBreadcrumbTrail();   // returns 1=success, 0=failure

  void deleteFile();   // model_breadcrumbs.cpp

  void dumpHistoryGPS(int limit = 0);

  void dumpHistoryKML();

  // ----- Internal helpers
private:
  bool isValidBreadcrumb(const char *crumb) {
    // examine an input line from saved breadcrumb trail to see if it's a plausible GPS record type
    // clang-format off
    if ((strlen(crumb) > 4)
      &&(',' == crumb[3])
      &&('A' <= crumb[0] && crumb[0] <= 'Z')
      &&('A' <= crumb[1] && crumb[1] <= 'Z')
      &&('A' <= crumb[2] && crumb[2] <= 'Z')) {
      return true;
    } else {
      return false;
    }
    // clang-format on
  }

};   // end class Breadcrumbs
