// Please format this file with clang before check-in to GitHub
/*
  File:     cfg_volume2.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the user interface to increase/decrease/mute speaker volume.
            This "view" module is all about speaker volume.
            As such, the only thing it does is set the DS1804 digital potentiometer.
            It has nothing to do with the DAC, audio files, Morse code speed/pitch, or other playback attributes.

            +---------------------------------+
            | *        Settings 1           > |
            |         Audio Volume            |
            |                                 |
            |  +---+---+---+---+---+---+---+  |
            |  |###|###|###|   |   |   |   |  |
            |  +---+---+---+---+---+---+---+  |
            |  +------+             +------+  |
            |  | Down |             |  Up  |  |
            |  +------+             +------+  |
            |                                 |
            |            +------+             |
            |            | Mute |             |
            |            +------+             |
            +------+---------------+----------+

*/

#include <Arduino.h>
#include "Adafruit_ILI9341.h"   // TFT color display library
#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "morse_dac.h"          // morse code
#include "DS1804.h"             // DS1804 digital potentiometer library
#include "save_restore.h"       // save/restore configuration data to SDRAM
#include "TextField.h"          // Optimize TFT display text for proportional fonts

// ========== extern ===========================================
extern Logger logger;             // Griduino.ino
extern Adafruit_ILI9341 tft;      // Griduino.ino
extern DACMorseSender dacMorse;   // morse code (so we can send audio samples)
extern DS1804 volume;             // digital potentiometer

extern void setFontSize(int font);   // TextField.cpp

// ========== forward reference ================================
void updateVolume2Screen();
void volUp2();
void volDown2();
void volMute2();
// int loadConfigVolume();
// void saveConfigVolume();

// ============== constants ====================================
// vertical placement of text rows
const int yRow1 = 32;           // label: "Audio Volume"
const int yRow2 = yRow1 + 30;   // text:  "of 10"

// color scheme: see constants.h

// ========== globals ==========================================
int gVolIndex2     = 5;    // init to middle value
int gPrevVolIndex2 = -1;   // remembers previous volume setting to avoid erase/write the same value

TextField txtVolume2[] = {
    //        text             x,y      color
    TextField("Audio Volume", 98, yRow1, cLABEL),   // normal size text labels
};
const int numVolFields = sizeof(txtVolume2) / sizeof(TextField);

const int margin     = 10;   // slight margin between button border and edge of screen
const int radius     = 10;   // rounded corners
Button volButtons2[] = {
    // text     x,y        w,h        r      color
    {"", 38, 76, 136, 72, radius, cBUTTONLABEL, volUp2},          // Up
    {"", 38, 156, 136, 72, radius, cBUTTONLABEL, volDown2},       // Down
    {"Mute", 208, 116, 90, 68, radius, cBUTTONLABEL, volMute2},   // Mute
};
const int nVolButtons = sizeof(volButtons2) / sizeof(Button);
int volLevel2[11]     = {
    // Digital potentiometer settings, about 2 dB steps = ratio 1.585
    /* 0 */ 0,    // mute, lowest allowed wiper position
    /* 1 */ 1,    // lowest possible position with non-zero output
    /* 2 */ 2,    // next lowest poss
    /* 3 */ 4,    //  2.000 * 1.585 =  4.755
    /* 4 */ 7,    //  4.755 * 1.585 =  7.513
    /* 5 */ 12,   //  7.513 * 1.585 = 11.908
    /* 6 */ 19,   // 11.908 * 1.585 = 18.874
    /* 7 */ 29,   // 18.874 * 1.585 = 29.916
    /* 8 */ 47,   // 29.916 * 1.585 = 47.417
    /* 9 */ 75,   // 47.417 * 1.585 = 75.155
    /*10 */ 99,   // max allowed wiper position
};

// ========== helpers ==========================================
void setVolume2(int volIndex) {
  // set digital potentiometer
  // @param wiperPosition = 0..10
  int wiperPosition = volLevel2[volIndex];
  volume.setWiperPosition(wiperPosition);
  logger.log(CONFIG, INFO, "Set wiper position %d", wiperPosition);
  // saveConfigVolume();     // non-volatile storage
}
void changeVolume2(int diff) {
  gVolIndex2 += diff;
  gVolIndex2 = constrain(gVolIndex2, 0, 10);
  setVolume2(gVolIndex2);
  updateVolume2Screen();
  dacMorse.setMessage("hi");
  dacMorse.sendBlocking();
}
void volUp2() {
  changeVolume2(+1);
}
void volDown2() {
  changeVolume2(-1);
}
void volMute2() {
  gVolIndex2 = 0;
  setVolume2(gVolIndex2);
  updateVolume2Screen();
}

