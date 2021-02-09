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
            Use the touchscreen to enter yor current sea-level barometer setting.

            +-----------------------------------------+
            | *              Altimeter                |
            |                                         |
            | Barometer:      17.8 feet               |. . .yRow1
            | GPS (5#):      123.4 feet               |. . .yRow2
            |                                         |
            | Enter your current local                |. . .yRow3
            | pressure at sea level:                  |. . .yRow4
            +-------+                         +-------+
            |   ^   |     1016.7 hPa          |   v   |. . .yRow9
            +-------+-------------------------+-------+
*/

#include <Adafruit_ILI9341.h>         // TFT color display library
#include <TimeLib.h>                  // BorisNeubert / Time (who forked it from PaulStoffregen / Time)
#include "constants.h"                // Griduino constants and colors
#include "model_gps.h"                // Model of a GPS for model-view-controller
#include "model_baro.h"               // Model of a barometer that stores 3-day history
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "view.h"                     // Base class for all views

// ========== extern ===========================================
extern Model* model;                  // "model" portion of model-view-controller
extern BarometerModel baroModel;      // singleton instance of the barometer model

extern void showDefaultTouchTargets();// Griduino.ino

// ========== class ViewAltimeter ==============================
class ViewAltimeter : public View {
  public:
    // ---------- public interface ----------
    // This derived class must implement the public interface:
    ViewAltimeter(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    {
      background = cBACKGROUND;       // every view can have its own background color
    }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
    void loadConfig();
    void saveConfig();

  protected:
    // ---------- local data for this derived class ----------
    // color scheme: see constants.h

    // "Sea Level Pressure" is stored here instead of model_gps.h, because model->save() is slow
    float gSealevelPressure = 1017.4; // default starting value, hPa; adjustable by touchscreen

    //void showReadings();
    //void drawButtons();

    // ========== text screen layout ===================================

    // vertical placement of text rows   ---label---         ---button---
    const int yRow1 = 84;             // barometer's altitude
    const int yRow2 = yRow1 + 40;     // GPS's altitude
    const int yRow3 = 172;
    const int yRow4 = yRow3 + 20;
    const int yRow9 = gScreenHeight - 14; // sea level pressure

    const int col1 = 10;              // left-adjusted column of text
    const int col2 = 200;             // right-adjusted numbers
    const int col3 = col2 + 10;
    const int colPressure = 156;
	
    // these are names for the array indexes, must be named in same order as array below
    enum txtIndex {
      eTitle=0,
      eDate, eNumSat, eTimeHHMM, eTimeSS,
      eBaroLabel, eBaroValue, eBaroUnits,
      eGpsLabel,  eGpsValue,  eGpsUnits,
      ePrompt1, /*ePrompt2,*/
      eSealevel,
    };

    #define nTextAltimeter 13
    TextField txtAltimeter[nTextAltimeter] = {
      // text            x,y    color       align       font
      {"Altitude",      -1, 18, cTITLE,     ALIGNCENTER,eFONTSMALLEST}, // [eTitle] screen title, centered
      {"mm-dd",         48, 18, cWARN,      ALIGNLEFT,  eFONTSMALLEST}, // [eDate]
      {"0#",            48, 36, cWARN,      ALIGNLEFT,  eFONTSMALLEST}, // [eNumSat]
      {"hh:mm",        276, 18, cWARN,      ALIGNRIGHT, eFONTSMALLEST}, // [eTimeHHMM]
      {"ss",           276, 36, cWARN,      ALIGNRIGHT, eFONTSMALLEST}, // [eTimeSS]
      {"Barometer:",  col1,yRow1, cLABEL,   ALIGNLEFT,  eFONTSMALL},    // [eBaroLabel]
      {"12.3",        col2,yRow1, cVALUE,   ALIGNRIGHT, eFONTSMALL},    // [eBaroValue]
      {"feet",        col3,yRow1, cLABEL,   ALIGNLEFT,  eFONTSMALL},    // [eBaroUnits]
      {"GPS:",        col1,yRow2, cLABEL,   ALIGNLEFT,  eFONTSMALL},    // [eGpsLabel]
      {"4567.8",      col2,yRow2, cVALUE,   ALIGNRIGHT, eFONTSMALL},    // [eGpsValue]
      {"feet",        col3,yRow2, cLABEL,   ALIGNLEFT,  eFONTSMALL},    // [eGpsUnits]
      {"Enter local sea level pressure:", 
                        -1,yRow4, cTEXTCOLOR,ALIGNCENTER,eFONTSMALLEST},// [ePrompt1]
      {"34.567 inHg",   -1,yRow9, cVALUE,   ALIGNCENTER, eFONTSMALLEST},// [eSealevel]
    };

    enum buttonID {
      ePressurePlus,
      ePressureMinus,
    };
    #define nPressureButtons 2
    FunctionButton pressureButtons[nPressureButtons] = {
      // For "sea level pressure" we have rather small modest +/- buttons, meant to visually
      // fade a little into the background. However, we want touch-targets larger than the
      // button's outlines to make them easy to press.
      //
      // 3.2" display is 320 x 240 pixels, landscape, (y=239 reserved for activity bar)
      //
      // label            origin   size      touch-target
      // text              x,y      w,h      x,y      w,h   radius  color     functionID
      {  "+",      160-20-90,202,  38,32, {  1,158, 159,89},  4,  cTEXTCOLOR, ePressurePlus  },  // Up
      {  "-",      160-20+90,202,  38,32, {161,158, 159,89},  4,  cTEXTCOLOR, ePressureMinus },  // Down
    };

    // ======== barometer and temperature helpers ==================

    void showTimeOfDay() {
      // fetch RTC and display it on screen
      char msg[12];                       // strlen("12:34:56") = 8
      int mo, dd, hh, mm, ss;
      if (timeStatus() == timeNotSet) {
        mo = dd = hh = mm = ss = 0;
      } else {
        time_t tt = now();
        mo = month(tt);
        dd = day(tt);
        hh = hour(tt);
        mm = minute(tt);
        ss = second(tt);
      }
    
      snprintf(msg, sizeof(msg), "%d-%02d", mo, dd);
      txtAltimeter[eDate].print(msg);
    
      snprintf(msg, sizeof(msg), "%d#", GPS.satellites);
      txtAltimeter[eNumSat].print(msg);    // show number of satellites, help give sense of positional accuracy
    
      snprintf(msg, sizeof(msg), "%02d:%02d", hh,mm);
      txtAltimeter[eTimeHHMM].print(msg);  // show time, help identify when RTC stops
    
      snprintf(msg, sizeof(msg), "%02d", ss);
      txtAltimeter[eTimeSS].print(msg);
    }

    // ----- helpers -----
    float delta() {
      // how much to change "sea level pressure" with each button press epends on english/metric setting
      float hPa;
      if (model->gMetric) {
        hPa = (0.002F)*HPA_PER_INCHES_MERCURY; // 0.003 inHg is about 2 feet
      } else {
        hPa = (0.1F);                  // 0.1 hPa is about 1 meter
      }
      return hPa;
    }
    
    void increaseSeaLevelPressure() {
      gSealevelPressure += delta();
      Serial.print("Sea level pressure increased to "); Serial.println(gSealevelPressure, 2);
    }

    void decreaseSeaLevelPressure() {
      float change = 
      gSealevelPressure -= delta();
      Serial.print("Sea level pressure decreased to "); Serial.println(gSealevelPressure, 2);
    }

};  // end class ViewAltimeter

// ============== implement public interface ================
void ViewAltimeter::updateScreen() {
  // called on every pass through main()

  // update clock display
  showTimeOfDay();

  // read altitude from barometer and GPS, and display everything
  float pascals = baroModel.getBaroPressure();  // get pressure to trigger a fresh reading
  //Serial.print("Altimeter "); Serial.print(pascals); Serial.print(" Pa ["); Serial.print(__LINE__); Serial.println("]"); // debug
  
  char msg[16];                     // strlen("12,345.6 meters") = 15

  float altMeters = baroModel.getAltitude(gSealevelPressure);
  float altFeet = altMeters*feetPerMeters;
  //Serial.print("Altimeter "); Serial.print(altFeet); Serial.print(" feet ["); Serial.print(__LINE__); Serial.println("]"); // debug
  if (model->gMetric) {
    txtAltimeter[eBaroValue].print(altMeters, 0);
  } else {
    txtAltimeter[eBaroValue].print(altFeet, 0);
  }

  // if (lost GPS position lock)
  // todo
  
  float altitude = model->gAltitude*feetPerMeters;
  txtAltimeter[eGpsValue].print(altitude, 0);

  // show sea level pressure
  if (model->gMetric) {
    // metric: hecto Pascal
    float sealevel = gSealevelPressure;
    String sFloat = String(sealevel, 1) + " hPa";
    txtAltimeter[eSealevel].print(sFloat);
  } else {
     // english: inches Mercury
    float sealevel = gSealevelPressure*INCHES_MERCURY_PER_PASCAL*100;
    String sFloat = String(sealevel, 3) + " inHg";
    txtAltimeter[eSealevel].print(sFloat);
  }

} // end updateScreen


void ViewAltimeter::startScreen() {
  // called once each time this view becomes active
  loadConfig();                       // restore our settings from NVR
  this->clearScreen(this->background);                      // clear screen
  txtAltimeter[0].setBackground(this->background);          // set background for all TextFields in this view
  TextField::setTextDirty( txtAltimeter, nTextAltimeter );  // make sure all fields get re-printed on screen change

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();          // optionally draw box around default button-touch areas
  showMyTouchTargets(pressureButtons, nPressureButtons); // optionally show this view's touch targets
  showScreenBorder();                 // optionally outline visible area
  showScreenCenterline();             // optionally draw visual alignment bar

  if (model->gMetric) {
    txtAltimeter[eBaroUnits].print("meters");
    txtAltimeter[eGpsUnits].print("meters");
  } else {
    txtAltimeter[eBaroUnits].print("feet");
    txtAltimeter[eGpsUnits].print("feet");
  }

  // ----- draw buttons
  setFontSize(eFONTSMALL);
  for (int ii=0; ii<nPressureButtons; ii++) {
    FunctionButton item = pressureButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y+item.h/2+5);  // place text centered inside button
    tft->setTextColor(item.color);
    tft->print(item.text);
  }

  // ----- draw text fields
  for (int ii=0; ii<nTextAltimeter; ii++) {
      txtAltimeter[ii].print();
  }

  updateScreen();                     // update UI immediately, don't wait for laggy mainline loop
} // end startScreen()


bool ViewAltimeter::onTouch(Point touch) {
  Serial.println("->->-> Touched altimeter screen.");

  bool handled = false;               // assume a touch target was not hit
  for (int ii=0; ii<nPressureButtons; ii++) {
    FunctionButton item = pressureButtons[ii];
    if (item.hitTarget.contains(touch)) {
        handled = true;               // hit!
        switch (item.functionIndex)   // do the thing
        {
          case ePressurePlus:
              increaseSeaLevelPressure();
              break;
          case ePressureMinus:
              decreaseSeaLevelPressure();
              break;
          default:
              Serial.print("Error, unknown function "); Serial.println(item.functionIndex);
              break;
        }
        updateScreen();               // update UI immediately, don't wait for laggy mainline loop
        this->saveConfig();           // after UI is updated, save setting to nvr
     }
  }
  return handled;                     // true=handled, false=controller uses default action
} // end onTouch()

// ========== load/save config setting =========================
// the user's sea level pressure is saved here instead of the model, to keep the 
// screen responsive. Otherwise it's slow to save the whole GPS model.
#define ALTIMETER_CONFIG_FILE    CONFIG_FOLDER "/altimetr.cfg" // must be 8.3 filename
#define CONFIG_ALTIMETER_VERSION "Altimeter v01"               // <-- always change this when changing data saved

// ----- load from SDRAM -----
void ViewAltimeter::loadConfig() {
  // Load altimeter settings from NVR

  SaveRestore config(ALTIMETER_CONFIG_FILE, CONFIG_ALTIMETER_VERSION);
  float tempPressure;
  int result = config.readConfig( (byte*) &tempPressure, sizeof(tempPressure) );
  if (result) {
    this->gSealevelPressure = tempPressure;
    Serial.print("Loaded sea level pressure: "); Serial.println(this->gSealevelPressure, 2);
  } else {
    Serial.println("Failed to load sea level pressure, re-initializing config file");
    saveConfig();
  }
}
// ----- save to SDRAM -----
void ViewAltimeter::saveConfig() {
  SaveRestore config(ALTIMETER_CONFIG_FILE, CONFIG_ALTIMETER_VERSION);
  int rc = config.writeConfig( (byte*) &gSealevelPressure, sizeof(gSealevelPressure) );
  Serial.print("Finished with rc = "); Serial.println(rc);  // debug
}
