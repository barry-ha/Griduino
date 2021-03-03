#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     cfg_volume.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the user interface to increase/decrease/mute speaker volume.
            This "view" module is all about speaker volume.
            As such, the only thing it does is set the DS1804 digital potentiometer.
            It has nothing to do with the DAC, audio files, Morse code speed/pitch, 
            or with other playback attributes.

            +---------------------------------+
            | *          1.Speaker          > |
            |    33    Audio Volume           |... yRow1
            |    33    of 10                  |... yRow2
            |                                 |
            |      +---------------+          |
            |      |      Up       |          |
            |      |               | +--------+
            |      +---------------+ |  Mute  |
            |      |     Down      | +--------+
            |      |               |          |
            +------+---------------+----------+

*/

#include <Arduino.h>
#include "Adafruit_ILI9341.h"   // TFT color display library
#include <DS1804.h>             // DS1804 digital potentiometer library
#include "constants.h"          // Griduino constants and colors
#include "model_gps.h"          // Model of a GPS for model-view-controller
#include "morse_dac.h"          // morse code
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
extern DACMorseSender dacMorse;   // morse code (so we can send audio samples)
extern DS1804 volume;             // digital potentiometer

// ========== class ViewVolume =================================
class ViewVolume : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewVolume(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {}
  void updateScreen();
  void startScreen();
  void endScreen();
  bool onTouch(Point touch);
  void loadConfig();
  void saveConfig();

protected:
  // ----- 'globals' -----
  int gVolIndex = 5;       // init to middle value
  int gMute     = false;   // true=muted, false=UNmuted (not saved in NVR)

  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  // vertical placement of text rows
  const int yRow1 = 50;           // label: "Audio Volume"
  const int yRow2 = yRow1 + 30;   // text:  "of 10"

#define col1    10    // left-adjusted column of text
#define xButton 160   // indented column of buttons

  enum txtSettings {
    SETTINGS = 0,
    BIGVOLUME,
    LINE1,
    LINE2,
    MUTELABEL,
  };

#define numVolFields 5
  TextField txtVolume[numVolFields] = {
      //  text             x, y    color       alignment    size
      {"1. Speaker", col1, 20, cHIGHLIGHT, ALIGNCENTER, eFONTSMALLEST},   // [SETTINGS]
      {"0", 82, yRow2, cVALUE, ALIGNRIGHT, eFONTGIANT},                   // [BIGVOLUME] giant audio volume display
      {"Audio Volume", 98, yRow1, cLABEL, ALIGNLEFT, eFONTSMALL},         // [LINE1] normal size text labels
      {"of 10", 98, yRow2, cLABEL, ALIGNLEFT, eFONTSMALL},                // [LINE2]
      {"  Mute", 208, 156, cBUTTONLABEL, ALIGNLEFT, eFONTSMALL},          // [MUTELABEL]
  };

  // ----- constants -----

  enum functionID {
    UP_ID = 0,
    DOWN_ID,
    MUTE_ID,
  };
#define nVolButtons 3
  FunctionButton volButtons[nVolButtons] = {
      // label   origin     size       touch-target
      // text     x,y        w,h       x,y      w,h    radius  color         functionID
      {"", 38, 92, 136, 64, {38, 92, 136, 64}, 10, cBUTTONLABEL, UP_ID},        // Up
      {"", 38, 166, 136, 64, {38, 166, 136, 64}, 10, cBUTTONLABEL, DOWN_ID},    // Down
      {"", 200, 120, 98, 62, {198, 100, 122, 80}, 10, cBUTTONLABEL, MUTE_ID},   // Mute
  };

#define numLevels 11
  const int volLevel[numLevels] = {
      // Digital potentiometer settings, about 2 dB steps = ratio 1.585
      0,    // [0] mute, lowest allowed wiper position
      1,    // [1] lowest possible position with non-zero output
      2,    // [2] next lowest poss
      4,    // [3]  2.000 * 1.585 =  4.755
      7,    // [4]  4.755 * 1.585 =  7.513
      12,   // [5]  7.513 * 1.585 = 11.908
      19,   // [6] 11.908 * 1.585 = 18.874
      29,   // [7] 18.874 * 1.585 = 29.916
      47,   // [8] 29.916 * 1.585 = 47.417
      75,   // [9] 47.417 * 1.585 = 75.155
      99,   // [10] max allowed wiper position
  };

  // ---------- local functions for this derived class ----------
  void setVolume(int volIndex) {
    // set digital potentiometer
    // @param volIndex = 0..10
    int wiperPosition = volLevel[volIndex];
    volume.setWiperPosition(wiperPosition);

    char msg[256];
    snprintf(msg, 256, "Set volume index %d, wiper position %d", volIndex, wiperPosition);   // debug
    Serial.println(msg);
  }
  void changeVolume(int diff) {
    gVolIndex += diff;
    gVolIndex = constrain(gVolIndex, 0, numLevels - 1);
    setVolume(gVolIndex);
    this->updateScreen();   // update screen _before_ playing lengthy morse code
  }
  void volumeUp() {
    if (gMute) {
      unmuteVolume();
    } else {
      changeVolume(+1);   // increase volume
    }
  }
  void volumeDown() {
    if (gMute) {
      unmuteVolume();
    } else {
      changeVolume(-1);   // decrease volume
    }
  }
  void volumeMute() {   // mute
    // we keep track of "mute" status separately from "volume" level
    if (gMute) {
      // already muted, so UN mute
      unmuteVolume();
    } else {
      // mute volume
      gMute = true;
      txtVolume[BIGVOLUME].setColor(cDISABLED);
      txtVolume[BIGVOLUME].setBackground(cBACKGROUND);

      txtVolume[MUTELABEL].setBackground(cBUTTONFILL);
      txtVolume[MUTELABEL].print("Unmute");
      setVolume(0);
    }
  }
  void unmuteVolume() {
    gMute = false;
    txtVolume[BIGVOLUME].setColor(cVALUE);
    txtVolume[BIGVOLUME].setBackground(cBACKGROUND);

    txtVolume[MUTELABEL].setBackground(cBUTTONFILL);
    txtVolume[MUTELABEL].print("  Mute");
    setVolume(gVolIndex);
  }

};   // end class ViewSettings4

