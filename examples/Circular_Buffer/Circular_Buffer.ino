// Please format this file with clang before check-in to GitHub
/*
  Circular Buffer using ETL (embedded template library)

  ETL by John Wellbelove, https://www.etlcpp.com/circular_buffer.html
 */

// ensure no STL
#define ETL_NO_STL

#include <TimeLib.h>                     // time_t=seconds since Jan 1, 1970, https://github.com/PaulStoffregen/Time
#include "Embedded_Template_Library.h"   // Required for any etl import using Arduino IDE
#include "etl/circular_buffer.h"         // Embedded Template Library

// ========== struct for each item in the buffer
struct satCountItem {
  time_t tm;  // seconds since Jan 1, 1970
  int numSats;
};

etl::circular_buffer<int, 12> cb;              // dummy, unused
etl::circular_buffer<int, 12>::iterator itr;   // dummy, unused

etl::circular_buffer<satCountItem, 12> cbSats;
etl::circular_buffer<satCountItem, 12>::iterator cbIter;

// ========== helpers
// pushes a value to the back of the circular buffer
void mypush(time_t tm, int nSats) {
  satCountItem item = {tm, nSats};
  cbSats.push(item);
}

// output one element of data
// slot = which bar of the bar graph, 0..12
// value = number of satellites in view
  void showBar(int slot, satCountItem item) {
    char msg[128];
    int hh = hour(item.tm);
    int mm = minute(item.tm);
    int ss = second(item.tm);
    snprintf(msg, sizeof(msg), "[%d] %02d:%02d:%02d time, %d satellites", 
                               slot, hh, mm, ss, item.numSats);
    Serial.println(msg);
  }

// the setup routine runs once when you press reset:
void setup() {
  // ----- init serial monitor
  Serial.begin(115200);   // init for debugging in the Arduino IDE
  while (!Serial) {       // Wait for console in the Arduino IDE
    yield();
  }
  Serial.println("========== Test 1: Print empty buffer");
  // print all elements
  int ii = 0;
  for (cbIter=cbSats.begin(); cbIter<cbSats.end(); ++cbIter) {
    showBar(ii, *cbIter);
    ii++;
  }

  Serial.println("========== Test 2: Print buffer with one element");
  // populate the circular buffer
  //      hh mm ss day mo yr
  setTime( 2, 0, 0, 1, 1, 2023);
  mypush(now(), 1);

  // print all elements
  ii = 0;
  for (cbIter=cbSats.begin(); cbIter<cbSats.end(); ++cbIter) {
    showBar(ii, *cbIter);
    ii++;
  }

  Serial.println("========== Test 3: Print buffer with three elements");
  setTime( 3, 0, 0, 1, 1, 2023);
  mypush(now(), 2);
  mypush(now(), 3);

  // print all elements
  ii = 0;
  for (cbIter=cbSats.begin(); cbIter<cbSats.end(); ++cbIter) {
    showBar(ii, *cbIter);
    ii++;
  }

  Serial.println("========== Test 4: Print a full buffer");
  setTime( 4, 0, 0, 1, 1, 2023);
  for (int ii=0; ii<130; ii++) {
    mypush(now(), 4+ii);
  }
  // print all elements
  int bar = 0;
  for (cbIter=cbSats.begin(); cbIter<cbSats.end(); ++cbIter) {
    showBar(bar, *cbIter);
    bar++;
  }

}

// the loop routine runs over and over again forever:
void loop() {
}
