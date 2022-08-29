#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     cfg_audio_type.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This selects the audio output type: Morse code or speech.
            Since it's not intended for a driver in motion, we can use
            a smaller font and cram more stuff onto the screen.

            +-----------------------------------------+
            |             2. Audio Type               |
            |                                         |
            | Announcements      (o)[ Morse code ]    |... yRow1
            |                                         |
            |                    ( )[ Spoken word ]   |... yRow2
            |                                         |
            |                    ( )[ No audio ]      |... yRow3
            |                                         |
            | v0.36, Apr 6 2021                       |
            +-----------------------------------------+
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>   // TFT color display library
#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "grid_helper.h"        // lat/long conversion routines
#include "model_gps.h"          // Model of a GPS for model-view-controller
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
extern Logger logger;   // Griduino.ino
extern Grids grid;      // grid_helper.h
extern Model *model;    // "model" portion of model-view-controller

extern void showDefaultTouchTargets();                // Griduino.ino
extern void announceGrid(String gridName, int len);   // Griduino.ino

// ========== class ViewCfgAudioType ==============================
class ViewCfgAudioType : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewCfgAudioType(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background    = cBACKGROUND;   // every view can have its own background color
    selectedAudio = MORSE;         // default to Morse code (until we read setting from Flash)
  }
  void updateScreen();
  void startScreen();
  void endScreen();
  bool onTouch(Point touch);
  void loadConfig();
  void saveConfig();

  enum functionID {
    MORSE = 0,
    SPEECH,
    NO_AUDIO,
  };
  functionID selectedAudio;   // <-- this is the whole reason for this module's existence

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  // vertical placement of text rows   ---label---           ---button---
  const int yRow1 = 80;                   // "Announcements", "Morse code"
  const int yRow2 = yRow1 + 52;           //                  "Spoken word"
  const int yRow3 = yRow2 + 52;           //                  "No audio"
  const int yRow9 = gScreenHeight - 12;   // "v0.38, Apr 13 2021"

#define col1    10    // left-adjusted column of text
#define xButton 160   // indented column of buttons

  // these are names for the array indexes, must be named in same order as array below
  enum txtSettings5 {
    SETTINGS = 0,
    ANNOUNCEMENTS,
    ANNOUNCEMENTS2,
    COMPILED,
  };

#define nTxtSettings5 4
  TextField txtSettings5[nTxtSettings5] = {
      //        text                x, y        color                      enum
      TextField("2. Audio Type", col1, 20, cHIGHLIGHT, ALIGNCENTER),   // [SETTINGS]
      TextField("Announce", col1, yRow1, cVALUE),                      // [ANNOUNCEMENTS]
      TextField("grids using:", col1, yRow1 + 20, cVALUE),             // [ANNOUNCEMENTS2]
      TextField(PROGRAM_VERSION ", " __DATE__,                         // [COMPILED]
                col1, yRow9, cLABEL, ALIGNCENTER),
  };

#define nButtonsAudio 3
  FunctionButton myButtons[nButtonsAudio] = {
      // label            origin          size      touch-target
      // text               x,y            w,h       x,y           w,h  radius color  functionID
      {"Morse code", xButton, yRow1 - 26, 140, 40, {120, yRow1 - 42, 195, 62}, 4, cVALUE, MORSE},
      {"Spoken word", xButton, yRow2 - 26, 140, 40, {120, yRow2 - 32, 195, 52}, 4, cVALUE, SPEECH},
      {"No audio", xButton, yRow3 - 26, 140, 40, {120, yRow3 - 32, 195, 62}, 4, cVALUE, NO_AUDIO},
  };

  // ---------- local functions for this derived class ----------
  void setMorse() {
    logger.info("->->-> Clicked MORSE CODE button.");
    selectedAudio = MORSE;
    updateScreen();   // update UI before the long pause to send sample audio

    // announce grid name for an audible example of this selection
    char newGrid4[7];
    grid.calcLocator(newGrid4, model->gLatitude, model->gLongitude, 4);
    announceGrid(newGrid4, 4);   // announce 4-digit grid by Morse code
  }

  void setSpeech() {
    selectedAudio = SPEECH;
    updateScreen();   // update UI before the long pause to send sample audio

    // announce grid square for an audible example of this selection
    char newGrid4[7];
    grid.calcLocator(newGrid4, model->gLatitude, model->gLongitude, 4);
    announceGrid(newGrid4, 4);   // announce 4-digit grid by Morse code OR speech
  }
  void setNone() {
    selectedAudio = NO_AUDIO;
  }

};   // end class ViewCfgAudioType

