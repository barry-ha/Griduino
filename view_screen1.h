#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     view_screen1.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  When you power up the Griduino, we display Screen 1.
            This is a visual time-waster while waiting about 6 seconds
            for Windows USB port to stabilize and connect to a
            serial monitor program.

  Pinwheel:
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
  void starburst(int totalDelayMsec) {
    int w        = gScreenWidth;
    int h        = gScreenHeight;
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
};   // end class ViewScreen1

// ============== implement public interface ================
void ViewScreen1::updateScreen() {
  // called on every pass through main()
  // nothing to do in the main loop - this screen has no dynamic items
}

void ViewScreen1::startScreen() {
  // called once during bootup, and optionally via console command
  // goal is to waste 6-8 seconds while looking busy
  starburst(6000);
  // timeTunnel(10000);

  showDefaultTouchTargets();   // optionally draw box around default button-touch areas
  showMyTouchTargets(0, 0);    // no real buttons on this view
  showScreenBorder();          // optionally outline visible area
  showScreenCenterline();      // optionally draw visual alignment bar

  updateScreen();   // update UI immediately, don't wait for the main loop to eventually get around to it
}

bool ViewScreen1::onTouch(Point touch) {
  // do nothing - this screen does not respond to buttons
  logger.info("->->-> Touched screen1.");
  return false;   // true=handled, false=controller uses default action

}   // end onTouch()
