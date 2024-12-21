#pragma once   // Please format this file with clang before check-in to GitHub

#define ANIMATED_LOGO   // Pick one, recompile
// #define STARBURST     // Pick one, recompile
// #define TIME_TUNNEL   // Pick one, recompile
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
            We can independently write anything we want in this module.
            To save flash program space, we only compile one result at a
            time, selectable by #define. See top of file.

  Animated Logo:
            +-----------------------------------+
            |      |                     |      |
            |   ---+---------------------+---   |
            |      |      GGGGGGG        |      |
            |      |     G               |      |
            |      |     G     GGG       |      |
            |      |       GGGGG         |      |
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
extern void selectNewView(int cmd);      // Griduino.ino
extern int goto_next_view;               // Griduino.ino
extern int help_view;                    // Griduino.ino

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
  // The "screen 1" implementations must implement:
  void startView(int totalDelayMsec);
  bool continueViewing();
  void pinwheel(int x0, int y0, uint16_t color);
  void animateHorizLine(int left, int right, int y, int color);
  void animateVertLine(int top, int bot, int x, int color);
  void drivePath(Route path[], int count);
};
// end class ViewScreen1

// ============== implement public interface, common to all ================
void ViewScreen1::updateScreen() {
  bool allDone = continueViewing();
  if (allDone) {                // all done with very first screen
    selectNewView(help_view);   // switch to very second screen
  }
}

void ViewScreen1::startScreen() {
  // one-time preparation
  startView(6000);
  showDefaultTouchTargets();   // optionally draw box around default button-touch areas
  showMyTouchTargets(0, 0);    // no real buttons on this view
  showScreenBorder();          // optionally outline visible area
  showScreenCenterline();      // optionally draw visual alignment bar
  updateScreen();
}

bool ViewScreen1::onTouch(Point touch) {
  selectNewView(goto_next_view);   // any touch anywhere, exit this time-wasting view
  return true;                     // true=handled, false=controller uses default action
}   // end onTouch()