// ============== implement public interface ================
void ViewVolume::updateScreen() {
  // called on every pass through main()

  // ----- fill in replacment string text
  txtVolume[BIGVOLUME].setBackground(cBACKGROUND);
  txtVolume[BIGVOLUME].print(gVolIndex);

  txtVolume[MUTELABEL].setBackground(cBUTTONFILL);
  txtVolume[MUTELABEL].print();
}

void ViewVolume::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(cBACKGROUND);                     // clear screen
  txtVolume[BIGVOLUME].setBackground(cBACKGROUND);    // set background for all TextFields in this view
  TextField::setTextDirty(txtVolume, numVolFields);   // make sure all fields get re-printed on screen change

  drawAllIcons();              // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();   // optionally draw boxes around button-touch area
  showScreenBorder();          // optionally outline visible area
  showScreenCenterline();      // optionally draw alignment bar

  // ----- draw buttons
  for (int ii = 0; ii < nVolButtons; ii++) {
    FunctionButton item = volButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    if (strlen(item.text) > 0) {
      tft->setCursor(item.x + 20, item.y + 32);
      tft->setTextColor(cVALUE);
      tft->print(item.text);
    }

#ifdef SHOW_TOUCH_TARGETS
    Rect target = item.hitTarget;
    tft->drawRect(target.ul.x, target.ul.y,   // debug: draw outline around hit target
                  target.size.x, target.size.y,
                  cTOUCHTARGET);
#endif
  }

  // ----- draw text fields
  // draw text AFTER buttons because the MUTE text is on top of a button
  for (int ii = 0; ii < numVolFields; ii++) {
    int bkg = (ii == MUTELABEL) ? cBUTTONFILL : cBACKGROUND;   // TODO:
    txtVolume[ii].setBackground(bkg);
    txtVolume[ii].print();
  }

  // ----- icons on buttons
  int ht = 24;                                      // height of triangle
  int ww = 16;                                      // width of triangle
  int nn = 8;                                       // nudge toward center of button
  int xx = volButtons[0].x + volButtons[0].w / 2;   // centerline is halfway in the middle
  int yy = volButtons[0].y + volButtons[0].h / 2;   // baseline is halfway in the middle
  //                   x0,y0,        x1,y1,     x2,y2,   color
  tft->fillTriangle(xx - ww, yy + nn, xx + ww, yy + nn, xx, yy - ht + nn, cVALUE);   // arrow UP

  yy = volButtons[1].y + volButtons[1].h / 2;
  tft->fillTriangle(xx - ww, yy - nn, xx + ww, yy - nn, xx, yy + ht - nn, cVALUE);   // arrow DOWN

  updateScreen();   // update UI immediately, don't wait for laggy mainline loop
}   // end startScreen()

void ViewVolume::endScreen() {
  // Called once each time this view becomes INactive
  // This is a 'goodbye kiss' to do cleanup work
  // For the volume control, save our settings here instead of on each
  // button press because writing to NVR is slow (0.5 sec) and would delay the user
  // while trying to press a button many times in a row.
  saveConfig();
}

bool ViewVolume::onTouch(Point touch) {
  Serial.println("->->-> Touched volume screen.");
  bool handled = false;   // assume a touch target was not hit
  for (int ii = 0; ii < nVolButtons; ii++) {
    FunctionButton item = volButtons[ii];
    if (item.hitTarget.contains(touch)) {
      handled = true;               // hit!
      switch (item.functionIndex)   // do the thing
      {
      case UP_ID:
        volumeUp();
        break;
      case DOWN_ID:
        volumeDown();
        break;
      case MUTE_ID:
        volumeMute();
        break;
      default:
        Serial.print("Error, unknown function ");
        Serial.println(item.functionIndex);
        break;
      }
      updateScreen();   // update UI immediately, don't wait for laggy mainline loop
      if (!gMute) {
        dacMorse.setMessage("hi");   // announce new volume in Morse code
        dacMorse.sendBlocking();
      }
    }
  }
  return handled;   // true=handled, false=controller uses default action
}   // end onTouch()

// ========== load/save config setting =========================
#define VOLUME_CONFIG_FILE    CONFIG_FOLDER "/volume.cfg"
#define CONFIG_VOLUME_VERSION "Volume v02"

// ----- load from SDRAM -----
void ViewVolume::loadConfig() {
  SaveRestore config(VOLUME_CONFIG_FILE, CONFIG_VOLUME_VERSION);
  int tempVolIndex;
  int result = config.readConfig((byte *)&tempVolIndex, sizeof(tempVolIndex));
  if (result) {
    gVolIndex = constrain(tempVolIndex, 0, 10);   // global volume index
    setVolume(gVolIndex);                         // set the hardware to this volume index
    Serial.print("Loaded volume setting from NVR: ");
    Serial.println(gVolIndex);
  } else {
    Serial.println("Failed to load Volume control settings, re-initializing config file");
    saveConfig();
  }
}
// ----- save to SDRAM -----
void ViewVolume::saveConfig() {
  SaveRestore config(VOLUME_CONFIG_FILE, CONFIG_VOLUME_VERSION);
  config.writeConfig((byte *)&gVolIndex, sizeof(gVolIndex));
}
