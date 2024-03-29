// Please format this file with clang before check-in to GitHub
/*
  This is a splash screen playground.

  Version history:
            2023-01-15 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Tested with:
         1. Adafruit Feather M4 Express (120 MHz SAMD51)    https://www.adafruit.com/product/3857

         2. Adafruit Feather RP2040 (133 MHz M0+)           https://www.adafruit.com/product/4884

         3. Adafruit 3.2" TFT color LCD display ILI-9341    https://www.adafruit.com/product/1743

*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include "hardware.h"           // Griduino pin definitions
#include "constants.h"          // Griduino constants, colors, typedefs

// ------- Identity for splash screen and console --------
#define PROGRAM_NAME "Animate Logo"

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ------------ definitions
const int howLongToWait = 6;   // max number of seconds at startup waiting for Serial port to console

void clearScreen(uint16_t color) {
  tft.fillScreen(color);
}

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
void animateHorizLine(int left, int right, int y, int color) {
  for (int xx = left; xx < right; xx++) {
    tft.drawBitmap(xx - icon6w / 2, y - icon6h / 2, icon6, icon6w, icon6h, ILI9341_BLACK);
    delay(1);
  }
}

void animateVertLine(int top, int bot, int x, int color) {
  for (int yy = top; yy < bot; yy++) {
    tft.drawBitmap(x - icon6w, yy, icon6, icon6w, icon6h, ILI9341_BLACK);
    delay(1);
  }
}

#define MY_NAVY 0x0014                     /// 0, 0, 20
void drivePath(Route path[], int count);   // declaration fixes "error: variable or field 'drivePath' declared void", dunno why it's required
void drivePath(Route path[], int count) {
  for (int ii = count - 1; ii >= 0; ii--) {
    tft.drawBitmap(path[ii].x, path[ii].y, icon9, icon9w, icon9h, MY_NAVY);
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
    {142, 136}, {145, 135}, {148, 134}, {150, 133}, {152, 132},   // s3 1-5
    {155, 131}, {158, 130}, {161, 129}, {165, 129}, {168, 129},   // s3 6-10
    {172, 130}, {176, 131}, {178, 132}, {180, 133}, {182, 134},   // s3 11-15
    {184, 135}, {186, 137}, {188, 138}, {190, 139}, {192, 140},   // s3 16-20
    {194, 141}, {196, 143}, {198, 145}, {200, 147}, {202, 149},   // s3 21-25
    {203, 151}, {203, 153}, {202, 156},                           // s3 26-28
};
Route stroke4[] {
    {202, 156}, {201, 158}, {200, 160}, {199, 162}, {198, 164},   // s4 1-5
    {196, 166}, {194, 168}, {191, 170}, {189, 172}, {187, 173},   // s4 6-10
    {185, 174}, {183, 175}, {179, 177}, {176, 178}, {173, 179},   // s4 11-15
    {170, 180}, {165, 181}, {161, 182}, {157, 183}, {154, 183},   // s4 16-20
    {151, 183}, {148, 184}, {145, 184}, {142, 184},               // s4 21-24
};
Route stroke5[] {
    {139, 184}, {136, 184}, {133, 183}, {130, 183}, {127, 182},   // s5 1-5
    {125, 181}, {122, 180}, {120, 179}, {118, 178}, {116, 176},   // s5 6-10
    {114, 174}, {112, 172}, {110, 170}, {109, 168}, {108, 166},   // s5 11-15
    {106, 164}, {105, 162}, {104, 160}, {103, 158}, {103, 156},   // s5 16-20
    {103, 154}, {102, 151},                                       // s5 21-22
};
Route stroke6[] {
    {102, 149}, {102, 147}, {102, 144}, {102, 141}, {102, 138},   // s6 1-5
    {103, 135}, {103, 132}, {103, 129}, {103, 126}, {104, 123},   // s6 6-10
    {105, 120}, {106, 117}, {107, 114}, {108, 111}, {109, 108},   // s6 11-15
    {110, 105}, {112, 102}, {113,  99}, {114,  96}, {116,  93},   // s6 16-20
    {118,  90}, {120,  87}, {122,  84}, {124,  81}, {126,  78},   // s6 21-25
    {129,  75}, {131,  72}, {134,  69}, {137,  66}, {141,  63},   // s6 26-30
    {145,  60}, {149,  58}, {152,  56}, {155,  54}, {158,  53},   // s6 31-35
    {161,  53}, {164,  52}, {168,  51}, {171,  51},               // s6 36-39
};
Route stroke7[] {
    {174,  51}, {177,  51}, {180,  51}, {183,  52}, {186,  53},   // s7 1-5
    {189,  54}, {192,  55}, {194,  56}, {196,  57}, {198,  59},   // s7 6-10
    {200,  61}, {202,  63}, {203,  65}, {204,  67}, {205,  68},   // s7 11-15
    {205,  69}, {206,  70},                                       // s7 16-17
};
const int elements1 = sizeof(stroke1) / sizeof(stroke1[0]);
const int elements2 = sizeof(stroke2) / sizeof(stroke2[0]);
const int elements3 = sizeof(stroke3) / sizeof(stroke3[0]);
const int elements4 = sizeof(stroke4) / sizeof(stroke4[0]);
const int elements5 = sizeof(stroke5) / sizeof(stroke5[0]);
const int elements6 = sizeof(stroke6) / sizeof(stroke6[0]);
const int elements7 = sizeof(stroke7) / sizeof(stroke7[0]);
// clang-format on

//=========== setup ============================================
void setup() {

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 0xC0);   // backlight 75% brightness to reduce glare, screen is mostly white

  // ----- init TFT display
  tft.begin();                     // initialize TFT display
  tft.setRotation(LANDSCAPE);      // 1=landscape (default is 0=portrait)
  tft.fillScreen(ILI9341_BLACK);   // note that "begin()" does not clear screen

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);   // init for debugging in the Arduino IDE
  delay(20);              // minimal delay, this is the screen usually shown while waiting for Serial
  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_NAME " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init Feather M4 onboard lights
  pinMode(RED_LED, OUTPUT);     // diagnostics RED LED
  digitalWrite(PIN_LED, LOW);   // turn off little red LED
  Serial.println("NeoPixel initialized and turned off");
}

//=========== main work loop ===================================

void loop() {
  // RGB 565 true color: https://chrishewett.com/blog/true-rgb565-colour-picker/

  int w = tft.width();
  int h = tft.height();

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

  delay(800);

  tft.setCursor(72, 14);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_NAVY);
  tft.print("Griduino.com");

  delay(700);

  tft.setCursor(72, h - 28);
  tft.print(PROGRAM_VERSION);

  delay(4000);
}
