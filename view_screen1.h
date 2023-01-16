#pragma once   // Please format this file with clang before check-in to GitHub
// #define ANIMATED_LOGO      // Pick one, recompile
 #define STARBURST          // Pick one, recompile
// #define TIME_TUNNEL        // Pick one, recompile
/*
  File:     view_screen1.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  When you power up the Griduino, we display Screen 1.
            This is a visual time-waster while waiting about 6 seconds
            for Windows USB port to stabilize and connect to a
            serial monitor program.
            Invoked once during startup and optionally from command line.
            Goal is to waste 6-8 seconds while looking busy and also
            checking for exit conditions, such as touching the screen.
            All "screen 1" implementations must divide its work into small
            time slices, to allow checking touchscreen and exit conditions.
  Usage:
            Main code is not aware of how "screen 1" looks, nor does it
            request any particular implementation. 
  Griduino.ino:
            ViewScreen1       screen1View(&tft, SCREEN1_VIEW);

  Implementation:
            We can independently build anything we want into this module.
            To save flash program space, we only compile one result at a 
            time, selectable by #define. See top of file.

  Animated Logo:
            +-----------------------------------+
            |      |                     |      |
            |   ---+---------------------+---   |
            |      |                     |      |
            |      |                     |      |
            |      |          G          |      |
            |      |                     |      |
            |   ---+---------------------+---   |
            |      |                     |      |
            +-----------------------------------+
  Pinwheel / Starburst:
            +-----------------------------------+
            |     \ | /         \   |   /       |
            |      \|/           \  |  /        |
            | - - - * - - - - - - \ | /- - - - -|
            |      /|\             \|/          |
            |- - -/-|-\ - - - - - - * - - - - - |
            |    /  |  \           /|\          |
            |   /   |   \         / | \         |
            |  /    |    \       /  |  \        |
            +-----------------------------------+
  Time Tunnel:
            +-----------------------------------+
            | +-------------------------------+ |
            | | +---------------------------+ | |
            | | | +-----------------------+ | | |
            | | | | +-------------------+ | | | |
            | | | | |                   | | | | |
            | | | | +-------------------+ | | | |
            | | +---------------------------+ | |
            | +-------------------------------+ |
            +-----------------------------------+
*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include "constants.h"          // Griduino constants and colors
#include "logger.h"             // conditional printing to Serial port
#include "TextField.h"          // Optimize TFT display text for proportional fonts
#include "view.h"               // Base class for all views

// ========== extern ===========================================
extern Logger logger;                    // Griduino.ino
extern void showDefaultTouchTargets();   // Griduino.ino
extern int goto_next_view;               // Griduino.ino

// ========== class ViewScreen1 =================================
class ViewScreen1 : public View {
public:
  // ---------- public interface ----------
  // This derived class must implement the public interface:
  ViewScreen1(Adafruit_ILI9341 *vtft, int vid)   // ctor
      : View{vtft, vid} {}
  void updateScreen();
  void startScreen();
  bool onTouch(Point touch);

protected:
  const int w = gScreenWidth;
  const int h = gScreenHeight;

  // Starburst variables
  const int maxFullScreens    = 4;   // max number of full screens to show before we're done
  const int numStarsPerScreen = 4;   // number of stars (pinwheels) per full screen
  int starNum;                       // count 0..numStarsPerScreen
  int starDelay;                     // msec delay after each full screen to admire display
  int countFullScreens;              // count 0..maxFullScreens
  const int maxStarLoops = 4;
  void startStarburst(int totalDelayMsec) {
    starNum          = 0;                               // how many stars have been drawn on this screen
    starDelay        = totalDelayMsec / maxStarLoops;   // msec delay between full screens
    countFullScreens = 0;                               // how many screens have been filled
  }
  bool continueViewing() {
    // divide up our screen updates into small units that are called frequently from updateScreen()
    // this allows us to check Touch() and other events
    // keep track of our own state
    switch (starNum) {
    case 0:
      this->clearScreen(this->background);                         // clear screen
      pinwheel(random(0, w / 2), random(0, h / 2), ILI9341_RED);   // ul
      starNum++;
      break;
    case 1:
      pinwheel(random(w / 2, w), random(0, h / 2), ILI9341_GREEN);   // ur
      starNum++;
      break;
    case 2:
      pinwheel(random(w / 2, w), random(h / 2, h), ILI9341_YELLOW);   // lr
      starNum++;
      break;
    case 3:
      pinwheel(random(0, w / 2), random(h / 2, h), ILI9341_CYAN);   // ll
      delay(starDelay);
      starNum = 0;          // reset counter
      countFullScreens++;   // finished another screen full of stars
      break;
    }
    bool finished = (countFullScreens > maxStarLoops);
    return finished;
  }
  /*
  void starburst(int totalDelayMsec) {
    int numLoops = 4;
    for (int ii = 0; ii < numLoops; ii++) {
      this->clearScreen(this->background);                            // clear screen
      pinwheel(random(0, w / 2), random(0, h / 2), ILI9341_RED);      // ul
      pinwheel(random(w / 2, w), random(0, h / 2), ILI9341_GREEN);    // ur
      pinwheel(random(w / 2, w), random(h / 2, h), ILI9341_YELLOW);   // lr
      pinwheel(random(0, w / 2), random(h / 2, h), ILI9341_CYAN);     // ll
      delay(totalDelayMsec / numLoops);
    }
  }
  */
  void pinwheel(int x0, int y0, uint16_t color) {
    // draw a starburst with arbitrary origin x0,y0
    // and evenly-spaced lines

    int w2 = tft->width() * tft->width();     // width squared
    int h2 = tft->height() * tft->height();   // height squared
    int r  = sqrt(w2 + h2);                   // radius is the diagonal measure of the display

    float angle       = 0.0;
    const int steps   = 80;   // number of lines to draw within the circle
    const float delta = 2.0 * PI / steps;
    for (angle = 0.0; angle < (2 * PI); angle += delta) {
      int x = (int)(x0 + r * cos(angle));
      int y = (int)(y0 + r * sin(angle));
      tft->drawLine(x0, y0, x, y, color);
    }
  }