// ============== implement public interface ================
void ViewCfgAudioType::updateScreen() {
  // called on every pass through main()

  // ----- show selected radio buttons by filling in the circle
  for (int ii = 0; ii < nButtonsAudio; ii++) {
    FunctionButton item = myButtons[ii];
    int xCenter         = item.x - 16;
    int yCenter         = item.y + (item.h / 2);
    int buttonFillColor = cBACKGROUND;

    if (ii == MORSE && selectedAudio == MORSE) {
      buttonFillColor = cLABEL;
    }
    if (ii == SPEECH && selectedAudio == SPEECH) {
      buttonFillColor = cLABEL;
    }
    if (ii == NO_AUDIO && selectedAudio == NO_AUDIO) {
      buttonFillColor = cLABEL;
    }
    tft->fillCircle(xCenter, yCenter, 4, buttonFillColor);
  }
}   // end updateScreen

void ViewCfgAudioType::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                    // clear screen
  txtSettings5[0].setBackground(this->background);        // set background for all TextFields in this view
  TextField::setTextDirty(txtSettings5, nTxtSettings5);   // make sure all fields get re-printed on screen change

  drawAllIcons();              // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();   // optionally draw box around default button-touch areas
  showMyTouchTargets(myButtons, nButtonsAudio);   // optionally show this view's touch targets
  showScreenBorder();          // optionally outline visible area
  showScreenCenterline();      // optionally draw visual alignment bar

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii = 0; ii < nButtonsAudio; ii++) {
    FunctionButton item = myButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y + item.h / 2 + 5);   // place text centered inside button
    tft->setTextColor(item.color);
    tft->print(item.text);
  }

  // ----- draw outlines of radio buttons
  for (int ii = 0; ii < nButtonsAudio; ii++) {
    FunctionButton item = myButtons[ii];
    int xCenter         = item.x - 16;
    int yCenter         = item.y + (item.h / 2);

    // outline the radio button
    // the active button will be indicated in updateScreen()
    tft->drawCircle(xCenter, yCenter, 7, cVALUE);
  }

  // ----- draw text fields
  for (int ii = 0; ii < nTxtSettings5; ii++) {
    txtSettings5[ii].print();
  }

  showScreenBorder();       // optionally outline visible area
  showScreenCenterline();   // optionally draw alignment bar

  updateScreen();   // update UI immediately, don't wait for laggy mainline loop
}   // end startScreen()

void ViewCfgAudioType::endScreen() {
  // Called once each time this view becomes INactive
  // This is a 'goodbye kiss' to do cleanup work
  // For the current configuration screen; save our settings here instead of on each
  // button press because writing to NVR is slow (0.5 sec) and would delay the user
  // while trying to press a button many times in a row.
  saveConfig();
}

bool ViewCfgAudioType::onTouch(Point touch) {
  logger.info("->->-> Touched settings screen.");
  bool handled = false;   // assume a touch target was not hit
  for (int ii = 0; ii < nButtonsAudio; ii++) {
    FunctionButton item = myButtons[ii];
    if (item.hitTarget.contains(touch)) {
      handled = true;               // hit!
      switch (item.functionIndex)   // do the thing
      {
      case MORSE:
        setMorse();
        break;
      case SPEECH:
        setSpeech();
        break;
      case NO_AUDIO:
        setNone();
        break;
      default:
        logger.error("Error, unknown function ", item.functionIndex);
        break;
      }
    }
  }
  return handled;   // true=handled, false=controller uses default action
}   // end onTouch()

// ========== load/save config setting =========================
#define AUDIO_CONFIG_FILE    CONFIG_FOLDER "/announce.cfg"
#define AUDIO_CONFIG_VERSION "Audio Announce v01"

// ----- load from SDRAM -----
void ViewCfgAudioType::loadConfig() {
  SaveRestore config(AUDIO_CONFIG_FILE, AUDIO_CONFIG_VERSION);
  functionID tempAudioOutputType;
  int result = config.readConfig((byte *)&tempAudioOutputType, sizeof(tempAudioOutputType));
  if (result) {
    this->selectedAudio = tempAudioOutputType;
    logger.info("Loaded audio output type: ", this->selectedAudio);
  } else {
    logger.warning("Failed to load Audio Output Type, re-initializing config file");
    saveConfig();
  }
}
// ----- save to SDRAM -----
void ViewCfgAudioType::saveConfig() {
  SaveRestore config(AUDIO_CONFIG_FILE, AUDIO_CONFIG_VERSION);
  int rc = config.writeConfig((byte *)&selectedAudio, sizeof(selectedAudio));
}
