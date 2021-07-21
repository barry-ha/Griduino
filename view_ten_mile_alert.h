// Please format this file with clang before check-in to GitHub
/*
  File:     view_ten_mile_alert.h
  
  Version history: 
            2021-07-12 created 1.04

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  

            +-----------------------------------------+
            | *          Microwave Rover            > |
            |             Ten Mile Alert              |
            |    /---\                                |
            |   /     \    00.00 miles                |
    xCenter |  |   +   |         NNE                  |
            |   \     /                               |
            |    \---/                                |
            |                                       S |
            |     Start:   CN87aa                   E |
            |     Here:    CN87bb                   T |
            |                                         |
            +-----------------------------------------+
              |     |            | |
              col1  yCenter   col2 col3
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include <TimeLib.h>            // BorisNeubert / Time (who forked it from PaulStoffregen / Time)
#include "constants.h"          // Griduino constants and colors
#include "model_gps.h"          // Model of a GPS for model-view-controller
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
extern Model *model;   // "model" portion of model-view-controller

extern void showDefaultTouchTargets();   // Griduino.ino

// ========== class ViewTenMileAlert ===========================
class ViewTenMileAlert : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewTenMileAlert(Adafruit_ILI9341 *vtft, int vid)   // ctor
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

  // "Starting Point" is stored here instead of model_gps.h,
  // because model->save() is large and much slower than saving our one value
  double startLat  = 40.0;     // approximate center of US on Kansas/Nebraska border
  double startLong = -100.0;   // 40,-100 = EN00aa

  float prevDiffLat  = 99.9;
  float prevDiffLong = 99.9;

  // ========== text screen layout ===================================

  // vertical placement of text rows   ---label---         ---button---
  const int yRow1 = 178;          // "Start at" grid square
  const int yRow2 = yRow1 + 50;   // "Now at" grid square
  const int yRow3 = 92;           // "Distance"
  const int yRow4 = yRow3 + 28;   // "NNW"
  const int col1  = 6;            // left-adjusted text
  const int col2  = 96;           // left-adjusted grid square
  const int col3  = col2 + 124;   // left-adjusted unit names

  // compass size and placement
  const int xCenter = (col1 + col2) / 2 - 8;   // pixels
  const int yCenter = yRow3;                   // pixels
  const int radius  = 36;                      // pixels

  const float DEG2RAD            = PI / 180.0f;
  const float DEGREES_PER_RADIAN = 180.0f / PI;

  // these are names for the array indexes, must be named in same order as array below
  enum txtIndex {
    eTitle1 = 0,
    eTitle2,
    eDistance,
    eUnits,           // miles, km
    eDirectionName,   // N, S, E, W, ...
    eStartLabel,
    eStartValue,
    eCurrentLabel,
    eCurrentValue,
  };

#define nTextTenMileAlert 9
  TextField txtTenMileAlert[nTextTenMileAlert] = {
      // text             x,y      color      align       font
      {"Microwave Rover", -1, 18, cTITLE, ALIGNCENTER, eFONTSMALLEST},   // [eTitle1] centered
      {"Ten Mile Alert", -1, 40, cTITLE, ALIGNCENTER, eFONTSMALLEST},    // [eTitle2] centered
                                                                         //
      {"12.34", col3 - 10, yRow3, cVALUE, ALIGNRIGHT, eFONTBIG},         // [eDistance]
      {"miles", col3, yRow3, cVALUE, ALIGNLEFT, eFONTSMALL},             // [eUnits]
      {"NNE", -1, yRow4, cVALUE, ALIGNCENTER, eFONTSMALL},               // [eDirectionName]
                                                                         //
      {"Start:", col1, yRow1, cLABEL, ALIGNLEFT, eFONTSMALL},            // [eStartLabel]
      {"CN87us", col2, yRow1, cTEXTCOLOR, ALIGNLEFT, eFONTBIG},          // [eStartValue]
                                                                         //
      {"Here:", col1, yRow2, cLABEL, ALIGNLEFT, eFONTSMALL},             // [eCurrentLabel]
      {"CN87vv", col2, yRow2, cTEXTCOLOR, ALIGNLEFT, eFONTBIG},          // [eCurrentValue]
  };

  enum buttonID {
    eSetStart,
  };
#define nTenMileAlertButtons 1
  FunctionButton tenMileAlertButtons[nTenMileAlertButtons] = {
      // "Set Start" is a small rhs button meant to visually fade into the background.
      // However, we want touch-targets larger than the button's outline to make it easy to press.
      //
      // 3.2" display is 320 x 240 pixels, landscape, (y=239 reserved for activity bar)
      //
      // label            origin   size      touch-target
      // text              x,y      w,h      x,y      w,h    radius  color     functionID
      {"", 290, 96, 29, 84, {210, 108, 109, 128}, 4, cTEXTCOLOR, eSetStart},
  };

  // ======== helpers =========================================
  void setStartingPoint() {   // drop pushpin to begin measuring distances
    startLat  = model->gLatitude;
    startLong = model->gLongitude;

    Serial.print("Set new starting point: ");
    Serial.print(startLat, 4);
    Serial.print(", ");
    Serial.println(startLong, 4);

    startCompass();
    clearDirectionName();
  }

  void clearDirectionName() {
    txtTenMileAlert[eDirectionName].print("");
  }

  void startCompass() {
    // erase compass area and start over
    int x0 = xCenter - radius;
    int y0 = yCenter - radius;
    int w  = 2 * radius;
    int h  = w;
    tft->fillRect(x0, y0, w, h, cBACKGROUND);

    // outer circle, centered on (xCenter, yCenter)
    tft->drawCircle(xCenter, yCenter, radius, cHIGHLIGHT);

    // tick marks
    /* commented out, it's just visual clutter
    tft->drawLine(xCenter, yCenter - radius - 3, xCenter, yCenter - radius, cHIGHLIGHT);   // N
    tft->drawLine(xCenter, yCenter + radius + 3, xCenter, yCenter + radius, cHIGHLIGHT);   // S
    tft->drawLine(xCenter - radius - 3, yCenter, xCenter - radius, yCenter, cHIGHLIGHT);   // W
    tft->drawLine(xCenter + radius + 3, yCenter, xCenter + radius, yCenter, cHIGHLIGHT);   // E
    */
  }

  float updateCompass(double diffLat, double diffLong, uint16_t color) {
    // called for both drawing and erasing compass pointer
    float theta = atan2(diffLat, diffLong);   // returns angle in radians

    // draw new arrow inside compass circle
    float xStart = xCenter - (radius - 3) * cos(theta);
    float yStart = yCenter + (radius - 3) * sin(theta);

    float xEnd = xCenter + (radius - 4) * cos(theta);
    float yEnd = yCenter - (radius - 4) * sin(theta);

    // draw primary line in arrow
    tft->drawLine(xStart, yStart, xEnd, yEnd, color);

    // draw origin of arrow
    /* Works fine, but it's heavy and draws too much attention away from arrowhead
    tft->drawCircle(xStart, yStart, 2, color);
    */

    /* Works fine, but a short line doesn't look good at this small scale
    float tailLength = radius * 0.10;
    float tailAngle = theta + (PI / 2.0); // = (90.0 * DEG2RAD);
    int x0 = xStart - tailLength * cos(tailAngle);
    int y0 = yStart + tailLength * sin(tailAngle);
    int x1 = xStart + tailLength * cos(tailAngle);
    int y1 = yStart - tailLength * sin(tailAngle);
    tft->drawLine(x0, y0, x1, y1, color);
    */

    // arrowhead at (xEnd,yEnd) is two short lines about +/-10 degrees from theta
    float arLength = radius * 0.5;   // arrowhead is short, about 10% of diameter of circle
    float arAngle  = theta + (8.0 * DEG2RAD);
    tft->drawLine(xEnd, yEnd,   // from end of circle..
                  xEnd - arLength * cos(arAngle),
                  yEnd + arLength * sin(arAngle),
                  color);
    arAngle = theta - (8.0 * DEG2RAD);
    tft->drawLine(xEnd, yEnd,   // from end of circle..
                  xEnd - arLength * cos(arAngle),
                  yEnd + arLength * sin(arAngle),
                  color);

    return theta;   // returns angle (radians)
  }

  void updateDistance(double dist) {
    // show this distance on the screen
    int digits;
    if (dist < 99.9) {
      digits = 2;
    } else if (dist < 999.9) {
      digits = 1;
    } else {
      digits = 0;
    }
    txtTenMileAlert[eDistance].print(dist, digits);
  }

  const char *names[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

  void updateDirectionName(float theta) {
    // show N, NNW, NW, WNW, W, ...
    // input: direction in radians
    int degr = theta * (-1.0) * DEGREES_PER_RADIAN;   // radians ccw, degrees clockwise
    degr += 90;                                       // 0 radians = 90 degrees = East
    if (degr < 0) {
      degr += 360;
    }

    int index = (degr + 45) / 45;
    Serial.print("Heading ");
    Serial.print(theta, 2);
    Serial.print(" radians, ");
    Serial.print(degr);
    Serial.print(" degrees, index ");
    Serial.print(index);
    Serial.print(", ");
    if (0 <= index && index < 8) {
      Serial.print(names[index]);
    } else {
      Serial.print("?");
    }
    Serial.println();

    if (0 <= index && index < 8) {
      txtTenMileAlert[eDirectionName].print(names[index]);
    } else {
      txtTenMileAlert[eDirectionName].print("?");
    }
  }

};   // end class ViewTenMileAlert

