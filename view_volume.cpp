/*
  File:     view_volume.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the user interface to increase/decrease/mute speaker volume.
            This "view" module is all about speaker volume.
            As such, the only thing it does is set the DS1804 digital potentiometer.
            It has nothing to do with the DAC, audio files, Morse code speed/pitch, or other playback attributes.

            +---------------------------------+
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
#include "Adafruit_ILI9341.h"       // TFT color display library
#include "constants.h"              // Griduino constants and colors
#include "model.cpp"                // "Model" portion of model-view-controller
#include "morse_dac.h"              // morse code
#include "DS1804.h"                 // DS1804 digital potentiometer library
#include "save_restore.h"           // save/restore configuration data to SDRAM
#include "TextField.h"              // Optimize TFT display text for proportional fonts
#include "view.h"                   // Base class for all views

// ========== extern ===========================================
void showNameOfView(String sName, uint16_t fgd, uint16_t bkg);  // Griduino.ino
extern DACMorseSender dacMorse;     // morse code (so we can send audio samples)
extern DS1804 volume;               // digital potentiometer

void setFontSize(int font);         // Griduino.ino
int getOffsetToCenterTextOnButton(String text, int leftEdge, int width ); // Griduino.ino
void drawAllIcons();                // draw gear (settings) and arrow (next screen) // Griduino.ino
void showScreenBorder();            // optionally outline visible area

// ========== forward reference ================================
void volumeUp();
void volumeDown();
void volumeMute();

// ============== constants ====================================
// color scheme: see constants.h

// vertical placement of text rows
const int yRow1 = 50;             // label: "Audio Volume"
const int yRow2 = yRow1 + 30;     // text:  "of 10"

#define col1 10                   // left-adjusted column of text
#define xButton 160               // indented column of buttons

// ========== globals ==========================================
int gVolIndex = 5;          // init to middle value
int gPrevVolIndex = -1;     // remembers previous volume setting to avoid erase/write the same value
enum txtSettings {
  SETTINGS=0, 
  BIGVOLUME,
  LINE1,
  LINE2
};
TextField txtVolume[] = {
  //        text                  x, y   color
  TextField("Settings 1",      col1, 20, cHIGHLIGHT, ALIGNCENTER),// [SETTINGS]
  TextField("0",               82,yRow2, cVALUE, ALIGNRIGHT),     // [BIGVOLUME] giant audio volume display
  TextField("Audio Volume",    98,yRow1, cLABEL),                 // [LINE1] normal size text labels
  TextField("of 10",           98,yRow2, cLABEL),                 // [LINE2]
};
const int numVolFields = sizeof(txtVolume)/sizeof(txtVolume[0]);

// ============== constants ====================================
const int nVolButtons = 3;
const int margin = 10;      // slight margin between button border and edge of screen
const int radius = 10;      // rounded corners
Button volButtons[nVolButtons] = {
  // text     x,y        w,h      r      color        function
  {"",       38, 92,   136,64,  radius, cBUTTONLABEL, volumeUp  }, // Up
  {"",       38,166,   136,64,  radius, cBUTTONLABEL, volumeDown}, // Down
  {"Mute",  208,120,    90,62,  radius, cBUTTONLABEL, volumeMute}, // Mute
};
const int volLevel[11] = {
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
const int numLevels = sizeof(volLevel)/sizeof(volLevel[0]);

// ========== helpers ==========================================
void setVolume(int volIndex) {
  // set digital potentiometer
  // @param volIndex = 0..10
  int wiperPosition = volLevel[ volIndex ];
  volume.setWiperPosition( wiperPosition );

  char msg[256];
  snprintf(msg, 256, "Set volume index %d, wiper position %d", volIndex, wiperPosition);  // debug
  Serial.println(msg);
}
void changeVolume(int diff) {
  gVolIndex += diff;
  gVolIndex = constrain(gVolIndex, 0, 10);
  setVolume(gVolIndex);
  dacMorse.setMessage("hi");
  dacMorse.sendBlocking();
}
void volumeUp() {
  changeVolume( +1 );
}
void volumeDown() {
  changeVolume( -1 );
}
void volumeMute() {
  gVolIndex = 0;
  setVolume(gVolIndex);
}

// ========== class ViewVolume =================================
void ViewVolume::updateScreen() {
  // called on every pass through main()

  // ----- fill in replacment string text
  setFontSize(eFONTBIG);
  txtVolume[BIGVOLUME].print(gVolIndex);
}

void ViewVolume::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(cBACKGROUND);     // clear screen
  txtVolume[BIGVOLUME].setBackground(cBACKGROUND);        // set background for all TextFields in this view
  TextField::setTextDirty( txtVolume, numVolFields );     // make sure all fields get re-printed on screen change

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showScreenBorder();                 // optionally outline visible area

  // ----- draw text fields
  setFontSize(eFONTSMALLEST);         // [0] is screen title
  txtVolume[0].print();

  // txtVolume[1] is giant audio number, is drawn later

  setFontSize(eFONTSMALL);
  for (int ii=2; ii<numVolFields; ii++) {   // start at [2], since [0] and [1] are different fonts
    txtVolume[ii].print();
  }

  // ----- draw buttons
  for (int ii=0; ii<nVolButtons; ii++) {
    Button item = volButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    tft->setCursor(item.x+20, item.y+32);
    tft->setTextColor(cVALUE);
    tft->print(item.text);

    #ifdef SHOW_TOUCH_TARGETS
    tft->drawRect(item.x, item.y,  // debug: draw outline around hit target
                 item.w, item.h, 
                 cWARN);
    #endif
  }

  // ----- icons on buttons
  int ht = 24;                                  // height of triangle
  int ww = 16;                                  // width of triangle
  int nn = 8;                                   // nudge toward center of button
  int xx = volButtons[0].x + volButtons[0].w/2; // centerline is halfway in the middle
  int yy = volButtons[0].y + volButtons[0].h/2; // baseline is halfway in the middle
  //                  x0,y0,     x1,y1,     x2,y2,   color
  tft->fillTriangle(xx-ww,yy+nn,  xx+ww,yy+nn,  xx,yy-ht+nn, cVALUE);  // arrow UP

  yy = volButtons[1].y + volButtons[1].h/2;
  tft->fillTriangle(xx-ww,yy-nn,  xx+ww,yy-nn,  xx,yy+ht-nn, cVALUE);  // arrow DOWN

  gPrevVolIndex = -1;
  updateScreen();                     // fill in values immediately, don't wait for the main loop to eventually get around to it

  #ifdef SHOW_SCREEN_CENTERLINE
    // show centerline at      x1,y1              x2,y2             color
    tft->drawLine( tft->width()/2,0,  tft->width()/2,tft->height(), cWARN); // debug
  #endif
}

bool ViewVolume::onTouch(Point touch) {
  Serial.println("->->-> Touched volume screen.");
  bool handled = false;             // assume a touch target was not hit
  for (int ii=0; ii<nVolButtons; ii++) {
    Button item = volButtons[ii];
    if (touch.x >= item.x && touch.x <= item.x+item.w
     && touch.y >= item.y && touch.y <= item.y+item.h) {
        handled = true;             // hit!
        item.function();            // do the thing
        this->saveConfig();         // save the new volume in non-volatile RAM
        updateScreen();             // show the result

     }
  }
  return handled;                   // true=handled, false=controller uses default action
}

// ========== load/save config setting =========================
#define VOLUME_CONFIG_FILE    CONFIG_FOLDER "/volume.cfg"
#define CONFIG_VOLUME_VERSION "Volume v01"

// ----- load from SDRAM -----
void ViewVolume::loadConfig() {
  SaveRestore config(VOLUME_CONFIG_FILE, CONFIG_VOLUME_VERSION);
  int tempVolIndex;
  int result = config.readConfig( (byte*) &tempVolIndex, sizeof(tempVolIndex) );
  if (result) {
    gVolIndex = constrain( tempVolIndex, 0, 10);  // global volume index
    setVolume( gVolIndex );                       // set the hardware to this volume index
    Serial.print("Loaded volume setting from NVR: "); Serial.println(gVolIndex);
  } else {
    Serial.println("Failed to load Volume control settings, re-initializing config file");
    saveConfig();
  }
}
// ----- save to SDRAM -----
void ViewVolume::saveConfig() {
  SaveRestore config(VOLUME_CONFIG_FILE, CONFIG_VOLUME_VERSION);
  config.writeConfig( (byte*) &gVolIndex, sizeof(gVolIndex) );
}
