/*
  Griduino -- Grid Square Tracker with GPS

  Version history:
            https://github.com/barry-ha/Griduino/blob/master/downloads/CHANGELOG.md

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is a screen-saver-like playground.
*/

#include <Adafruit_ILI9341.h>         // TFT color display library
#include "constants.h"                // Griduino constants, colors, typedefs

// TFT display and SD card share the hardware SPI interface, and have
// separate 'select' pins to identify the active device on the bus.
  // Adafruit Feather M4 Express pin definitions
  // To compile for Feather M0/M4, install "additional boards manager"
  // https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup
  
  #define TFT_BL   4                  // TFT backlight
  #define TFT_CS   5                  // TFT chip select pin
  #define TFT_DC  12                  // TFT display/command pin

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ------------ definitions
const int howLongToWait = 25;         // max number of seconds at startup waiting for Serial port to console
const int RED_LED = 13;               // diagnostics RED LED

  uint16_t pinks[] = {
    0xd970, 0xd170,   // pink
    0xc970, 0xc170,   // progressively darker...
    0xb970, 0xb170, 
    0xa970, 0xa170, 
    0x9970, 0x9170, 
    0x8970, 0x8170, 
    0x7970, 0x7170, 
    0x6970, 0x6170,
    0x5970, 0x5170,
    0x4970, 0x4170,
    0x3970, 0x3170,
    0x2970, 0x2170,
    0x1970, 0x1170,
    0x0970, 0x0170,   // dark blue
  };
  int numPinks = sizeof(pinks)/sizeof(pinks[0]);

  uint16_t tinyPinks[] = {
    0xd970, 0xd170,   // pink
//    0xc970, 0xc170,   // progressively darker...
//    0xb970, 0xb170, 
//    0xa970, 0xa170, 
//    0x9970, 0x9170, 
    0x8970, 0x8170, 
//    0x7970, 0x7170, 
//    0x6970, 0x6170,
//    0x5970, 0x5170,
//    0x4970, 0x4170,
//    0x3970, 0x3170,
//    0x2970, 0x2170,
    0x0970, 0x0170,   // dark blue
  };
  int numTinyPinks = sizeof(tinyPinks)/sizeof(tinyPinks[0]);

  uint16_t greens[] = {  // 0x0fe9 = rgb(1,63,9)
    0x0fe9,0x0fc9, 0x0fa9,0x0f89, 0x0f69,0x0f49, 0x0f29,0x0f09,  // bright green
    0x0ee9,0x0ec9, 0x0ea9,0x0e89, 0x0e69,0x0e49, 0x0e29,0x0e09,
    0x0de9,0x0dc9, 0x0da9,0x0d89, 0x0d69,0x0d49, 0x0d29,0x0d09,
    0x0ce9,0x0cc9, 0x0ca9,0x0c89, 0x0c69,0x0c49, 0x0c29,0x0c09,
    0x0be9,0x0bc9, 0x0ba9,0x0b89, 0x0b69,0x0b49, 0x0b29,0x0b09,
    0x0ae9,0x0ac9, 0x0aa9,0x0a89, 0x0a69,0x0a49, 0x0a29,0x0a09,
    0x09e9,0x09c9, 0x09a9,0x0989, 0x0969,0x0949, 0x0929,0x0909,
  };
  int numGreens = sizeof(greens)/sizeof(greens[0]);

  uint16_t yellowreds[] = {
    0xffe0, 0xffa0, 0xff60, 0xff20, 
    0xfee0, 0xfea0, 0xfe60, 0xfe20, 
    0xfde0, 0xfda0, 0xfd60, 0xfd20, 
    0xfce0, 0xfca0, 0xfc60, 0xfc20, 
    0xfbe0, 0xfba0, 0xfb60, 0xfb20, 
    0xfae0, 0xfaa0, 0xfa60, 0xfa20, 
    0xf9e0, 0xf9a0, 0xf960, 0xf920, 
  };
  int numYellowreds = sizeof(yellowreds)/sizeof(yellowreds[0]);

