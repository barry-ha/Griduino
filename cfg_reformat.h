#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:    cfg_reformat.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Factory reset the flash memory.
            This is useful to recover from a fatal memory error.

            +--------------------------------------------+
            |            Flash Memory Error              |...yRow1
            |                                            |
            |   All settings, logs, trails and audio     |...yRow2
            |   files will be erased. If you need to     |...yRow3
            |   reformat frequently, please let us       |...yrow4
            |   know at griduino@gmail.com               |...yRow5
            |                                            |
            |               +------------+               |...yButtonUL
            |               |  Reformat  |               |
            |               +------------+               |
            |                                            |
            |        Program v1.14.4, Dec 3 2024         |...yRow9
            +---:-----------:------------:---------------+
            xLeft    xButtonUL
*/

#include "constants.h"   // Griduino constants and colors
#include "logger.h"      // conditional printing to Serial port
#include "TextField.h"   // Optimize TFT display text for proportional fonts
#include "view.h"        // Base class for all views
// #include "SdFat_format/SdFat_format.h"   // Adafruit FAT formatter: provides format_fat12(), check_fat12()

// ========== extern ===========================================
extern int goto_next_view;        // Griduino.ino
extern void ff_setup(void);       // ff_SdFat_Format.cpp
extern void format_fat12(void);   // ff_SdFat_format.cpp
extern void check_fat12(void);    // ff_SdFat_format.cpp

// ========== class ViewCfgReformat =================================
class ViewCfgReformat : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewCfgReformat(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {
    background = cBACKGROUND;   // every view can have its own background color
  }
  void updateScreen();
  void startScreen();
  void endScreen();
  bool onTouch(Point touch) override;

protected:
  // ---------- local data for this derived class ----------
  // color scheme: see constants.h

  // ========== text screen layout ===================================

  // vertical placement of text rows
  const int lineHeight = 24;

  const int yRow1 = 18;
  const int yRow2 = yRow1 + 50;
  const int yRow3 = yRow2 + lineHeight;
  const int yRow4 = yRow3 + lineHeight;
  const int yRow5 = yRow4 + lineHeight;
  const int yRow6 = yRow5 + lineHeight;
  const int yRow7 = yRow6 + lineHeight;
  const int yRow9 = gScreenHeight - 10;   // "v1.14.4, compiled Dec 16, 2024"

  const int labelX = 10;   // left-adjusted static text

  // ----- screen text
  // clang-format off
#define nTextReformat 6
  TextField txtStatic[nTextReformat] = {
      {"Reformat Flash Memory",                   -1,     yRow1, cTITLE, ALIGNCENTER},   // view title, centered
      {"All settings, logs, and audio files",     labelX, yRow2, cFAINT, ALIGNLEFT},
      {"will be erased. If you need to reformat", labelX, yRow3, cFAINT, ALIGNLEFT},
      {"often, please let us know at",            labelX, yRow4, cFAINT, ALIGNLEFT},
      {"griduino@gmail.com",                      labelX, yRow5, cFAINT, ALIGNLEFT},
      {PROGRAM_VERDATE,                           -1,     yRow9, cLABEL, ALIGNCENTER},
  };
  // clang-format on

  enum buttonID {
    eREFORMAT,
  };
  // clang-format off
#define nReformatButtons 1
#define buttonW   (107)    // = 320/3 = about one-third width of screen
#define buttonH   (44)

#define xButtonUL  (320/2 - buttonW/2)    // centered
#define yButtonUL  (158)      
  FunctionButton myButtons[nReformatButtons] = {
      // 3.2" display is 320 x 240 pixels, landscape, (y=239 reserved for activity bar)
      //
      // label            origin              size                touch-target
      // text              x,y                 w,h                      x,y                     w,h         radius  color     functionID
      {"Reformat", xButtonUL,yButtonUL,  buttonW,buttonH,  {  xButtonUL-3,yButtonUL-3,  buttonW+6,buttonH+6},  4, cTEXTCOLOR, eREFORMAT },
  };
  // clang-format on

  // ---------- local functions for this derived class ----------
  void reformatFlash() {
    logger.log(CONFIG, INFO, "->->-> Clicked REFORMAT FLASH button.");
    logger.log(CONFIG, INFO, "Starting to format FAT12 filesystem");

    // Call fatfs begin and passed flash object to initialize file system
    logger.fencepost("cfg_refornat.h", "ff_setup()", __LINE__);   // debug
    ff_setup();
    format_fat12();
    check_fat12();
    logger.fencepost("cfg_refornat.h", "reformatFlash()", __LINE__);   // debug

    // TODO - turn off spoken-word audio (if it was on)

    // TODO - success message should tell user
    // 1. Flash reformat success/failure
    countdown("Formatting flash");
    countdown("Flashing format");
    countdown("Erasing chip");
    countdown("Washing bits");
    countdown("Drying bits");
    countdown("Petting cat");
    countdown("Watching TV");

    // To re-install audio files, see github.com/barry-ha/Griduino docs folder
    finalScreen();

    logger.log(CONFIG, INFO, "Flash chip formatting finished");
  }

  // ----- visible delay with message
  // Creating new file system is practically instantaneous, so we add
  // a visible delay (with funny messages) to add gravitas
  // clang-format off
  FunctionButton btnWait = {
      // label             origin             size            touch-target
      // text               x,y                w,h           0,0   w,h  radius color functionID
      "Please Wait", (320/5),(240/5), (320*3/5),(240*3/5), {1, 1, 1, 1}, 4, cWARN, 0,
  };
  // clang-format on

  void countdown(const char *message) {
    logger.log(CONFIG, DEBUG, message);

    // draw "Wait" message as if it's a button
    setFontSize(eFONTSMALL);

    // ----- button outline
    FunctionButton item = btnWait;
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cWARN);

    // ----- label on top of button
    int labelx = getOffsetToCenterTextOnButton(message, item.x, item.w);
    int labely = item.y + (item.h / 2) - 16;   // a little above center, to make room below for countdown

    tft->setCursor(labelx, labely);   // message apprx centered inside button
    tft->setTextColor(item.color);
    tft->print(message);

    // ----- countdown number is one line below "Please wait" message
    int xx = getOffsetToCenterTextOnButton("4 3 2 1", btnWait.x, btnWait.w);
    int yy = labely + 36;   // countdown one line below message
    tft->setCursor(xx, yy);
    tft->setTextColor(cHIGHLIGHT);

    // show progress countdown
    for (int ii = 4; ii > 0; ii--) {
      tft->print(ii);
      tft->print(" ");
      delay(1200);
    }
  }

  // clang-format off
