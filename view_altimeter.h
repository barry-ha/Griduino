#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     view_altimeter.h

  Version history:
            2021-02-06 merged from examples/altimeter into a Griduino view
            2021-01-30 added support for BMP390 and latest Adafruit_BMP3XX library
            2020-05-12 updated TouchScreen code
            2020-03-06 created 0.9

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Display altimeter readings on a 3.2" display.
            Show comparison of Barometric result to GPS result.
            - Barometer reading requires frequent calibration with a known altitude or sea level pressure.
            - GPS result can vary by a few hundred feet depending on satellites overhead.
            Use the touchscreen to enter your current sea-level barometer setting.
            As much as possible, this module uses Pascals (not hPa).

            +-------------------------------------------+
            | *               Altitude                  |
            |                                        +--+
            |                                       | S |
            | Barometer:       17.8 feet            | y |. . .yRow1
            | GPS (5#):       123.4 feet            | n |. . .yRow2
            |                                       | c |
            | Enter local sea level pressure        +--+|
            | Accuracy depends on your input            |
            |                                           |
            +-------+       30.150 inHg         +-------+
            |   ^   |       1016.7 hPa          |   v   |
            +-------+---------------------------+-------+
              :                  : :
              col1            col2 col3
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include <TimeLib.h>            // time_t=seconds since Jan 1, 1970, https://github.com/PaulStoffregen/Time
#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "model_gps.h"          // Model of a GPS for model-view-controller
#include "model_baro.h"         // Model of a barometer that stores 3-day history
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
extern Logger logger;              // Griduino.ino
extern Model *model;               // "model" portion of model-view-controller
extern BarometerModel baroModel;   // singleton instance of the barometer model

extern void showDefaultTouchTargets();   // Griduino.ino

// ========== class ViewAltimeter ==============================
class ViewAltimeter : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewAltimeter(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background = cBACKGROUND;   // every view can have its own background color
  }
  void updateScreen();
  void startScreen();
  void endScreen();
  bool onTouch(Point touch);
  void loadConfig();
  void saveConfig();

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  // "Sea Level Pressure" is stored here instead of model_gps.h,
  // because model->save() is large and much slower than saving our one value
  float sealevelPa = DEFAULT_SEALEVEL_PASCALS;   // default starting value, Pascals; adjustable by touchscreen

  // ========== text screen layout ===================================

  // vertical placement of text rows   ---label---         ---button---
  const int yRow1 = 78;           // barometer's altitude
  const int yRow2 = yRow1 + 44;   // GPS's altitude

  const int col1 = 6;           // left-adjusted text
  const int col2 = 238;         // right-adjusted altitude
  const int col3 = col2 + 12;   // left-adjusted unit names

  // ========== text screen layout ===================================
  // these are names for the array indexes, must be named in same order as array below
  enum txtIndex {
    eTitle = 0,
    eDate,
    eNumSat,
    eTimeHHMM,
    eTimeSS,
    eBaroLabel,
    eBaroValue,
    eBaroUnits,
    eGpsLabel,
    eGpsValue,
    eGpsUnits,
    ePrompt1,
    ePrompt2,
    eSealevelEnglish,
    eSealevelMetric,
  };

  // ----- static + dynamic screen text
  // clang-format off
#define nTextAltimeter 15
  TextField txtAltimeter[nTextAltimeter] = {
      {"Altitude", -1, 18,    cTITLE,        ALIGNCENTER,  eFONTSMALLEST},   // [eTitle] screen title, centered
      {"mm-dd",    60, 18,     cWARN,          ALIGNLEFT,  eFONTSMALLEST},    // [eDate]
      {"0#",       60, 36,     cWARN,          ALIGNLEFT,  eFONTSMALLEST},    // [eNumSat]
      {"hh:mm",   276, 18,     cWARN,          ALIGNRIGHT, eFONTSMALLEST},    // [eTimeHHMM]
      {"ss",      276, 36,     cWARN,          ALIGNRIGHT, eFONTSMALLEST},    // [eTimeSS]

      {"Barometer:", col1, yRow1, cLABEL, ALIGNLEFT, eFONTSMALL},      // [eBaroLabel]
      {"12.3",       col2, yRow1, cVALUE, ALIGNRIGHT, eFONTBIG},       // [eBaroValue]
      {"ft",         col3, yRow1, cLABEL, ALIGNLEFT, eFONTSMALL},      // [eBaroUnits]

      {"GPS:",       col1, yRow2, cLABEL, ALIGNLEFT, eFONTSMALL},      // [eGpsLabel]
      {"4567.8",     col2, yRow2, cVALUE, ALIGNRIGHT, eFONTBIG},       // [eGpsValue]
      {"ft", col3,  yRow2, cLABEL, ALIGNLEFT, eFONTSMALL},             // [eGpsUnits]

      {"Enter local sea level pressure.",
       -1, 162, cFAINT, ALIGNCENTER, eFONTSMALLEST},                  // [ePrompt1]
      {"Accuracy depends on your input.",
       -1, 184, cFAINT, ALIGNCENTER, eFONTSMALLEST},                  // [ePrompt21]
      {"34.567 inHg", -1, 208, cVALUE, ALIGNCENTER, eFONTSMALLEST},   // [eSealevelEnglish]
      {"1111.1 hPa", -1, 230, cVALUE, ALIGNCENTER, eFONTSMALLEST},    // [eSealevelMetric]
  };
  // clang-format on

  enum buttonID {
    ePressurePlus,
    ePressureMinus,
    eSynchronize,
  };
  // clang-format off
#define nPressureButtons 3
  FunctionButton pressureButtons[nPressureButtons] = {
      // For "sea level pressure" we have rather small modest +/- buttons, meant to visually
      // fade a little into the background. However, we want touch-targets larger than the
      // button's outlines to make them easy to press.
      //
      // 3.2" display is 320 x 240 pixels, landscape, (y=239 reserved for activity bar)
      //
      // label         origin      size      touch-target
      // text           x,y        w,h        x,y       w,h    radius  color     functionID
      {"+", 160 - 20 - 98, 198,   42, 36,  {  1, 158, 159, 89},  4, cTEXTCOLOR, ePressurePlus },   // Up
      {"-", 160 - 20 + 98, 198,   42, 36,  {161, 158, 159, 89},  4, cTEXTCOLOR, ePressureMinus},   // Down
      {"",            280,  46,   39, 84,  {220,  48,  99, 84},  4, cTEXTCOLOR, eSynchronize  },   // sync readings
  };
  // clang-format on

  // ======== barometer and temperature helpers ==================

  void showTimeOfDay() {
    // fetch RTC and display it on screen
    char msg[12];   // strlen("12:34:56") = 8
    int mo, dd, hh, mm, ss;
    if (timeStatus() == timeNotSet) {
      mo = dd = hh = mm = ss = 0;
    } else {
      time_t tt = now();
      mo        = month(tt);
      dd        = day(tt);
      hh        = hour(tt);
      mm        = minute(tt);
      ss        = second(tt);
    }

    snprintf(msg, sizeof(msg), "%d-%02d", mo, dd);
    txtAltimeter[eDate].print(msg);

    snprintf(msg, sizeof(msg), "%d#", GPS.satellites);
    txtAltimeter[eNumSat].print(msg);   // show number of satellites, help give sense of positional accuracy

    snprintf(msg, sizeof(msg), "%02d:%02d", hh, mm);
    txtAltimeter[eTimeHHMM].print(msg);   // show time, help identify when RTC stops

    snprintf(msg, sizeof(msg), "%02d", ss);
    txtAltimeter[eTimeSS].print(msg);
  }

  // ----- helpers -----
  float delta() {
    // calculates a minimum visible altitude change
    // returns a small pressure change, Pascals

    // how much to change "sea level pressure" with each button press depends on english/metric setting
    float pascals;
    if (model->gMetric) {
      pascals = (0.002F) * PASCALS_PER_INCHES_MERCURY;   // 0.002 inHg = 6.0 Pa = about 2 feet
    } else {
      pascals = (10.0F);   // 10 Pa is about 1 meter
    }
    return pascals;
  }

  void increaseSeaLevelPressure() {
    sealevelPa += delta();
    logger.logTwoFloats(BARO, DEBUG, "Sea level pressure increased by %s to %s Pa", delta(), 2, sealevelPa, 1);
  }

  void decreaseSeaLevelPressure() {
    sealevelPa -= delta();
    logger.logTwoFloats(BARO, DEBUG, "Sea level pressure decreased by %s to %s Pa", delta(), 2, sealevelPa, 1);
  }

  void syncBarometerToGPS() {
    int count          = 0;                                   // loop counter for debug
    float baroAltitude = baroModel.getAltitude(sealevelPa);   // meters
    float gpsAltitude  = model->gAltitude;                    // meters

    if (baroAltitude < gpsAltitude) {
      logger.log(BARO, DEBUG, "~~> Barometer's altitude is lower than GPS' altitude.");
      logger.logTwoFloats(BARO, DEBUG, ". INCREASE sea level pressure from %s Pa in steps of %s Pa", sealevelPa, 1, delta(), 2);

      // loop using _increasing_ pressure until altitudes match
      while (baroAltitude < model->gAltitude) {
        sealevelPa += delta();
        if (++count > 1000)
          break;   // avoid infinite loop
        baroAltitude = baroModel.getAltitude(sealevelPa);
      }
    } else {
      logger.log(BARO, DEBUG, "~~> Barometer's altitude is higher than GPS' altitude.");
      logger.logTwoFloats(BARO, DEBUG, ". DECREASE sea level pressure from %s Pa in steps of %s Pa", sealevelPa, 1, delta(), 2);

      // loop using _decreasing_ pressure until altitudes match
      while (baroAltitude > gpsAltitude) {
        sealevelPa -= delta();
        if (++count > 1000)
          break;   // avoid infinite loop
        baroAltitude = baroModel.getAltitude(sealevelPa);
      }
    }

    logger.logTwoFloats(BARO, DEBUG, ". Altimeter synchronized to GPS at %s Pa, %s steps", sealevelPa, 1, count, 0);

    char msg[128], sPressure[12];
    floatToCharArray(sPressure, sizeof(sPressure), sealevelPa, 1);
    snprintf(msg, sizeof(msg), ". Altimeter synchronized to GPS at %s Pa, %d steps", sPressure, count);
    logger.log(BARO, DEBUG, msg);
  }

};   // end class ViewAltimeter