// ----- slant wash ----------------------------------
void slantWash(int howLong, uint16_t color[], int numColors, float slope) {
  // Effect: Multiple ripple covers the whole screen and moves (slowly!) towards the left
  // Result: This is too slow. It takes too long to drift sideways, because it 
  //         draws every single line, 320 times, to adjust the screen.
  // inner loop: x=0..w, y=h/2
  // outer loop: increment color
  
  unsigned long targetTime = millis() + howLong*1000;
  int x0, y0;         // start of line
  int x1, y1;         // end of line
  int h = tft.height();
  int offset = (int)(h + slope*x0)/slope; // horiz distance (pixels) from x0 to x1

  int startc = 0;     // starting color index for outer loop
  bool up = true;     // smooth out colors by counting up/down

  int cii = 0;        // color index
  uint16_t c = color[cii];
  int xii = -offset;        // x-axis index
  while (millis() < targetTime) {
    x0 = xii;
    y0 = 0;

    x1 = x0 + offset;
    y1 = h;
    tft.drawLine(x0, y0, x1, y1, c);

    // adjust for next pass through loop
    cii = (cii + 1) % numColors;
    c = color[cii];
    xii += 1;

    // start over when you reach the rhs of the display
    if (xii >= tft.width()) {
      //delay(1);
      startc = (startc + 2) % numColors;
      cii = startc;
      xii = -offset;
    }
  }
}

// ----- slant wave, 1 ----------------------------------
void slantWaveFastV(uint16_t color[], int numColors, int howLong, int howManyScreenFulls) {
  // Effect: SINGLE wave traverses the screen and moves vertically downward.
  //         
  //         This attempts to move faster then "slantWashFastV" by only 
  //         drawing a single wave using an optimized function.
  //         The "background" is the last color in the lookup table.
  // Duration of effect:
  //         Whichever comes first: howLong OR howManyScreenFulls
  // Suggested palette size:
  //         8 .. 32
  
  unsigned long targetTime = millis() + howLong*1000;
  int h = tft.height();
  int w = tft.width();
  int waveCount = 0;

  int cii = 0;          // color index
  int starty = 0;       // starting pixel coordinate for leading edge of wave
  int y = starty;       // y-axis pixel index
  int screenCount = 0;  // how many times we looped through the full screen
  bool done = false;
  //while (millis() < targetTime) {
  while (!done) {

    // draw 1 wave
    uint16_t c;
    // loop once through palette, drawing each line
    for (cii=0; cii<numColors; cii++) {
      c = color[cii];
      tft.drawFastHLine(0, y, w, c);
      y++;
      if (y >= h) {
        y = 0;
      }
    }
    
    starty++;
    if (starty >= h) {
      starty = 0;
      screenCount++;
    }
    y = starty;
    if ( (millis() > targetTime)
      || (screenCount >= howManyScreenFulls)) {
        done = true;
      }
  }
}

// ----- slant wash ----------------------------------
void slantWashFastV(int howLong, uint16_t color[], int numColors) {
  // Effect: Multiple ripple covers the whole screen and moves vertically upward.
  //         This attempts to move faster by only drawing Vertical lines
  //         using the optimized function 
  // Result: Pretty good, not really very convincing waves
  //         About 13.0 seconds to sweep the screen
  // inner loop: x=0..w, y=h/2
  // outer loop: increment color
  
  unsigned long targetTime = millis() + howLong*1000;
  int h = tft.height();
  int w = tft.width();

  int startc = 0;     // starting color index for outer loop
  int cii = 0;        // color index
  int yii = 0;        // y-axis index
  while (millis() < targetTime) {
    uint16_t c = color[cii];
    tft.drawFastHLine(0, yii, w, c);

    // adjust for next pass through loop
    cii = (cii + 1) % numColors;
    yii += 1;

    // start over when you reach the top row of the display
    if (yii >= h) {
      startc = (startc + 2) % numColors;
      cii = startc;
      yii = 0;
    }
  }
}

