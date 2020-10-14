/*
  File: view_settings4.h 

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is the 'control panel' for screen rotation.
            Since it's NOT intended for a driver in motion, we use a
            smaller font to cram more stuff onto the screen.

            +-----------------------------------+
            |              Settings 4  /|\      |
            | Screen                    |       |
            | Orientation   (o)[ This edge up ] |
            |                                   |
            |               ( )[ That edge up ] |
            |                           |       |
            |                           |       |
            |                           |       |
            | v0.26, Oct 12 2020       \|/      |
            +-----------------------------------+
*/

#include <Arduino.h>
#include "Adafruit_ILI9341.h"         // TFT color display library
#include "constants.h"                // Griduino constants and colors
#include "model.cpp"                  // "Model" portion of model-view-controller
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "view.h"                     // Base class for all views

// ========== extern ===========================================
void showNameOfView(String sName, uint16_t fgd, uint16_t bkg);  // Griduino.ino
extern Model* model;                  // "model" portion of model-view-controller

void setFontSize(int font);           // Griduino.ino
int getOffsetToCenterTextOnButton(String text, int leftEdge, int width ); // Griduino.ino
void drawAllIcons();                  // draw gear (settings) and arrow (next screen) // Griduino.ino
void showScreenBorder();              // optionally outline visible area


// ========== class ViewSettings4 ==============================
class ViewSettings4 : public View {
  public:
    // ---------- public interface ----------
    // This derived class must implement the public interface:
    ViewSettings4(Adafruit_ILI9341* vtft, int vid)  // ctor 
      : View{ vtft, vid }
    { }
    void updateScreen();
    void startScreen();
    bool onTouch(Point touch);
    void loadConfig();
    void saveConfig();

  protected:
    // ---------- local data for this derived class ----------
    // color scheme: see constants.h

    // vertical placement of text rows   ---label---           ---button---
    const int yRow1 = 70;             // "Screen Orientation", "This edge up"
    const int yRow2 = yRow1 + 70;     //                       "That edge up"
    const int yRow9 = gScreenHeight - 8; // "v0.25, Oct 12 2020"

    #define col1 10                   // left-adjusted column of text
    #define xButton 160               // indented column of buttons

    enum txtSettings4 {
      SETTINGS=0, 
      SCREEN,
      ORIENTATION,
      COMPILED,
    };
    #define nFields 4
    TextField txtSettings4[nFields] = {
      //        text                x, y        color
      TextField("Settings 4",    col1, 20,      cHIGHLIGHT, ALIGNCENTER),// [SETTINGS]
      TextField("Screen",        col1,yRow1,    cVALUE),                 // [SCREEN]
      TextField("Orientation",   col1,yRow1+20, cVALUE),                 // [ORIENTATION]
      TextField(PROGRAM_VERSION ", " __DATE__, 
                                 col1,yRow9,    cLABEL, ALIGNLEFT),      // [COMPILED]
    };

    enum txtLines4 {
      THIS_BUTTON=0,
      THAT_BUTTON,
    };
    #define nButtons 2
    FunctionButton myButtons[nButtons] = {
      // label             origin         size      touch-target     
      // text                x,y           w,h      x,y      w,h  radius  color   function
      {"This edge up", xButton,yRow1-26, 140,40, {120, 35, 190,65},  4,  cVALUE,  THIS_BUTTON },
      {"That edge up", xButton,yRow2-26, 140,40, {120,100, 190,75},  4,  cVALUE,  THAT_BUTTON },
    };
    /* */

    #define xLine (xButton+80)
    #define topY 8
    #define botY (gScreenHeight-6)

    #define nLines 6
    TwoPoints myLines[nLines] = {
      //   x1,y1           x2,y2
      {xLine,yRow1-26,  xLine,  topY,    cBUTTONOUTLINE},    // from top outline of top button, to top edge of screen
      {xLine,5,         xLine-4,topY+15, cBUTTONOUTLINE},    // left half of arrowhead
      {xLine,5,         xLine+4,topY+15, cBUTTONOUTLINE},    // right half of arrowhead

      {xLine,yRow2+14,  xLine,botY,      cBUTTONOUTLINE},    // from bottom outline of bottom button, to bottom edge of screen
      {xLine,botY,      xLine-4,botY-16, cBUTTONOUTLINE},    // left half of arrowhead
      {xLine,botY,      xLine+4,botY-16, cBUTTONOUTLINE},    // right half of arrowhead
    };

    // ---------- local functions for this derived class ----------
    void fRotateScreen() {
      Serial.println("->->-> Clicked OTHER EDGE UP button.");
      if (this->screenRotation == eSCREEN_ROTATE_0) {
        this->setScreenRotation(eSCREEN_ROTATE_180);
      } else {
        this->setScreenRotation(eSCREEN_ROTATE_0);
      }
    }

};  // end class ViewSettings4