#if defined(TIME_TUNNEL)
  void timeTunnel(int totalDelayMsec) {
    // 2022-11-17 'time tunnel' is available but currently unused
    int startMsec = millis();
    int endMsec   = startMsec + totalDelayMsec;
    this->clearScreen(this->background);

    uint16_t colors[]   = {ILI9341_GREEN,
                           ILI9341_CYAN,
                           ILI9341_ORANGE,
                           ILI9341_YELLOW,
                           ILI9341_LIGHTGREY};
    const int numColors = sizeof(colors) / sizeof(colors[0]);

    int x     = 0;
    int y     = 0;
    int w     = gScreenWidth;
    int h     = gScreenHeight;
    bool done = false;
    int ii    = 0;
    while (millis() < endMsec) {
      tft->drawRect(x, y, w, h, colors[ii]);   // look busy
      x += 2;
      y += 2;
      w -= 4;
      h -= 4;
      if (x >= gScreenWidth) {
        x = y = 0;
        w     = gScreenWidth;
        h     = gScreenHeight;
        ii    = (ii + 1) % numColors;
      }
      delay(15);
    }
  }
#endif
};   // end class ViewScreen1

// ============== implement public interface ================
void ViewScreen1::updateScreen() {
  bool allDone = continueViewing();
  if (allDone) {
    selectNewView(goto_next_view);
  }
}

void ViewScreen1::startScreen() {
  // called once during bootup, and optionally via console command
  // goal is to waste 6-8 seconds while looking busy
  startStarburst(6000);
  // starburst(6000);
  // timeTunnel(10000);

  showDefaultTouchTargets();   // optionally draw box around default button-touch areas
  showMyTouchTargets(0, 0);    // no real buttons on this view
  showScreenBorder();          // optionally outline visible area
  showScreenCenterline();      // optionally draw visual alignment bar

  updateScreen();   // update UI immediately, don't wait for the main loop to eventually get around to it
}

bool ViewScreen1::onTouch(Point touch) {
  // todo - when stars are all finished, change view to next thing in list
  logger.info("->->-> Touched screen1.");
  selectNewView(goto_next_view);   // any touch anywhere, exit this decorator view
  return true;                     // true=handled, false=controller uses default action

}   // end onTouch()