// ----- slant wave ----------------------------------
void slantWashFastH(int howLong, uint16_t color[], int numColors) {
  // Effect: Multiple ripple covers the whole screen and moves in HORIZONTALLY
  //         This attempts to move faster by only drawing Horizontal lines
  //         using the optimized drawing function 
  // Result: Pretty good, not really very convincing waves
  
  unsigned long targetTime = millis() + howLong*1000;
  int h = tft.height();
  int w = tft.width();

  int startc = 0;     // starting color index for outer loop
  int cii = 0;        // color index
  int xii = 0;        // x-axis index
  while (millis() < targetTime) {
    uint16_t c = color[cii];
    tft.drawFastVLine(xii, 0, h, c);

    // adjust for next pass through loop
    cii = (cii + 1) % numColors;
    xii += 1;

    // start over when you reach the top row of the display
    if (xii >= w) {
      startc = (startc + 2) % numColors;
      cii = startc;
      xii = 0;
    }
  }
}

// ----- color tunnel --------------------------------
void colorTunnel(int howLong, uint16_t color[], int numColors) {
  unsigned long targetTime = millis() + howLong*1000;
  int x = 0;
  int y = 0;
  int w = tft.width();
  int h = tft.height();

  int ii = 0;
  uint16_t c = color[ii];
  while (millis() < targetTime) {
    tft.drawRect(x, y, w, h, c);      // look busy
    x += 2;
    y += 2;
    w -= 4;
    h -= 4;

    // start over when you reach the end of the display
    if (x >= gScreenWidth) {
      x = y = 0;
      w = gScreenWidth;
      h = gScreenHeight;
      uint16_t c = color[ii];
    }

    // adjust for next pass through loop
    delay(18);
    ii = (ii + 1) % numColors;
    c = color[ii];
  }
}

void pinwheel(int x0, int y0, uint16_t color) {
  // draws a starburst with arbitrary origin x0,y0
  // and evenly-spaced lines
  int w2 = tft.width() * tft.width();
  int h2 = tft.height() * tft.height();
  int r = sqrt(w2 + h2);  // radius is the diagonal measure of the display

  float angle = 0.0;
  const int steps = 80;   // number of lines to draw within the circle
  const float delta = 2.0 * PI / steps;
  for (angle=0.0; angle<(2*PI); angle+=delta) {
    int x = (int)(x0 + r*cos(angle));
    int y = (int)(y0 + r*sin(angle));
    tft.drawLine(x0, y0, x, y, color);
  }
  return;
}

// ----- make gradient Palette 
// Input values MUST be in the range 0..63, ie, RGB666
// Output values are 16-bit in RGB565
void makePalette(uint16_t result[], 
                byte fromRed, byte fromGreen, byte fromBlue, 
                byte toRed, byte toGreen, byte toBlue, 
                int numSteps) {
  for (int ii=0; ii<=numSteps; ii++) {
    //  map(value,  frLow,frHigh, toLow, toHigh)
    int r = map(ii, 0,numSteps,   fromRed, toRed) /2;
    int g = map(ii, 0,numSteps,   fromGreen, toGreen);
    int b = map(ii, 0,numSteps,   fromBlue, toBlue) /2;

    uint16_t shade = (r << 11) | (g << 5) | (b << 0);
    result[ii] = shade;
    char msg[128];
    sprintf(msg, "shade[%d] = rgb(%d,%d,%d) = %d = 0x%x", ii, r,g,b, shade, shade);
    Serial.println(msg);
  }
  Serial.println("");
}