// ============== implement public interface ================
void ViewTenMileAlert::updateScreen() {
  // called on every pass through main()

  // compute distance
  double dist = model->calcDistance(startLat, startLong, model->gLatitude, model->gLongitude);
  updateDistance(dist);

  // draw starting and current grid text
  char grid6[7];
  calcLocator(grid6, startLat, startLong, 6);
  txtTenMileAlert[eStartValue].print(grid6);

  calcLocator(grid6, model->gLatitude, model->gLongitude, 6);
  txtTenMileAlert[eCurrentValue].print(grid6);

  // compute (x,y) intercept from line starting at (0,0) and a unit circle
  // test data:  From     CN87us50  47.752581, -122.284038
  //             To       CN87vv58  47.912052, -122.204514
  //             Distance 11.62 miles
  //             Bearing  19.43 degrees
  //float deltaLat  = (47.912052 - 47.752581);   // CN87vv58 - CN87us50
  //float deltaLong = (-122.284038 - (-122.204514));

  // test data:  From     DN08pu    48.855186, -118.698004
  //             To       CN87us    47.772743, -122.287847
  //             Distance 181.05 miles
  //             Bearing  246.9 degrees
  //float deltaLat  = (47.772743 - 48.855186);   // CN87vv58 - CN87us50
  //float deltaLong = (-118.698004 - (-122.287847));

  float deltaLat  = model->gLatitude - startLat;
  float deltaLong = model->gLongitude - startLong;
  if (dist > 0.005) {
    // GPS will drift while you're stationary, so pick a small circle around
    // the Set point to keep the noise down
    // "0.01" miles is 52 feet
    if (deltaLat != prevDiffLat || deltaLong != prevDiffLong) {
      updateCompass(prevDiffLat, prevDiffLong, cBACKGROUND);     // erase old arrow
      float rads = updateCompass(deltaLat, deltaLong, cVALUE);   // draw new arrow
      updateDirectionName(rads);

      prevDiffLat  = deltaLat;
      prevDiffLong = deltaLong;
    }
  }

}   // end updateScreen

