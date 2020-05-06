/* File: view_volume.cpp


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
#include "Adafruit_GFX.h"         // Core graphics display library
#include "Adafruit_ILI9341.h"     // TFT color display library
#include "constants.h"            // Griduino constant declarations
#include "model.cpp"              // "Model" portion of model-view-controller
#include "morse_dac.h"            // morse code
#include "DS1804.h"               // DS1804 digital potentiometer library
#include "save_restore.h"         // save/restore configuration data to SDRAM

// ------------ typedef's
typedef struct {
  int x;
  int y;
} Point;

typedef struct {
  char text[26];
  int x;
  int y;
  uint16_t color;
} Label;

typedef void (*simpleFunction)();
typedef struct {
  char text[26];
  int x, y;
  int w, h;
  int radius;
  uint16_t color;
  simpleFunction function;
} Button;

// ========== global scope =====================================
int gVolIndex = 5;          // init to middle value
int gPrevVolIndex = -1;     // remembers previous volume setting to avoid erase/write the same value


// ========== extern ===========================================
extern Adafruit_ILI9341 tft;      // Griduino.ino
extern void printProportionalText(int xx, int yy, String text, uint16_t cc);
extern void erasePrintProportionalText(int xx, int yy, int ww, String text, uint16_t cc); // view_status.cpp
extern DACMorseSender dacMorse;   // morse code (so we can send audio samples)
extern DS1804 volume;             // digital potentiometer

//id showNameOfView(String sName, uint16_t fgd, uint16_t bkg);  // Griduino.ino
void initFontSizeSmall();         // Griduino.ino
void initFontSizeBig();           // Griduino.ino
int getOffsetToCenterText(String text); // Griduino.ino

// ========== forward reference ================================
void updateVolumeScreen();
void volUp();
void volDown();
void volMute();
int loadConfigVolume();
void saveConfigVolume();

// ========== constants ===============================
const int yRow1 = 32;             // label: "Audio Volume"
const int yRow2 = yRow1 + 30;     // text:  "4 of 10"
//nst int yRow3 = yRow2 + 20;
//nst int yRow4 = yRow3 + 44;
//nst int yRow5 = yRow4 + 44;
//nst int yRow6 = yRow5 + 44;

const int labelX = 8;       // indent labels, slight margin to left edge of screen
const int xBigNum = 56;     // indent values
const int yBigNum = yRow2;

const int numLabels = 2;
Label volLabels[numLabels] = {
  //   text          x,y      color  
  {"Audio Volume",  98,yRow1, cLABEL},
  {"of 10",         98,yRow2, cLABEL},
};
const int nVolButtons = 3;
const int margin = 10;      // slight margin between button border and edge of screen
const int radius = 10;      // rounded corners
Button volButtons[nVolButtons] = {
  // text     x,y        w,h        r      color
  {"",       38, 76,   136,72,  radius, cBUTTONLABEL, volUp  }, // Up
  {"",       38,156,   136,72,  radius, cBUTTONLABEL, volDown}, // Down
  {"Mute",  208,116,    90,68,  radius, cBUTTONLABEL, volMute}, // Mute
};
int volLevel[11] = {
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

// ========== helpers =================================
void setVolume(int volIndex) {
  // set digital potentiometer
  // @param wiperPosition = 0..10
  int wiperPosition = volLevel[ volIndex ];
  volume.setWiperPosition( wiperPosition );
  Serial.print("Set wiper position "); Serial.println(wiperPosition);
  saveConfigVolume();     // non-volatile storage
}
void changeVolume(int diff) {
  gVolIndex += diff;
  gVolIndex = constrain(gVolIndex, 0, 10);
  setVolume(gVolIndex);
  updateVolumeScreen();
  dacMorse.setMessage("hi");
  dacMorse.sendBlocking();
}
void volUp() {
  changeVolume( +1 );
}
void volDown() {
  changeVolume( -1 );
}
void volMute() {
  gVolIndex = 0;
  setVolume(gVolIndex);
  updateVolumeScreen();
}

// ========== load/save config setting =========================
#define VOLUME_CONFIG_FILE    CONFIG_FOLDER "/volume.cfg"
#define CONFIG_VOLUME_VERSION "Volume v01"

// ----- load from SDRAM -----
int loadConfigVolume() {
  SaveRestore config(VOLUME_CONFIG_FILE, CONFIG_VOLUME_VERSION);
  int result = config.readConfig();
  if (result) {
    gVolIndex = constrain( config.intSetting, 0, 10);
    setVolume( gVolIndex );
    Serial.print(". Loaded volume setting: "); Serial.println(gVolIndex);
  }
  return result;
}
// ----- save to SDRAM -----
void saveConfigVolume() {
  SaveRestore config(VOLUME_CONFIG_FILE, CONFIG_VOLUME_VERSION, gVolIndex);
  config.writeConfig();
}

// ========== volume screen view ===============================
void startVolumeScreen() {
  tft.fillScreen(cBACKGROUND);
  initFontSizeSmall();

  // ----- draw buttons
  for (int ii=0; ii<nVolButtons; ii++) {
    Button item = volButtons[ii];
    tft.fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft.drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    tft.setCursor(item.x+20, item.y+32);
    tft.setTextColor(cVALUE);
    tft.print(item.text);
  }

  // ----- other labels around buttons
  for (int ii=0; ii<numLabels; ii++) {
    Label item = volLabels[ii];
    tft.setCursor(item.x, item.y);
    tft.setTextColor(item.color);
    tft.print(item.text);
  }

  // ----- icons on buttons
  int ht = 24;                                  // height of triangle
  int ww = 16;                                  // width of triangle
  int nn = 8;                                   // nudge toward center of button
  int xx = volButtons[0].x + volButtons[0].w/2; // centerline is halfway in the middle
  int yy = volButtons[0].y + volButtons[0].h/2; // baseline is halfway in the middle
  //                  x0,y0,     x1,y1,     x2,y2,   color
  tft.fillTriangle(xx-ww,yy+nn,  xx+ww,yy+nn,  xx,yy-ht+nn, cVALUE);  // arrow UP

  yy = volButtons[1].y + volButtons[1].h/2;
  tft.fillTriangle(xx-ww,yy-nn,  xx+ww,yy-nn,  xx,yy+ht-nn, cVALUE);  // arrow DOWN

  gPrevVolIndex = -1;
  updateVolumeScreen();             // fill in values immediately, don't wait for loop() to eventually get around to it

  // ----- label this view in upper left corner
  //showNameOfView("Volume:", cWARN, cBACKGROUND);  // todo - unused, delete this everywhere
}

void updateVolumeScreen() {
  // ----- volume
  if (gVolIndex != gPrevVolIndex) {
    initFontSizeBig();
    //                               x,y         w  String             color
    erasePrintProportionalText(xBigNum,yBigNum, 36, String(gVolIndex), cVALUE);

    gPrevVolIndex = gVolIndex;
  }
}

bool onTouchVolume(Point touch) {
  bool handled = false;             // assume a touch target was not hit
  for (int ii=0; ii<nVolButtons; ii++) {
    Button item = volButtons[ii];
    if (touch.x >= item.x && touch.x <= item.x+item.w
     && touch.y >= item.y && touch.y <= item.y+item.h) {
        handled = true;             // hit!
        item.function();            // do the thing

        //tft.setCursor(touch.x, touch.y);  // debug: show where hit
        //initFontSizeSmall();
        //tft.print("x");
     }
  }
  return handled;         // true=handled, false=controller uses default action
}