// ----- TOOSP: The One and Only Setup Program
void setup() {

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(eSCREEN_ROTATE_0);  // 1=landscape (default is 0=portrait)
  tft.fillScreen(ILI9341_BLACK);      // note that "begin()" does not clear screen

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);               // init for debugging in the Arduino IDE
  delay(20);  // wait for Serial
  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init Feather M4 onboard lights
  pinMode(RED_LED, OUTPUT);           // diagnostics RED LED
  digitalWrite(PIN_LED, LOW);         // turn off little red LED
  Serial.println("NeoPixel initialized and turned off");

}

void loop() {
  delay(5);  // wait for Serial
  // RGB 565 true color: https://chrishewett.com/blog/true-rgb565-colour-picker/
  // palette = makePalette(&result[], fromRed, fromGreen, fromBlue, toRed, toGreen, toBlue, numSteps);
  //                from (R  G  B)  to (R  G  B)
  //makePalette(palette, 31,62,16,     31,23,00, 64);

  int w = tft.width();
  int h = tft.height();
  /*
  tft.fillScreen(ILI9341_BLACK);
  pinwheel(0, 0, ILI9341_CYAN);
  pinwheel(w, 0, ILI9341_GREEN);
  pinwheel(w, h, ILI9341_YELLOW);
  pinwheel(0, h, ILI9341_RED);
  delay(2000);
  */

  int x0, y0;
  tft.fillScreen(ILI9341_BLACK);
  pinwheel(random(0, w/2),  random(0, h/2),  ILI9341_RED);    // ul
  pinwheel(random(w/2, w),  random(0, h/2),  ILI9341_GREEN);  // ur
  pinwheel(random(w/2, w),  random(h/2, h),  ILI9341_YELLOW); // lr
  pinwheel(random(0, w/2),  random(h/2, h),  ILI9341_CYAN);   // ll
  delay(2000);


  const int sizePallette = 40;
  const int nSeconds = 99; // OR: = sizePallette / 6;
  const int nScreens = 2;
  uint16_t palette[sizePallette];
  /*
  Serial.println("---white---");
  makePalette(palette, 0,0,0, 63,63,63, sizePallette);
  slantWaveFastV(palette, sizePallette, nSeconds, nScreens);

  Serial.println("---red---");
  makePalette(palette, 0,0,0, 63,0,0, sizePallette);
  slantWaveFastV(palette, sizePallette, nSeconds, nScreens);
  
  Serial.println("---green---");
  makePalette(palette, 0,0,0, 0,63,0, sizePallette);
  slantWaveFastV(palette, sizePallette, nSeconds, nScreens);
  
  Serial.println("---blue---");
  makePalette(palette, 0,0,0, 0,0,63, sizePallette);
  slantWaveFastV(palette, sizePallette, nSeconds, nScreens);

  Serial.println("---pinks---");
  makePalette(palette, 0,11,32, 54,11,32, sizePallette);  // from 0xd970 to 0x0170
  slantWaveFastV(palette, sizePallette, nSeconds, nScreens);
  
  Serial.println("---greens---");
  makePalette(palette, 0,8,18, 0,63,18, sizePallette);  // from 0x0fe9 to 0x0909
  slantWaveFastV(palette, sizePallette, nSeconds, nScreens);
  
  Serial.println("---yellowreds---");
  makePalette(palette, 63,9,0, 63,63,0, sizePallette);  // from 0xffe0 to 0xf920
  slantWaveFastV(palette, sizePallette, nSeconds, nScreens);
  */
  
/**** 
  slantWaveFastV(6, greens, numGreens);
  slantWaveFastV(6, yellowreds, numYellowreds);
  slantWaveFastV(6, tinyPinks, numTinyPinks);

  slantWashFastV(6, yellowreds, numYellowreds);
  slantWashFastH(6, pinks, numPinks);
  slantWash(10, greens, numGreens, 1.50);
  
  colorTunnel(8, pinks, numPinks);
  colorTunnel(8, greens, numGreens);
  colorTunnel(8, yellowreds, numYellowreds);
/* ****/
}