// ============== implement public interface ================
void ViewSettings4::updateScreen() {
  // called on every pass through main()

  // ----- show selected radio buttons by filling in the circle
  for (int ii=0; ii<nButtons; ii++) {
    FunctionButton item = myButtons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);
    int buttonFillColor = cBACKGROUND;

    if (item.functionIndex == THIS_BUTTON) {
      // it may seem strange, but we ALWAYS highlight the first button as active 
      // because the first button is always the "up" edge 
      buttonFillColor = cLABEL;
    } 
    tft->fillCircle(xCenter, yCenter, 4, buttonFillColor);
  }
}


void ViewSettings4::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(cBACKGROUND);     // clear screen
  txtSettings4[0].setBackground(cBACKGROUND);        // set background for all TextFields in this view
  TextField::setTextDirty( txtSettings4, nFields );  // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALLEST);

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showScreenBorder();                 // optionally outline visible area

  // ----- draw text fields
  for (int ii=0; ii<nFields; ii++) {
      txtSettings4[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii=0; ii<nButtons; ii++) {
    FunctionButton item = myButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y+item.h/2+5);
    tft->setTextColor(item.color);
    tft->print(item.text);

    #ifdef SHOW_TOUCH_TARGETS
    tft->drawRect(item.hitTarget.ul.x, item.hitTarget.ul.y,  // debug: draw outline around hit target
                 item.hitTarget.size.x, item.hitTarget.size.y, 
                 cWARN);
    #endif
  }

  // ----- draw arrows from buttons to top/bottom edges
  for (int ii=0; ii<nLines; ii++) {
    TwoPoints m = myLines[ii];
    tft->drawLine(m.x1, m.y1, m.x2, m.y2, m.color);
  }

  // ----- show outlines of radio buttons
  for (int ii=0; ii<nButtons; ii++) {
    FunctionButton item = myButtons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);

    // outline the radio button
    // the active button will be indicated in updateScreen()
    tft->drawCircle(xCenter, yCenter, 7, cVALUE);
  }

  updateScreen();                     // fill in values immediately, don't wait for the main loop to eventually get around to it

  #ifdef SHOW_SCREEN_CENTERLINE
    // show centerline at      x1,y1              x2,y2             color
    tft->drawLine( tft->width()/2,0,  tft->width()/2,tft->height(), cWARN); // debug
  #endif
}


bool ViewSettings4::onTouch(Point touch) {
  Serial.println("->->-> Touched settings screen.");
  bool handled = false;               // assume a touch target was not hit
  for (int ii=0; ii<nButtons; ii++) {
    FunctionButton item = myButtons[ii];
    if (item.hitTarget.contains(touch)) {
        handled = true;               // hit!
        switch (item.functionIndex)   // do the thing
        {
          case THIS_BUTTON:
              // nothing - this button is already at top of screen
              break;
          case THAT_BUTTON:
              // the guy wants the OTHER edge of the screen (whatever it is) to be "up"
              fRotateScreen();
              break;
          default:
              Serial.print("Error, unknown function ");Serial.println(item.functionIndex);
              break;
        }
        updateScreen();               // update UI immediately, don't wait for laggy mainline loop
        this->saveConfig();           // after UI is updated, save setting to nvr
     }
  }
  return handled;                     // true=handled, false=controller uses default action
}

// ========== load/save config setting =========================
#define SCREEN_CONFIG_FILE    CONFIG_FOLDER "/screen.cfg"
#define CONFIG_SCREEN_VERSION "Screen Orientation v02"

// ----- load from SDRAM -----
void ViewSettings4::loadConfig() {
  // Load screen orientation from NVR, and do minimal amount of work
  // Since this is called first thing during setup, we can't use 
  // resource-heavy functions like "this->setScreenRotation(int rot)"

  SaveRestore config(SCREEN_CONFIG_FILE, CONFIG_SCREEN_VERSION);
  int tempRotation;
  int result = config.readConfig( (byte*) &tempRotation, sizeof(tempRotation) );
  if (result) {
    this->screenRotation = tempRotation;
    tft->setRotation(tempRotation);    // 0=portrait (default), 1=landscape, 3=180 degrees
    Serial.print("Loaded screen orientation: "); Serial.println(this->screenRotation);
  } else {
    Serial.println("Failed to load screen orientation, re-initializing config file");
    saveConfig();
  }
}
// ----- save to SDRAM -----
void ViewSettings4::saveConfig() {
  SaveRestore config(SCREEN_CONFIG_FILE, CONFIG_SCREEN_VERSION);
  int rc = config.writeConfig( (byte*) &screenRotation, sizeof(screenRotation) );
  //Serial.print("Finished with rc = "); Serial.println(rc);  // debug
}