#define nTextFinal 7
  TextField txtFinal[nTextFinal] = {
      {"Reformat Successful",                 -1,     yRow1, cTITLE, ALIGNCENTER,   eFONTSMALLEST},   // view title, centered
      {"Be sure to re-install audio files",   labelX, yRow2, cHIGHLIGHT, ALIGNLEFT, eFONTSMALLEST},
      {"if you want spoken word audio for",   labelX, yRow3, cHIGHLIGHT, ALIGNLEFT, eFONTSMALLEST},
      {"grid lines.",                         labelX, yRow4, cHIGHLIGHT, ALIGNLEFT, eFONTSMALLEST},
      {"See docs folder in",                  labelX, yRow5, cHIGHLIGHT, ALIGNLEFT, eFONTSMALLEST},
      {"github.com/barry-ha/Griduino",        labelX, yRow6, cHIGHLIGHT, ALIGNLEFT, eFONTSMALLEST},
      {PROGRAM_VERDATE,                       -1,     yRow9, cLABEL, ALIGNCENTER},
  };
  // clang-format on

  // ----- final format reminders
  void finalScreen() {
    // shown once after flash reformat
    this->clearScreen(this->background);                // clear screen
    txtFinal[0].setBackground(this->background);        // set background for all TextFields in this view
    TextField::setTextDirty(txtFinal, nTextReformat);   // make sure all fields get re-printed on screen change
    setFontSize(eFONTSMALLEST);

    drawAllIcons();                     // draw gear (settings) and arrow (next screen)
    showDefaultTouchTargets();          // optionally draw box around default button-touch areas
    showMyTouchTargets(myButtons, 0);   // optionally show this view's touch targets
    showScreenBorder();                 // optionally outline visible area
    showScreenCenterline();             // optionally draw visual alignment bar

    // ----- draw text fields
    for (int ii = 0; ii < nTextFinal; ii++) {
      txtFinal[ii].print();
    }

    showProgressBar(9, 9);   // draw marker for advancing through settings
    updateScreen();          // update UI immediately, don't wait for laggy mainline loop
  }   // end startScreen()

};   // end class ViewCfgReformat

// ============== implement public interface ================
void ViewCfgReformat::updateScreen() {
  // called on every pass through main()
  // Nothing to do here - this module has nothing to update

}   // end updateScreen

void ViewCfgReformat::startScreen() {
  // called once each time this view becomes active
  logger.log(SCREEN, DEBUG, "ViewCfgReformat::startScreen()");
  this->clearScreen(this->background);                 // clear screen
  txtStatic[0].setBackground(this->background);        // set background for all TextFields in this view
  TextField::setTextDirty(txtStatic, nTextReformat);   // make sure all fields get re-printed on screen change
  setFontSize(eFONTSMALLEST);

  drawAllIcons();                                    // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();                         // optionally draw box around default button-touch areas
  showMyTouchTargets(myButtons, nReformatButtons);   // optionally show this view's touch targets
  showScreenBorder();                                // optionally outline visible area
  showScreenCenterline();                            // optionally draw visual alignment bar

  // ----- draw text fields
  for (int ii = 0; ii < nTextReformat; ii++) {
    txtStatic[ii].print();
  }

  // ----- draw buttons
  setFontSize(eFONTSMALLEST);
  for (int ii = 0; ii < nReformatButtons; ii++) {
    FunctionButton item = myButtons[ii];
    tft->fillRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONFILL);
    tft->drawRoundRect(item.x, item.y, item.w, item.h, item.radius, cBUTTONOUTLINE);

    // ----- label on top of button
    int xx = getOffsetToCenterTextOnButton(item.text, item.x, item.w);

    tft->setCursor(xx, item.y + item.h / 2 + 5);   // place text centered inside button
    tft->setTextColor(item.color);
    tft->print(item.text);
  }

  showProgressBar(9, 9);   // draw marker for advancing through settings
  updateScreen();          // update UI immediately, don't wait for laggy mainline loop
}   // end startScreen()

void ViewCfgReformat::endScreen() {
  // Called once each time this view becomes INactive
  // Nothing to do here - this module has no settings to save
}

bool ViewCfgReformat::onTouch(Point touch) {
  bool handled = false;   // assume a touch target was not hit
  for (int ii = 0; ii < nReformatButtons; ii++) {
    FunctionButton item = myButtons[ii];
    if (item.hitTarget.contains(touch)) {
      handled = true;               // hit!
      switch (item.functionIndex)   // do the thing
      {
      case eREFORMAT:
        reformatFlash();
        // selectNewView(goto_next_view);
        break;

      default:
        logger.log(CONFIG, ERROR, "unknown function %d", item.functionIndex);
        break;
      }
      updateScreen();   // update UI immediately, don't wait for laggy mainline loop
    }
  }
  return handled;   // true=handled, false=controller uses default action
}   // end onTouch()