// ========== load/save config setting =========================
#define VOLUME_CONFIG_FILE    CONFIG_FOLDER "/volume.cfg"
#define CONFIG_VOLUME_VERSION "Volume v02"

// ----- load from SDRAM -----
/*
int loadConfigVolume() {
  SaveRestore config(VOLUME_CONFIG_FILE, CONFIG_VOLUME_VERSION);
  int tempVolIndex;
  int result = config.readConfig( (byte*) &tempVolIndex, sizeof(tempVolIndex) );
  if (result) {
    gVolIndex2 = constrain( tempVolIndex, 0, 10);  // global volume index
    setVolume2( gVolIndex2 );                       // set the hardware to this volume index
    logger.log(CONFIG, INFO, ". Loaded volume setting: %d", gVolIndex2);
  }
  return result;
}
*/
// ----- save to SDRAM -----
/*
void saveConfigVolume() {
  SaveRestore config(VOLUME_CONFIG_FILE, CONFIG_VOLUME_VERSION);
  config.writeConfig( (byte*) &gVolIndex2, sizeof(gVolIndex2) );
}
*/

// ========== volume screen view ===============================
void updateVolume2Screen() {
  // called on every pass through main()

  // ----- volume
  setFontSize(eFONTBIG);
  txtVolume2[0].print(gVolIndex2);
}
void startVolume2Screen() {
  // called once each time this view becomes active
  tft.fillScreen(cBACKGROUND);                         // clear screen
  txtVolume2[0].setBackground(cBACKGROUND);            // set background for all TextFields in this view
  TextField::setTextDirty(txtVolume2, numVolFields);   // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALL);

  // drawAllIcons();                   // todo: update "drawAllIcons" draw gear (settings) and arrow (next screen)
  // showScreenBorder();               // todo: optionally outline visible area

  // ----- draw text fields
  for (int ii = 1; ii < numVolFields; ii++) {   // start at [1], since [0] is a different font size
    txtVolume2[ii].print();
  }

  // ----- draw buttons
  for (int ii = 0; ii < nVolButtons; ii++) {
    Button item = volButtons2[ii];
    tft.fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft.drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    tft.setCursor(item.x + 20, item.y + 32);
    tft.setTextColor(cVALUE);
    tft.print(item.text);
  }

  // ----- icons on buttons
  int ht = 24;                                        // height of triangle
  int ww = 16;                                        // width of triangle
  int nn = 8;                                         // nudge toward center of button
  int xx = volButtons2[0].x + volButtons2[0].w / 2;   // centerline is halfway in the middle
  int yy = volButtons2[0].y + volButtons2[0].h / 2;   // baseline is halfway in the middle
  //                  x0,y0,     x1,y1,     x2,y2,   color
  tft.fillTriangle(xx - ww, yy + nn, xx + ww, yy + nn, xx, yy - ht + nn, cVALUE);   // arrow UP

  yy = volButtons2[1].y + volButtons2[1].h / 2;
  tft.fillTriangle(xx - ww, yy - nn, xx + ww, yy - nn, xx, yy + ht - nn, cVALUE);   // arrow DOWN

  gPrevVolIndex2 = -1;
  updateVolume2Screen();   // fill in values immediately, don't wait for loop() to eventually get around to it
}
bool onTouchVolume2(Point touch) {
  logger.log(CONFIG, INFO, "->->-> Touched volume2 screen.");
  bool handled = false;   // assume a touch target was not hit
  for (int ii = 0; ii < nVolButtons; ii++) {
    Button item = volButtons2[ii];
    if (touch.x >= item.x && touch.x <= item.x + item.w && touch.y >= item.y && touch.y <= item.y + item.h) {
      handled = true;    // hit!
      item.function();   // do the thing
    }
  }
  return handled;   // true=handled, false=controller uses default action
}
