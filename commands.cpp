// Please format this file with clang before check-in to GitHub
/*
  File:     morse_dac.cpp

  Date:     2022-08-20 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Listen for incoming USB commands and handle them.

*/

#include <Arduino.h>             // for Serial
#include "constants.h"           // Griduino constants and colors
#include "logger.h"              // conditional printing to Serial port
#include "model_breadcrumbs.h"   // breadcrumb trail
#include "model_gps.h"           // Model of a GPS for model-view-controller
#include "model_baro.h"          // Model of a barometer that stores 3-day history
#include "view.h"                // View base class, public interface

// ========== extern ===========================================
extern Logger logger;                 // Griduino.ino
extern bool showTouchTargets;         // Griduino.ino
extern Model *model;                  // "model" portion of model-view-controller
extern Breadcrumbs trail;             // model of breadcrumb trail
extern BarometerModel baroModel;      // singleton instance of the barometer model
extern void selectNewView(int cmd);   // Griduino.ino
extern View *pView;                   // Griduino.ino

// ----- forward references
void help(), version();
void dump_kml(), dump_gps_history(), erase_gps_history(), list_files();
void start_nmea(), stop_nmea(), start_gmt(), stop_gmt();
void view_help(), view_screen1(), view_splash(), view_crossings(), view_events();
void show_touch(), hide_touch();
void run_unittest();

// ----- table of commands
#define Newline true   // use this to insert a CRLF before listing this command in help text
struct Command {
  char text[20];
  simpleFunction function;
  bool crlf;
};
Command cmdList[] = {
    {"help", help, 0},
    {"version", version, 0},

    {"dump kml", dump_kml, Newline},
    {"dump gps", dump_gps_history, 0},
    {"erase history", erase_gps_history, 0},

    {"start nmea", start_nmea, Newline},
    {"stop nmea", stop_nmea, 0},

    {"show touch", show_touch, Newline},
    {"hide touch", hide_touch, 0},

    {"start gmt", start_gmt, Newline},
    {"stop gmt", stop_gmt, 0},

    {"view help", view_help, Newline},
    {"view splash", view_splash, 0},
    {"view screen1", view_screen1, 0},
    {"view crossings", view_crossings, 0},
    {"view events", view_events, 0},

    {"list files", list_files, Newline},
    {"run unittest", run_unittest, 0},
};
const int numCmds = sizeof(cmdList) / sizeof(cmdList[0]);

// ----- functions to implement commands
void help() {
  Serial.print("Available commands are:\n");
  for (int ii = 0; ii < numCmds; ii++) {
    if (cmdList[ii].crlf) {
      Serial.println();
    }

    Serial.print(cmdList[ii].text);

    if (ii < numCmds - 1) {
      Serial.print(", ");
    }
  }
  Serial.println();
}

void version() {
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);
  Serial.println("Compiled " PROGRAM_COMPILED);
  Serial.println(PROGRAM_LINE1 "  " PROGRAM_LINE2);
  Serial.println(PROGRAM_FILE);
  Serial.println(PROGRAM_GITHUB);
}

void dump_kml() {
  model->dumpHistoryKML();
}

void dump_gps_history() {
  model->dumpHistoryGPS();
}

void erase_gps_history() {
  trail.clearHistory();
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

void view_help() {
  Serial.println("view Help screen");
  extern /*const*/ int help_view;   // see "Griduino.ino"
  selectNewView(help_view);         // see "Griduino.ino"
}

void view_splash() {
  Serial.println("view Splash screen");
  extern /*const*/ int splash_view;   // see "Griduino.ino"
  selectNewView(splash_view);         // see "Griduino.ino"
}

void view_screen1() {
  Serial.println("view Screen 1");
  extern /*const*/ int screen1_view;   // see "Griduino.ino"
  selectNewView(screen1_view);         // see "Griduino.ino"
}

void view_crossings() {
  logger.info("view grid crossings");
  extern /*const*/ int grid_crossings_view;   // see Griduino.com
  selectNewView(grid_crossings_view);
}

void view_events() {
  logger.info("view calendar events");
  extern /*const*/ int events_view;   // see Griduino.com
  selectNewView(events_view);
}

void show_touch() {
  Serial.println("showing touch targets");
  showTouchTargets = true;
  pView->startScreen();
  pView->updateScreen();
}

void hide_touch() {
  Serial.println("hiding touch targets");
  showTouchTargets = false;
  pView->startScreen();
  pView->updateScreen();
}

void run_unittest() {
  Serial.println("running unit test suite");
  void runUnitTest();   // extern declaration
  runUnitTest();        // see "unit_test.cpp"
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