// ============== implement public interface ================
void ViewAltimeter::updateScreen() {
  // called on every pass through main()

  // update clock display
  showTimeOfDay();

  // read altitude from barometer and GPS, and display everything
  // float pascals = baroModel.getBaroPressure();   (unused) // get pressure, causing BMP3XX to take a fresh reading from sensor
  // char msg[16];   // strlen("12,345.6 meters") = 15 (unused)

  float altMeters = baroModel.getAltitude(sealevelPa);
  float altFeet   = altMeters * feetPerMeters;
  // altMeters += 2000;              // debug, helps test layout with large numbers
  // altFeet += 2000;                // debug
  if (model->gMetric) {
    int precision = (abs(altMeters) < 10) ? 1 : 0;
    txtAltimeter[eBaroValue].print(altMeters, precision);
  } else {
    txtAltimeter[eBaroValue].print(altFeet, 0);
  }

  // if (lost GPS position lock)
  // todo - maybe the 'sync' button should be disabled when GPS is lost

  float gpsMeters = model->gAltitude;   // meters above MSL
  float gpsFeet   = gpsMeters * feetPerMeters;
  if (model->gMetric) {
    int precision = (abs(gpsMeters) < 10) ? 1 : 0;
    txtAltimeter[eGpsValue].print(gpsMeters, precision);
  } else {
    txtAltimeter[eGpsValue].print(gpsFeet, 0);
  }

  // show sea level pressure
  // english: inches Mercury
  float pressureInHg = sealevelPa * INCHES_MERCURY_PER_PASCAL;
  String sFloat      = String(pressureInHg, 3) + " inHg";
  txtAltimeter[eSealevelEnglish].print(sFloat);

  // metric: hecto Pascal
  float pressureHPa = sealevelPa / 100;
  sFloat            = String(pressureHPa, 1) + " hPa";
  txtAltimeter[eSealevelMetric].print(sFloat);

}   // end updateScreen

