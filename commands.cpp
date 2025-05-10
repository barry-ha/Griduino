// Please format this file with clang before check-in to GitHub
/*
  File:     commands.cpp

  Date:     2022-08-20 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Listen for incoming USB commands and handle them.

*/

#include <Arduino.h>             // for Serial
#include "constants.h"           // Griduino constants and colors
#include <elapsedMillis.h>       // Scheduling intervals in main loop
#include "logger.h"              // conditional printing to Serial port
#include "model_breadcrumbs.h"   // breadcrumb trail
#include "model_crypto.h"        // sha204 protected storage
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
void dump_kml(), dump_gps_history(), erase_gps_history(), list_files(), type_gpshistory();
void start_nmea(), stop_nmea(), start_gmt(), stop_gmt();
void show_help(), show_screen1(), show_splash(), show_crossings(), show_events(), show_reformat();
void show_touch(), hide_touch();
void show_centerline(), hide_centerline();
void run_unittest();
void enable_console_log(), disable_console_log();
void log_level_debug(), log_level_fence(), log_level_info(), log_level_warning(), log_level_error();
void show_logging_status();

// ----- table of commands
#define Newline true   // use this to insert a CRLF before listing this command in help text
struct Command {
  bool crlf;
  char text[20];
  simpleFunction function;
};
Command cmdList[] = {
    {0, "help", help},
    {0, "version", version},

    {Newline, "dump kml", dump_kml},
    {0, "dump gps", dump_gps_history},
    {0, "type gpshistory", type_gpshistory},
    {0, "erase history", erase_gps_history},

    {Newline, "start nmea", start_nmea},
    {0, "stop nmea", stop_nmea},

    {Newline, "start gmt", start_gmt},
    {0, "stop gmt", stop_gmt},

    {Newline, "show touch", show_touch},
    {0, "hide touch", hide_touch},

    {Newline, "show centerline", show_centerline},
    {0, "hide centerline", hide_centerline},

    {Newline, "show help", show_help},
    {0, "show splash", show_splash},
    {0, "show screen1", show_screen1},
    {0, "show crossings", show_crossings},
    {0, "show events", show_events},
    {0, "show reformat", show_reformat},

    {Newline, "dir", list_files},
    {0, "list files", list_files},

    {Newline, "run unittest", run_unittest},

    {Newline, "enable console log", enable_console_log},
    {0, "disable console log", disable_console_log},

    {Newline, "log level debug", log_level_debug},
    {0, "log level fence", log_level_fence},
    {0, "log level info", log_level_info},
    {0, "log level warning", log_level_warning},
    {0, "log level error", log_level_error},

    {Newline, "show logging", show_logging_status},
};
const int numCmds = sizeof(cmdList) / sizeof(cmdList[0]);

// ----- functions to implement commands
void help() {
  logger.println("Griduino " PROGRAM_VERSION ", compiled " PROGRAM_COMPILED);
  logger.println("\nAvailable commands are:");
  for (int ii = 0; ii < numCmds; ii++) {
    if (cmdList[ii].crlf) {
      logger.println();
    }

    logger.print(cmdList[ii].text);

    if (ii < numCmds - 1) {
      logger.print(", ");
    }
  }
  logger.println();
}

void version() {
  logger.log(COMMAND, CONSOLE, "version");
  logger.log(COMMAND, CONSOLE, PROGRAM_TITLE " " PROGRAM_VERSION);
  logger.log(COMMAND, CONSOLE, "Hardware PCB " HARDWARE_VERSION);
  logger.log(COMMAND, CONSOLE, "Compiled " PROGRAM_COMPILED);
  logger.log(COMMAND, CONSOLE, PROGRAM_LINE1 "  " PROGRAM_LINE2);
  logger.log(COMMAND, CONSOLE, PROGRAM_FILE);
  logger.log(COMMAND, CONSOLE, PROGRAM_GITHUB);
}

void dump_kml() {
  logger.log(COMMAND, CONSOLE, "dump KML");
  trail.dumpHistoryKML();
}

void dump_gps_history() {
  logger.log(COMMAND, CONSOLE, "dump GPS");
  trail.dumpHistoryGPS();
}

void erase_gps_history() {
  logger.log(COMMAND, CONSOLE, "erase history");
  trail.clearHistory();
  trail.rememberPUP();
  trail.deleteFile();               // out with the old history file
  trail.saveGPSBreadcrumbTrail();   // start over with new history file
}

void list_files() {
  logger.log(COMMAND, CONSOLE, "list files");
  SaveRestore saver("x", "y");   // dummy config object, we won't actually save anything
  saver.listFiles("/");          // list all files starting at root
}

void type_gpshistory() {
  logger.log(COMMAND, CONSOLE, "type gpshistory");
  SaveRestoreStrings saver("/Griduino/gpshistory.csv", "No Version");
  saver.typeFile();
}

void start_nmea() {
  logger.log(COMMAND, CONSOLE, "started NMEA");
  logger.printSystem[NMEA].enabled = true;
}

void stop_nmea() {
  logger.log(COMMAND, CONSOLE, "stopped NMEA");
  logger.printSystem[NMEA].enabled = false;
}

void start_gmt() {
  logger.log(COMMAND, CONSOLE, "started GMT");
  logger.printSystem[GMT].enabled = true;
}

void stop_gmt() {
  logger.log(COMMAND, CONSOLE, "stopped");
  logger.printSystem[GMT].enabled = false;
}