void ViewTenMileAlert::startScreen() {
  // called once each time this view becomes active
  loadConfig();                                                  // restore from NVR
  this->clearScreen(this->background);                           // clear screen
  txtTenMileAlert[0].setBackground(this->background);            // set all TextField's background
  TextField::setTextDirty(txtTenMileAlert, nTextTenMileAlert);   // redraw all fields on startScreen()

  drawAllIcons();                                                  // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();                                       // optionally draw box around default button-touch areas
  showMyTouchTargets(tenMileAlertButtons, nTenMileAlertButtons);   // optionally show this view's touch targets
  showScreenBorder();                                              // optionally outline visible area
  showScreenCenterline();                                          // optionally draw visual alignment bar

  // ----- draw text fields
  for (int ii = 0; ii < nTextTenMileAlert; ii++) {
    txtTenMileAlert[ii].print();
  }

  if (model->gMetric) {
    txtTenMileAlert[eUnits].print("km");
  } else {
    txtTenMileAlert[eUnits].print("miles");
  }

  // ----- draw static parts of direction indicators
  startCompass();
  clearDirectionName();

  // ----- draw text vertically onto "Set Start" button
  // for vertical text, temporarily rotate TFT screen into portrait mode
  int savedRotation = tft->getRotation();
  int newRotation   = (savedRotation + 3) % 4;
  tft->setRotation(newRotation);   // set portrait mode
  const int xx = tft->width() / 4;
  const int yy = tft->height() - 12;
  TextField sync("Set", xx, yy, cFAINT, ALIGNLEFT, eFONTSMALL);
  sync.print();
  tft->setRotation(savedRotation);   // restore screen orientation

  updateScreen();   // update UI immediately, don't wait for laggy mainline loop
}   // end startScreen()

