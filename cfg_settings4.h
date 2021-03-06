/*
  File:     cfg_settings4.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is one of the 'control panel' screens for Griduino setup.
            Since it's not intended for a driver in motion, we can use 
            a smaller font and cram more stuff onto the screen.

            +-----------------------------------------+
            |            4. Announcements             |
            |                                         |
            | Announce at       (o)[ 4-digit      ]   |
            | grid crossing     ( )[ 6-digit      ]   |
            |                                         |
            |                                         |
            |                                         |
            |                                         |
            | v0.32, Feb 2 2021  08:16                |
            +-----------------------------------------+
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>         // TFT color display library
#include "constants.h"                // Griduino constants and colors
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "view.h"                     // Base class for all views

// ========== extern ===========================================
extern Model* model;                  // "model" portion of model-view-controller

extern void showDefaultTouchTargets();// Griduino.ino
extern void sendMorseGrid4(String gridName);  // Griduino.ino
extern void sendMorseGrid6(String gridName);  // Griduino.ino

// ========== class ViewSettings4 ==============================
class ViewSettings4 : public View {
  public:
    // ---------- public interface ----------
    // This derived class must implement the public interface:
    ViewSettings4(Adafruit_ILI9341* vtft, int vid)  // ctor 
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

    // vertical placement of text rows   ---label---         ---button---
    const int yRow1 = 70;             // "Announce at",      "4-Digit"
    const int yRow2 = yRow1 + 24;     // "grid crossing"
    const int yRow3 = yRow1 + 68;     //                     "6-Digit"
    const int yRow9 = gScreenHeight - 12; // "v0.27, Nov 03 2020"

    #define col1 10                   // left-adjusted column of text
    #define xButton 160               // indented column of buttons

    enum txtSettings4 {
      SETTINGS=0, 
      ANNOUNCE1,
      ANNOUNCE2,
      DISTANCE4,
      DISTANCE6,
      COMPILED,
    };
    #define nTextCrossing 6
    TextField txtSettings4[nTextCrossing] = {
      //        text                  x, y        color
      TextField("4. Announcements",col1, 20,      cHIGHLIGHT, ALIGNCENTER),// [SETTINGS]
      TextField("Announce at",     col1,yRow1,    cVALUE),                 // [ANNOUNCE1]
      TextField("grid crossing",   col1,yRow2,    cVALUE),                 // [ANNOUNCE2]
      TextField("70 - 100 mi",  xButton+24,yRow1+22,cVALUE),               // [DISTANCE4]
      TextField("3 - 4 mi",     xButton+38,yRow3+22,cVALUE),               // [DISTANCE6]
      TextField(PROGRAM_VERSION ", " PROGRAM_COMPILED, 
                                   col1,yRow9,    cLABEL, ALIGNCENTER),    // [COMPILED]
    };

    // ----- helpers -----
    void f4Digit() {
      Serial.println("->->-> Clicked 4-DIGIT button.");
      model->compare4digits = true;
      this->updateScreen();             // update UI before starting the slow morse code

      // announce grid square, so they have an audible example of this selection
      char newGrid4[7];
      calcLocator(newGrid4, model->gLatitude, model->gLongitude, 4);
      sendMorseGrid4( newGrid4 );       // announce 4-digit grid by Morse code
    }
    void f6Digit() {
      Serial.println("->->-> Clicked 6-DIGIT button.");
      model->compare4digits = false;
      this->updateScreen();             // update UI before starting the slow morse code

      // announce grid square, so they have an audible example of this selection
      char newGrid6[7];
      calcLocator(newGrid6, model->gLatitude, model->gLongitude, 6);
      sendMorseGrid6( newGrid6 );       // announce 4-digit grid by Morse code
    }

    enum buttonID {
      e4DIGIT = 0,
      e6DIGIT,
    };
    #define nButtonsDigits 2
    FunctionButton myButtons[nButtonsDigits] = {
      // label           visible rectangle      touch-target          text
      // text            x,y           w,h      x,y      w,h  radius  color   function
      {"4-Digit",  xButton,yRow1-26, 140,60, {130, 40, 180,70},  4,  cVALUE,  e4DIGIT },   // [e4DIGIT] set units English
      {"6-Digit",  xButton,yRow3-26, 140,60, {130,110, 180,70},  4,  cVALUE,  e6DIGIT },   // [e6DIGIT] set units Metric
    };

};  // end class ViewSettings4

// ============== implement public interface ================
void ViewSettings4::updateScreen() {
  // called on every pass through main()

  // ----- show selected radio buttons by filling in the circle
  for (int ii=e4DIGIT; ii<=e6DIGIT; ii++) {
    FunctionButton item = myButtons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);
    int buttonFillColor = cBACKGROUND;

    if (ii==e4DIGIT && model->compare4digits) {
      buttonFillColor = cLABEL;
    } 
    if (ii==e6DIGIT && !model->compare4digits) {
      buttonFillColor = cLABEL;
    }
    tft->fillCircle(xCenter, yCenter, 4, buttonFillColor);
  }
}


void ViewSettings4::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                  // clear screen
  txtSettings4[0].setBackground(this->background);      // set background for all TextFields in this view
  txtSettings4[DISTANCE4].setBackground(cBUTTONFILL);   // change background for text on top of buttons
  txtSettings4[DISTANCE6].setBackground(cBUTTONFILL);   // change background for text on top of buttons
  TextField::setTextDirty( txtSettings4, nTextCrossing );  // make sure all fields get re-printed on screen change

  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();          // optionally draw boxes around button-touch area
  showScreenBorder();                 // optionally outline visible area

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii=0; ii<nButtonsDigits; ii++) {
    FunctionButton item = myButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y+item.h/3+5);  // place text on top third of button (not centered vertically) 
                                            // to make room for a second row of text on the button
    tft->setTextColor(item.color);
    tft->print(item.text);

    #ifdef SHOW_TOUCH_TARGETS
    tft->drawRect(item.hitTarget.ul.x, item.hitTarget.ul.y,  // debug: draw outline around hit target
                  item.hitTarget.size.x, item.hitTarget.size.y, 
                  cTOUCHTARGET);
    #endif
  }

  // ----- draw outlines of radio buttons
  for (int ii=e4DIGIT; ii<=e6DIGIT; ii++) {
    FunctionButton item = myButtons[ii];
    int xCenter = item.x - 16;
    int yCenter = item.y + (item.h/2);

    // outline the radio button
    // the active button will be indicated in updateScreen()
    tft->drawCircle(xCenter, yCenter, 7, cVALUE);
  }

  // ----- draw text fields
  txtSettings4[0].setBackground(cBUTTONFILL); // change background for text on top of buttons
  for (int ii=0; ii<nTextCrossing; ii++) {
      txtSettings4[ii].print();
  }

  showScreenBorder();                 // optionally outline visible area
  showScreenCenterline();             // optionally draw alignment bar

  updateScreen();                     // update UI immediately, don't wait for laggy mainline loop

  #ifdef SHOW_SCREEN_CENTERLINE
    // show centerline at      x1,y1              x2,y2             color
    tft->drawLine( tft->width()/2,0,  tft->width()/2,tft->height(), cWARN); // debug
  #endif
}


bool ViewSettings4::onTouch(Point touch) {
  Serial.println("->->-> Touched settings screen.");
  bool handled = false;               // assume a touch target was not hit
  for (int ii=0; ii<nButtonsDigits; ii++) {
    FunctionButton item = myButtons[ii];
    if (item.hitTarget.contains(touch)) {
        handled = true;               // hit!
        switch (item.functionIndex)   // do the thing
        {
          case e4DIGIT:
              f4Digit();
              break;
          case e6DIGIT:
              f6Digit();
              break;
          default:
              Serial.print("Error, unknown function "); Serial.println(item.functionIndex);
              break;
        }
        this->saveConfig();           // after UI is updated, save setting to nvr
     }
  }
  return handled;                     // true=handled, false=controller uses default action
}

// ========== load/save config setting =========================
// the user's choice of 4- or 6-digit grid announcements is saved in the model, not here
// so all we have to do is tell the model to save itself (a lengthy operation)

// ----- load from SDRAM -----
void ViewSettings4::loadConfig() {
  // Nothing to load - this setting is stored in the model
}
// ----- save to SDRAM -----
void ViewSettings4::saveConfig() {
  model->save();
}