void show_help() {
  logger.log(COMMAND, CONSOLE, "show Help screen");
  extern /*const*/ int help_view;   // Griduino.ino
  extern uint viewHelpTimeout;      // Griduino.ino
  extern elapsedSeconds viewHelpTimer;

  viewHelpTimer   = 0;               // reset timer
  viewHelpTimeout = SECS_PER_1MIN;   // a loooong time for user to read the 3-word help screen
  selectNewView(help_view);          // Griduino.ino
}

void show_splash() {
  logger.log(COMMAND, CONSOLE, "show Splash screen");
  extern /*const*/ int splash_view;   // see "Griduino.ino"
  selectNewView(splash_view);         // see "Griduino.ino"
}

void show_screen1() {
  logger.log(COMMAND, CONSOLE, "show Screen 1");
  extern /*const*/ int screen1_view;   // see "Griduino.ino"
  selectNewView(screen1_view);         // see "Griduino.ino"
}

void show_crossings() {
  logger.log(COMMAND, CONSOLE, "show grid crossings screen");
  extern /*const*/ int grid_crossings_view;   // see Griduino.com
  selectNewView(grid_crossings_view);
}

void show_events() {
  logger.log(COMMAND, CONSOLE, "show calendar events screen");
  extern /*const*/ int events_view;   // see Griduino.com
  selectNewView(events_view);
}

void show_reformat() {
  logger.log(COMMAND, CONSOLE, "show reformat flash memory screen");
  extern /*const*/ int reformat_view;   // see Griduino.com
  selectNewView(reformat_view);
}

void show_touch() {
  logger.log(COMMAND, CONSOLE, "showing touch targets");
  showTouchTargets = true;
  pView->startScreen();
  pView->updateScreen();
}

void hide_touch() {
  logger.log(COMMAND, CONSOLE, "hiding touch targets");
  showTouchTargets = false;
  pView->startScreen();
  pView->updateScreen();
}

void show_centerline() {
  logger.log(COMMAND, CONSOLE, "showing centerline");
  showCenterline = true;
  pView->startScreen();
  pView->updateScreen();
}

void hide_centerline() {
  logger.log(COMMAND, CONSOLE, "hiding centerline");
  showCenterline = false;
  pView->startScreen();
  pView->updateScreen();
}

void run_unittest() {
  logger.log(COMMAND, CONSOLE, "running unit test suite");
  void runUnitTest();   // extern declaration
  runUnitTest();        // see "unit_test.cpp"
}

void enable_console_log() {
  logger.log(COMMAND, CONSOLE, "enabling logging to console");
  logger.log_enabled = true;
}

void disable_console_log() {
  logger.log(COMMAND, CONSOLE, "disabling all logging to console");
  logger.log_enabled = false;
}
// ----- log sybsystems
void show_logging_status() {
  logger.log(COMMAND, CONSOLE, "subsystems being logged are:");
  for (int ii = 0; ii < numSystems; ii++) {
    if (logger.printSystem[ii].enabled) {
      logger.log(COMMAND, CONSOLE, "%s = true", logger.printSystem[ii].name);
    } else {
      logger.log(COMMAND, CONSOLE, "%s = .", logger.printSystem[ii].name);
    }
  }

  logger.log(COMMAND, CONSOLE, "severity levels being logged are:");
  char str[20];
  for (int ii = 0; ii < numLevels; ii++) {
    snprintf(str, sizeof(str), "%c = %s", logger.printLevel[ii].abbr, logger.printLevel[ii].name);
    if (logger.printLevel[ii].enabled) {
      logger.log(COMMAND, CONSOLE, "%s = true", str);
    } else {
      logger.log(COMMAND, CONSOLE, "%s = .", str);
    }
  }
}
// ----- log severity levels
void log_level_debug() {
  logger.log(COMMAND, CONSOLE, "setting log level debug, fence, info, warning, error, console");
  logger.setLevel(DEBUG);
}
void log_level_fence() {
  logger.log(COMMAND, CONSOLE, "setting log level fence, info, warning, error, console");
  logger.setLevel(POST);
}
void log_level_info() {
  logger.log(COMMAND, CONSOLE, "setting log level info, warning, error, console");
  logger.setLevel(INFO);
}
void log_level_warning() {
  logger.log(COMMAND, CONSOLE, "setting log level warning, error, console");
  logger.setLevel(WARNING);
}
void log_level_error() {
  logger.log(COMMAND, CONSOLE, "setting log level error, console");
  logger.setLevel(ERROR);
}

void removeCRLF(char *pBuffer) {
  // remove 0x0d and 0x0a from character arrays, shortening the array in-place
  const char key[] = "\r\n";
  char *pch        = strpbrk(pBuffer, key);
  while (pch != NULL) {
    strcpy(pch, pch + 1);
    pch = strpbrk(pBuffer, key);
  }
}

// do the thing
void processCommand(char *cmd) {

  for (char *p = cmd; *p != '\0'; ++p) {   // convert to lower case
    *p = tolower(*p);
  }

  removeCRLF(cmd);   // Arduino IDE can optionally add \r\n

  logger.print(cmd);
  logger.print(": ");

  bool found = false;
  for (int ii = 0; ii < numCmds; ii++) {        // loop through table of commands
    if (strcmp(cmd, cmdList[ii].text) == 0) {   // look for it
      cmdList[ii].function();                   // found it! call the subroutine
      found = true;
      break;
    }
  }
  if (!found) {
    logger.println("Unsupported");
  }
}