void ViewTenMileAlert::endScreen() {
  // Called once each time this view becomes INactive
  // This is a 'goodbye kiss' to do cleanup work
  // We save our settings here instead of on each button press
  // because writing to NVR is slow (0.5 sec) and would delay the user
  // while trying to press a button many times in a row.
  //saveConfig(); todo
}

bool ViewTenMileAlert::onTouch(Point touch) {
  Serial.println("->->-> Touched 10-mile alert screen.");

  bool handled = false;   // assume a touch target was not hit
  for (int ii = 0; ii < nTenMileAlertButtons; ii++) {
    FunctionButton item = tenMileAlertButtons[ii];
    if (item.hitTarget.contains(touch)) {
      switch (item.functionIndex)   // do the thing
      {
      case eSetStart:
        setStartingPoint();
        handled = true;
        break;
      default:
        Serial.print("Error, unknown function ");
        Serial.println(item.functionIndex);
        break;
      }
      updateScreen();   // update UI immediately, don't wait for laggy mainline loop
    }
  }
  if (!handled) {
    Serial.println("No match to my hit targets.");   // debug
  }
  return handled;   // true=handled, false=controller uses default action
}   // end onTouch()

// ========== load/save config setting =========================
// the user's sea level pressure is saved here instead of the model, to keep the
// screen responsive. Otherwise it's slow to save the whole GPS model.
//#define ALTIMETER_CONFIG_FILE    CONFIG_FOLDER "/altimetr.cfg" // must be 8.3 filename
//#define CONFIG_ALTIMETER_VERSION "Altimeter v01"               // <-- always change this when changing data saved

// ----- load from SDRAM -----
void ViewTenMileAlert::loadConfig() {
  // Load altimeter settings from NVR

  // todo

  /*
  SaveRestore config(ALTIMETER_CONFIG_FILE, CONFIG_ALTIMETER_VERSION);
  float tempPressure;
  int result = config.readConfig( (byte*) &tempPressure, sizeof(tempPressure) );
  if (result) {
    
    this->sealevelPa = tempPressure;
    Serial.print("Loaded sea level pressure: "); Serial.println(this->sealevelPa, 1);
  } else {
    Serial.println("Failed to load sea level pressure, re-initializing config file");
    saveConfig();
  }
  */
}
// ----- save to SDRAM -----
void ViewTenMileAlert::saveConfig() {
  /* todo 
  SaveRestore config(ALTIMETER_CONFIG_FILE, CONFIG_ALTIMETER_VERSION);
  int rc = config.writeConfig( (byte*) &sealevelPa, sizeof(sealevelPa) );
  Serial.print("Finished with rc = "); Serial.println(rc);  // debug
  */
}