void ViewAltimeter::startScreen() {
  // called once each time this view becomes active
  logger.log(SCREEN, DEBUG, "ViewAltimiter::startScreen()");
  loadConfig();   // restore our settings from NVR
  // this->sealevelPa = DEFAULT_SEALEVEL_PASCALS;  // debug: one-time override NVR
  this->clearScreen(this->background);                     // clear screen
  txtAltimeter[0].setBackground(this->background);         // set background for all TextFields in this view
  TextField::setTextDirty(txtAltimeter, nTextAltimeter);   // make sure all fields get re-printed on screen change

  drawAllIcons();                                          // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();                               // optionally draw box around default button-touch areas
  showMyTouchTargets(pressureButtons, nPressureButtons);   // optionally show this view's touch targets
  showScreenBorder();                                      // optionally outline visible area
  showScreenCenterline();                                  // optionally draw visual alignment bar

  if (model->gMetric) {
    txtAltimeter[eBaroUnits].print("m");
    txtAltimeter[eGpsUnits].print("m");
  } else {
    txtAltimeter[eBaroUnits].print("ft");
    txtAltimeter[eGpsUnits].print("ft");
  }

  // ----- draw buttons
  setFontSize(eFONTSMALL);
  for (int ii = 0; ii < nPressureButtons; ii++) {
    FunctionButton item = pressureButtons[ii];
    if (ii == eSynchronize) {
      // nothing - the button on top of the altimeter readout doesn't change background color
    } else {
      // show background color for normal buttons
      tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
      tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);
    }

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y + item.h / 2 + 5);   // place text centered inside button
    tft->setTextColor(item.color);
    tft->print(item.text);
  }

  // ----- draw text fields
  for (int ii = 0; ii < nTextAltimeter; ii++) {
    txtAltimeter[ii].print();
  }

  // ----- draw text vertically onto "Sync" button
  // for vertical text, temporarily rotate TFT screen into portrait mode
  int savedRotation = tft->getRotation();
  int newRotation   = (savedRotation + 3) % 4;
  tft->setRotation(newRotation);   // set portrait mode
  const int xx = tft->width() / 2 + 12;
  const int yy = tft->height() - 10;
  TextField sync("Sync", xx, yy, cFAINT);
  sync.print();
  tft->setRotation(savedRotation);   // restore screen orientation

  updateScreen();   // update UI immediately, don't wait for laggy mainline loop
}   // end startScreen()

