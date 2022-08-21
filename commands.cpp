// Please format this file with clang before check-in to GitHub
/*
  File:     morse_dac.cpp

  Date:     2022-08-20 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Listen for incoming USB commands and handle them.

*/

#include "constants.h"    // Griduino constants and colors
#include "logger.h"       // conditional printing to Serial port
#include "model_gps.h"    // Model of a GPS for model-view-controller
#include "model_baro.h"   // Model of a barometer that stores 3-day history

// ========== extern ===========================================
extern Logger logger;              // Griduino.ino
extern Model *model;               // "model" portion of model-view-controller
extern BarometerModel baroModel;   // singleton instance of the barometer model

// ----- forward references
void help(), version();
void dump_kml(), dump_gps_history(), list_files();
void start_nmea(), stop_nmea(), start_gmt(), stop_gmt();

// ----- table of commands
struct Command {
  char text[20];
  simpleFunction function;
};
Command cmdList[] = {
    {"help", help},
    {"version", version},
    {"dump kml", dump_kml},
    {"dump gps", dump_gps_history},
    {"list", list_files},
    {"start nmea", start_nmea},
    {"stop nmea", stop_nmea},
    {"start gmt", start_gmt},
    {"stop gmt", stop_gmt},
};
const int numCmds = sizeof(cmdList) / sizeof(cmdList[0]);

// ----- functions to implement commands
void help() {
  Serial.print("Available commands are:\n");
  for (int ii = 0; ii < numCmds; ii++) {
    if (ii > 0) {
      Serial.print(", ");
    }
    Serial.print(cmdList[ii].text);
  }
  Serial.println();
}
void version() {
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);
  Serial.println("Compiled " PROGRAM_COMPILED);
  Serial.println(PROGRAM_LINE1 "  " PROGRAM_LINE2);
  Serial.println(__FILE__);
  Serial.println(PROGRAM_GITHUB);
}

void dump_kml() {
  model->dumpHistoryKML();
}

void dump_gps_history() {
  model->dumpHistoryGPS();
}

void list_files() {
  SaveRestore saver("x", "y");   // dummy config object, we won't actually save anything
  saver.listFiles("/");          // list them starting at root
}

void start_nmea() {
  Serial.println("started");
  logger.print_nmea = true;
}

void stop_nmea() {
  Serial.println("stopped");
  logger.print_nmea = false;
}
void start_gmt() {
  Serial.println("started");
  logger.print_gmt = true;
}

void stop_gmt() {
  Serial.println("stopped");
  logger.print_gmt = false;
}

// do the thing
void processCommand(char *cmd) {

  for (char *p = cmd; *p != '\0'; ++p) {   // convert to lower case
    *p = tolower(*p);
  }
  Serial.print(cmd);
  Serial.print(": ");

  bool found = false;
  for (int ii = 0; ii < numCmds; ii++) {        // loop through table of commands
    if (strcmp(cmd, cmdList[ii].text) == 0) {   // look for it
      cmdList[ii].function();                   // found it! call the subroutine
      found = true;
      break;
    }
  }
  if (!found) {
    Serial.println("Unsupported");
  }
}