// ============== implement Starburst ================
#if defined(STARBURST)
/*
Example:  What we're REALLY trying to do is this simple:
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

// Starburst variables
const int maxFullScreens    = 4;   // max number of full screens to show before we're done
const int numStarsPerScreen = 4;   // number of stars (pinwheels) per full screen
int starNum;                       // count 0..numStarsPerScreen
int starDelay;                     // msec delay after each full screen to admire display
int countFullScreens;              // count 0..maxFullScreens
const int maxStarLoops = 4;

void ViewScreen1::pinwheel(int x0, int y0, uint16_t color) {
  // draw a starburst with arbitrary origin x0,y0
  // and evenly-spaced lines

  const int w = gScreenWidth;
  const int h = gScreenHeight;

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

void ViewScreen1::startView(int totalDelayMsec) {
  starNum          = 0;                               // how many stars have been drawn on this screen
  starDelay        = totalDelayMsec / maxStarLoops;   // msec delay between full screens
  countFullScreens = 0;                               // how many screens have been filled
}

bool ViewScreen1::continueViewing() {
  // divide up our screen updates into small units that are called frequently from updateScreen()
  // this allows us to check Touch() and other events
  // keep track of our own state
  const int w = gScreenWidth;
  const int h = gScreenHeight;

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
#endif

// ============== implement Time Tunnel ================
#if defined(TIME_TUNNEL)
void ViewScreen1::startView(int totalDelayMsec) {
  // 2022-11-17 'time tunnel' is available but currently unused
  int startMsec = millis();
  int endMsec   = startMsec + totalDelayMsec;
  clearScreen();

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
bool ViewScreen1::continueViewing() {
  // todo: divide up our screen updates into small units that are called frequently from updateScreen()
  // this allows us to check Touch() and other events
  return true;   // false = keep processing, true = finished, exit Screen1
}
#endif

// ============== implement Animated Logo ================
#if defined(ANIMATED_LOGO)
// ----- icons
const int icon6w = 6;
const int icon6h = 6;
// '2023-01-13 round-icon-6x6', 6x6px
// generated by http://javl.github.io/image2cpp/
const unsigned char icon6[] PROGMEM = {
    0x78, 0xfc, 0xfc, 0xfc, 0xfc, 0x78};

const int icon9w = 9;
const int icon9h = 9;
// '2023-01-13 round-icon-9x9', 9x9px
const unsigned char icon9[] PROGMEM = {
    0x3e, 0x00, 0x7f, 0x00, 0xff, 0x80, 0xff, 0x80, 0xff, 0x80, 0xff, 0x80, 0xff, 0x80, 0x7f, 0x00,
    0x3e, 0x00};

// ----- animation
void ViewScreen1::animateHorizLine(int left, int right, int y, int color) {
  for (int xx = left; xx < right; xx++) {
    tft->drawBitmap(xx - icon6w / 2, y - icon6h / 2, icon6, icon6w, icon6h, ILI9341_BLACK);
    delay(1);
  }
}

void ViewScreen1::animateVertLine(int top, int bot, int x, int color) {
  for (int yy = top; yy < bot; yy++) {
    tft->drawBitmap(x - icon6w, yy, icon6, icon6w, icon6h, ILI9341_BLACK);
    delay(1);
  }
}

#define MY_NAVY 0x0014   /// 0, 0, 20
void ViewScreen1::drivePath(Route path[], int count) {
  for (int ii = count - 1; ii >= 0; ii--) {
    tft->drawBitmap(path[ii].x, path[ii].y, icon9, icon9w, icon9h, MY_NAVY);
    delay(15);
  }
}

// clang-format off
Route stroke1[] = { // from bottom, drive upwards
    {181, 240}, {181, 235}, {182, 233}, {182, 231}, {183, 229},   // 1-5
    {183, 227}, {183, 225}, {184, 223}, {184, 221}, {184, 219},   // 6-10
    {185, 217}, {185, 215}, {185, 213}, {186, 211}, {186, 209},   // 11-15
    {187, 207}, {187, 205}, {188, 203}, {188, 201}, {188, 199},   // 16-20
    {189, 197}, {189, 195}, {190, 193}, {190, 191}, {191, 189},   // 21-25
    {192, 187}, {192, 185}, {193, 183}, {193, 181}, {195, 177},   // 26-30
    {195, 175}, {196, 173}, {197, 170}, {197, 169}, {198, 167},   // 31-35
    {199, 165}, {200, 163}, {201, 161}, {201, 159}, {202, 157},   // 36-40
    {203, 155}, {204, 153}, {204, 151}, {206, 149}, {206, 147},   // 41-45
    {207, 145}, {208, 143}, {209, 141}, {210, 139}, {211, 137},   // 46-50
    {214, 133}, {216, 131}, {217, 129}, {218, 127}, {219, 125},   // 51-55
    {221, 123}, {222, 121}, {223, 119}, {225, 117}, {226, 115},   // 56-60
    {227, 115},                                                   // 61
};
Route stroke2[] {
    {226, 115}, {224, 117}, {222, 117}, {220, 118}, {218, 118},   // 1-5
    {216, 119}, {214, 119}, {212, 120}, {210, 120}, {208, 121},   // 6-10
    {206, 121}, {204, 122}, {202, 122}, {200, 123}, {198, 123},   // 11-15
    {196, 124}, {194, 124}, {191, 125}, {189, 126}, {186, 127},   // 16-20
    {183, 128}, {180, 129}, {177, 130}, {174, 130}, {171, 131},   // 21-25
    {168, 131}, {165, 132}, {162, 133}, {158, 134}, {154, 135},   // 26-30
    {150, 136}, {146, 136}, {142, 136},                           // 31-33
};
Route stroke3[] {
    {142, 136}, {145, 135}, {148, 134}, {150, 133}, {152, 132},   // 1-5
    {155, 131}, {158, 130}, {161, 129}, {165, 129}, {168, 129},   // 6-10
    {172, 130}, {176, 131}, {178, 132}, {180, 133}, {182, 134},   // 11-15
    {184, 135}, {186, 137}, {188, 138}, {190, 139}, {192, 140},   // 16-20
    {194, 141}, {196, 143}, {198, 145}, {200, 147}, {202, 149},   // 21-25
    {203, 151}, {203, 153}, {202, 156},                           // 26-28
};
Route stroke4[] {
    {202, 156}, {201, 158}, {200, 160}, {199, 162}, {198, 164},   // 1-5
    {196, 166}, {194, 168}, {191, 170}, {189, 172}, {187, 173},   // 6-10
    {185, 174}, {183, 175}, {179, 177}, {176, 178}, {173, 179},   // 11-15
    {170, 180}, {165, 181}, {161, 182}, {157, 183}, {154, 183},   // 16-20
    {151, 183}, {148, 184}, {145, 184}, {142, 184},               // 21-24
};
Route stroke5[] {
    {139, 184}, {136, 184}, {133, 183}, {130, 183}, {127, 182},   // 1-5
    {125, 181}, {122, 180}, {120, 179}, {118, 178}, {116, 176},   // 6-10
    {114, 174}, {112, 172}, {110, 170}, {109, 168}, {108, 166},   // 11-15
    {106, 164}, {105, 162}, {104, 160}, {103, 158}, {103, 156},   // 16-20
    {103, 154}, {102, 151},                                       // 21-22
};
Route stroke6[] {
    {102, 149}, {102, 147}, {102, 144}, {102, 141}, {102, 138},   // 1-5
    {103, 135}, {103, 132}, {103, 129}, {103, 126}, {104, 123},   // 6-10
    {105, 120}, {106, 117}, {107, 114}, {108, 111}, {109, 108},   // 11-15
    {110, 105}, {112, 102}, {113,  99}, {114,  96}, {116,  93},   // 16-20
    {118,  90}, {120,  87}, {122,  84}, {124,  81}, {126,  78},   // 21-25
    {129,  75}, {131,  72}, {134,  69}, {137,  66}, {141,  63},   // 26-30
    {145,  60}, {149,  58}, {152,  56}, {155,  54}, {158,  53},   // 31-35
    {161,  53}, {164,  52}, {168,  51}, {171,  51},               // 36-39
};
Route stroke7[] {
    {174,  51}, {177,  51}, {180,  51}, {183,  52}, {186,  53},   // 1-5
    {189,  54}, {192,  55}, {194,  56}, {196,  57}, {198,  59},   // 6-10
    {200,  61}, {202,  63}, {203,  65}, {204,  67}, {205,  68},   // 11-15
    {205,  69}, {206,  70},                                       // 16-17
};
const int elements1 = sizeof(stroke1) / sizeof(stroke1[0]);
const int elements2 = sizeof(stroke2) / sizeof(stroke2[0]);
const int elements3 = sizeof(stroke3) / sizeof(stroke3[0]);
const int elements4 = sizeof(stroke4) / sizeof(stroke4[0]);
const int elements5 = sizeof(stroke5) / sizeof(stroke5[0]);
const int elements6 = sizeof(stroke6) / sizeof(stroke6[0]);
const int elements7 = sizeof(stroke7) / sizeof(stroke7[0]);
// clang-format on

void ViewScreen1::startView(int totalDelayMsec) {

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 0xC0);   // backlight 75% brightness to reduce glare, screen is mostly white
}
bool ViewScreen1::continueViewing() {
  // RGB 565 true color: https://chrishewett.com/blog/true-rgb565-colour-picker/

  int w = tft->width();
  int h = tft->height();

  clearScreen(ILI9341_WHITE);                            // white background except backlight is 75%
  animateHorizLine(18, w - 18, 36, ILI9341_BLACK);       // top line
  animateHorizLine(18, w - 18, h - 36, ILI9341_BLACK);   // bottom line
  animateVertLine(14, h - 14, 60, ILI9341_BLACK);        // left line
  animateVertLine(14, h - 14, w - 60, ILI9341_BLACK);    // right line

  delay(200);

  // draw elegant letter 'G' in logo font named "Bad Script"
  drivePath(stroke7, elements7);
  drivePath(stroke6, elements6);
  drivePath(stroke5, elements5);
  drivePath(stroke4, elements4);
  drivePath(stroke3, elements3);
  delay(100);   // pause pen at sharp corners
  drivePath(stroke2, elements2);
  delay(100);
  drivePath(stroke1, elements1);

  delay(700);

  TextField txtScreen1[] = {
      //        text     x,y    color       alignment    font size
      {PROGRAM_TITLE, 72, 14, ILI9341_NAVY, ALIGNLEFT, eFONTSYSTEM},
      {PROGRAM_VERSION, 72, h - 28, ILI9341_NAVY, ALIGNLEFT, eFONTSYSTEM},
  };
  const int numScreen1Fields = sizeof(txtScreen1) / sizeof(txtScreen1[0]);

  // ----- draw text fields
  for (int ii = 0; ii < numScreen1Fields; ii++) {
    txtScreen1[ii].print();
    delay(700);
  }

  delay(4000);

  // todo: divide up our screen updates into small units that are called frequently from updateScreen()
  // this allows us to check Touch() and other events
  return true;   // false = keep processing, true = finished, exit Screen1
}
#endif