void ViewAltimeter::endScreen() {
  // Called once each time this view becomes INactive
  // This is a 'goodbye kiss' to do cleanup work
  // We save our settings here instead of on each button press
  // because writing to NVR is slow (0.5 sec) and would delay the user
  // while trying to press a button many times in a row.
  saveConfig();

}   // end startScreen()

bool ViewAltimeter::onTouch(Point touch) {
  logger.log(CONFIG, INFO, "->->-> Touched altimeter screen.");

  bool handled = false;   // assume a touch target was not hit
  for (int ii = 0; ii < nPressureButtons; ii++) {
    FunctionButton item = pressureButtons[ii];
    if (item.hitTarget.contains(touch)) {
      switch (item.functionIndex)   // do the thing
      {
      case ePressurePlus:
        increaseSeaLevelPressure();
        handled = true;
        break;
      case ePressureMinus:
        decreaseSeaLevelPressure();
        handled = true;
        break;
      case eSynchronize:
        syncBarometerToGPS();
        handled = true;
        break;
      default:
        logger.log(CONFIG, ERROR, "unknown function %d", item.functionIndex);
        break;
      }
      updateScreen();   // update UI immediately, don't wait for laggy mainline loop
    }
  }
  if (!handled) {
    logger.log(CONFIG, DEBUG, "No match to my hit targets.");
  }
  return handled;   // true=handled, false=controller uses default action

}   // end onTouch()

// ========== load/save config setting =========================
// Save the user's sea level pressure
// Save it here instead of the model, to keep the screen responsive.
// Otherwise it's slow to save the whole GPS model.
#define ALTIMETER_CONFIG_FILE    CONFIG_FOLDER "/altimetr.cfg"   // must be 8.3 filename
#define CONFIG_ALTIMETER_VERSION "Altimeter v01"                 // <-- always change this when changing data saved

// ----- load from SDRAM -----
void ViewAltimeter::loadConfig() {
  // Load altimeter settings from NVR

  SaveRestore config(ALTIMETER_CONFIG_FILE, CONFIG_ALTIMETER_VERSION);
  float tempPressure;
  int result = config.readConfig((byte *)&tempPressure, sizeof(tempPressure));
  if (result) {
    this->sealevelPa = tempPressure;
    logger.log(CONFIG, INFO, "Loaded sea level pressure: %s", this->sealevelPa, 1);
  } else {
    logger.log(CONFIG, ERROR, "Failed to load sea level pressure, re-initializing config file");
    saveConfig();
  }
}
// ----- save to SDRAM -----
void ViewAltimeter::saveConfig() {
  SaveRestore config(ALTIMETER_CONFIG_FILE, CONFIG_ALTIMETER_VERSION);
  config.writeConfig((byte *)&sealevelPa, sizeof(sealevelPa));
}
