/*
  Griduino -- Grid Square Tracker with GPS

  Version history:
            https://github.com/barry-ha/Griduino/blob/master/downloads/CHANGELOG.md

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is a GPS display for your vehicle's dashboard, showing
            your position in your Maidenhead Grid Square, with distances 
            to nearby squares. This is optimized for ham radio rovers. 
            Read about the Maidenhead Locator System (grid squares) 
            at https://en.wikipedia.org/wiki/Maidenhead_Locator_System

            +---------------------------------------+
            |              CN88  30.1 mi            |
            |      +-------------------------+      |
            |      |                      *  |      |
            | CN77 |       CN87              | CN97 |
            | 61.2 |                         | 37.1 |
            |      |                         |      |
            |      +-------------------------+      |
            |              CN86  39.0 mi            |
            |            47.5644, -122.0378         |
            +---------------------------------------+

  Units of Time:
         This relies on "TimeLib.h" which uses "time_t" to represent time.
         The basic unit of time (time_t) is the number of seconds since Jan 1, 1970, 
         a compact 4-byte integer.
         https://github.com/PaulStoffregen/Time
  
  Scheduling:
         This uses the simple but powerful elapsedMillis library originally written
         by Paul Stoffregen at PJRC (manufacturer of the Teensy line).
         The key is that elapsedMillis objects can be reset to zero at any time, 
         allowing the value to be directly compared with a task delay value.
         https://github.com/pfeerick/elapsedMillis/wiki
         https://electricfiredesign.com/2021/03/18/simple-multi-tasking-for-arduino/

  Real Time Clock:
         The real time clock in the Adafruit Ultimate GPS is not directly readable nor 
         accessible from the Arduino. It's definitely not writeable. It's only internal 
         to the GPS. Once the battery is installed, and the GPS gets its first data 
         reception from satellites it will set the internal RTC. Then as long as the 
         battery is installed, this program can read the time from the GPS as normal. 
         Even without a current "gps fix" the time will be correct.
         The RTC timezone cannot be changed, it is always UTC.

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)     https://www.adafruit.com/product/3857

         2. Adafruit Ultimate GPS                           https://www.adafruit.com/product/746
               Tested and works great with the Adafruit Ultimate GPS module (Mediatek MTK33x9 chipset)
         3. Quectel LC86L GPS chip on a breakout board of our own design
               L86 is an ultra compact GNSS POT (Patch on Top) module (MediaTek MT3333 chipset)
               High sensitivity: -167dBm @ Tracking, -149dBm @ Acquisition

         4. Adafruit 3.2" TFT color LCD display ILI-9341    https://www.adafruit.com/product/1743
            How to:      https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2
            SPI Wiring:  https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/spi-wiring-and-test
            Touchscreen: https://learn.adafruit.com/adafruit-2-dot-8-color-tft-touchscreen-breakout-v2/resistive-touchscreen

         5. Adafruit BMP388 Barometric Pressure, SPI        https://www.adafruit.com/product/3966
            Adafruit BMP280 Barometric Pressure, I2C        https://www.adafruit.com/product/2651
 
         5. One-chip audio amplifier, digital potentiometer and mini speaker
            Speaker is a commodity item and many devices and options are available.
            We tested a piezo speaker but they're tuned for a narrow frequency and 
            unsatisfactory for anything but a single pitch.
            Breadboard-friendly speaker:   https://www.adafruit.com/product/1898
            Better fidelity speaker:       https://www.adafruit.com/product/4445
            Micro speaker:                 https://www.digikey.com/en/products/detail/cui-devices/CES-20134-088PM/10821309

  Source Code Outline:
         1. Hardware Wiring  (pin definitions)
         2. Helper Functions (touchscreen, fonts, grids, distance, etc)
         3. Model
         4. Views
         5. Controller
         6. setup()
         7. loop()
*/

#include <Adafruit_ILI9341.h>         // TFT color display library
#include <TouchScreen.h>              // Touchscreen built in to 3.2" Adafruit TFT display
#include <Adafruit_GPS.h>             // Ultimate GPS library
#include <Adafruit_NeoPixel.h>        // On-board color addressable LED
#include <DS1804.h>                   // DS1804 digital potentiometer library
#include <elapsedMillis.h>            // Scheduling intervals in main loop
#include "save_restore.h"             // save/restore configuration data to SDRAM
#include "constants.h"                // Griduino constants, colors, typedefs
#include "hardware.h"                 // Griduino pin definitions
#include "logger.h"                   // conditional printing to Serial port
#include "grid_helper.h"              // lat/long conversion routines

#include "view.h"                     // Griduino screens base class, followed by derived classes in alphabetical order
#include "view_altimeter.h"           // altimeter
#include "view_baro.h"                // barometric pressure graph
#include "view_events.h"              // counting days to/from calendar events
#include "view_grid_crossings.h"      // list of time spent in each grid
#include "view_help.h"                // help screen
#include "view_screen1.h"             // starting screen animation
#include "view_splash.h"              // splash screen
#include "view_status.h"              // status screen 
#include "view_ten_mile_alert.h"      // microwave rover screen
#include "view_time.h"                // GMT time screen 

#include "cfg_audio_type.h"           // config audio Morse/speech
#include "cfg_crossing.h"             // config 4/6 digit crossing
#include "cfg_gps.h"                  // config GPS (alphabetical order)
#include "cfg_reboot.h"               // show firmware update option
#include "cfg_rotation.h"             // config screen rotation 
#include "cfg_units.h"                // config english/metric
#include "cfg_volume.h"               // config volume level

#if defined(SAMD_SERIES)
  #warning ----- Compiling for Arduino Feather M4 Express -----
#elif defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  #warning ----- Compiling for Arduino RP2040 -----
#else
  #error Hardware platform unknown.
#endif

// ---------- extern
extern bool newScreenTap(Point* pPoint);       // Touch.cpp
extern uint16_t myPressure(void);              // Touch.cpp
void initTouchScreen(void);                    // Touch.cpp
//extern bool TouchScreen::isTouching(void);   // Touch.cpp
//extern void mapTouchToScreen(TSPoint touch, Point* screen);
//extern void setFontSize(int font);           // TextField.cpp
void processCommand(char *cmd);                // commands.cpp

// ---------- TFT display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// to select a different SPI bus speed, use this instead:
// -----------------------------------------------
//    #include <Adafruit_SPIDevice.h>           // from library Adafruit_BusIO, file Adafruit_SPIDevice.h
//    #define SPL06_DEFAULT_SPIFREQ (1000000)   // clock speed, default is spi_dev =
//    Adafruit_SPIDevice mySPI = 
//        Adafruit_SPIDevice(TFT_CS,                  // chip select
//                           SPL06_DEFAULT_SPIFREQ,   // frequency
//                           SPI_BITORDER_MSBFIRST,   // bit order
//                           SPI_MODE0,               // data mode
//                           theSPI);
//    if (!spi_dev->begin()) {
//      return false;
//    }
//    Adafruit_ILI9341(mySPI, TFT_DC, TFT_CS);
// -----------------------------------------------

// Hardware serial port for USB
Logger logger = Logger();

// Hardware serial port for GPS
Adafruit_GPS GPS(&Serial1);           // https://github.com/adafruit/Adafruit_GPS
/* "Ultimate GPS" pin wiring is connected to a dedicated hardware serial port
    available on an Arduino Mega, Arduino Feather and others.

    The "Ultimate GPS" LED indicates status:
        1-sec blink = searching for satellites
        15-sec blink = position fix found
*/

// ---------- config settings
// these are controllable by serial USB commands (command.h)
bool showTouchTargets = false;

// ---------- lat/long and date/time conversion utilities
Grids grid = Grids();
Dates date = Dates();

// ---------- Neopixel
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// ---------- Digital potentiometer
// ctor         DS1804( ChipSel pin, Incr pin,  U/D pin,  maxResistance (K) )
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  // todo - for now, RP2040 has no DAC, no volume control, no audio output
#else
  DS1804 volume = DS1804( PIN_VCS, PIN_VINC, PIN_VUD, DS1804_TEN );
#endif

int gWiper = 15;                      // initial digital potentiometer wiper position, 0..99
int gFrequency = 1100;                // initial Morse code sidetone pitch
int gWordsPerMinute = 18;             // initial Morse code sending speed

// ------------ definitions
const int howLongToWait = 6;          // max number of seconds at startup waiting for Serial port to console

// ---------- Morse Code ----------
#include "morse_dac.h"                // Morse Code using digital-audio converter DAC0
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  // todo - for now, RP2040 has no DAC, no speech, no audio output
#else
  DACMorseSender dacMorse(DAC_PIN, gFrequency, gWordsPerMinute);
#endif

// ----------- Speech PCM Audio Playback
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  // todo - for now, RP2040 has no DAC, no speech, no audio output
#else
  #include <Audio_QSPI.h>               // Audio playback library for Arduino
                                        // https://github.com/barry-ha/Audio_QSPI
  AudioQSPI dacSpeech;
#endif

// 2. Helper Functions
// ============== Touchable spots on all screens ===============
Rect areaGear { {0,                  0},  {gScreenWidth * 4/10, gScreenHeight * 5/10}};
Rect areaArrow{ {gScreenWidth *5/10, 0},  {gScreenWidth * 5/10, gScreenHeight * 5/10}};
Rect areaBrite{ {0, gScreenHeight *2/3},  {gScreenWidth,       (gScreenHeight * 1/3)-1}};

void showDefaultTouchTargets() {
  if (showTouchTargets) {
    tft.drawRect(areaGear.ul.x,areaGear.ul.y,   areaGear.size.x, areaGear.size.y,  ILI9341_MAGENTA);
    tft.drawRect(areaArrow.ul.x,areaArrow.ul.y, areaArrow.size.x,areaArrow.size.y, ILI9341_MAGENTA);
    tft.drawRect(areaBrite.ul.x,areaBrite.ul.y, areaBrite.size.x,areaBrite.size.y, ILI9341_MAGENTA);
  }
}

void showWhereTouched(Point touch) {
  if (showTouchTargets) {
    const int radius = 1;     // debug
    tft.fillCircle(touch.x, touch.y, radius, cTOUCHTARGET);  // debug - show dot
  }
}

// ============== helpers ======================================
void floatToCharArray(char* result, int maxlen, double fValue, int decimalPlaces) {
  String temp = String(fValue, decimalPlaces);
  temp.toCharArray(result, maxlen);
}

//==============================================================
//
//    GPS Model
//    This is MVC (model-view-controller) design pattern
//
//    This model collects data from the GPS sensor
//    on a schedule determined by the Controller.
//
//    The model knows about:
//    - current grid info, such as lat/long and grid square name
//    - nearby grids and distances
//    - when we enter a new grid
//    - when we lose GPS signal
//
//    We use "class" instead of the usual collection of random subroutines 
//    to help guide the programmer into designing an independent Model object 
//    with very specific functionality and interfaces.
//==============================================================
#include "model_gps.h"                // Model of a GPS for model-view-controller

// create an instance of the GPS model
Model modelGPS;                       // normal: use real GPS hardware
MockModel modelSimulator;             // test: simulated travel (see model_gps.h)

// at power-on, we choose to always start with real GPS receiver hardware 
// because I don't want to bother saving/restoring this selection right now
Model* model = &modelGPS;

void fSetReceiver() {
  model = &modelGPS;                  // use "class Model" for GPS receiver hardware
}
void fSetSimulated() {
  model = &modelSimulator;            // use "class MockModel" for simulated track
}
int fGetDataSource() {
  // this function allows the user interface to display which one is active
  // returns: enum
  if (model == &modelGPS) {
    return eGPSRECEIVER;
  } else {
    return eGPSSIMULATOR;
  }
}

bool waitingForRTC = true;   // true=waiting for GPS hardware to give us the first valid date/time

//==============================================================
//    Breadcrumb Trail model
//==============================================================
#include "model_breadcrumbs.h"
Breadcrumbs trail;

//==============================================================
//    Coin Battery Voltage model
//==============================================================
// PCB v7 added a sensor for coin battery voltage
// PCB v4 doesn't measure coin battery
// The hardware differences are handled in lower level code
#include "model_adc.h"                // Model of the analog-digital converter
BatteryVoltage gpsBattery;

//==============================================================
//
//      BarometerModel
//      "Class BarometerModel" is intended to be identical 
//      for both Griduino and the Barograph example
//
//    This model collects data from the BMP280 or BMP388 barometric pressure 
//    and temperature sensors on a schedule determined by the Controller.
//
//    288px wide graph ==> 96 px/day ==> 4px/hour ==> log pressure every 15 minutes
//
//==============================================================

#include "model_baro.h"     // a barometer that can also store history
BarometerModel baroModel;   // create instance of the model

//==============================================================
//
//      Views
//      This is MVC (model-view-controller) design pattern
//
//    "startXxxxScreen" is one-time setup of visual elements that never change
//    "updateXxxxScreen" is dynamic and displays things that change over time
//==============================================================

// alias names for all views - MUST be in same order as "viewTable" array below, alphabetical by class name
enum VIEW_INDEX {
  ALTIMETER_VIEW = 0,        // altimeter
  BARO_VIEW,             // barometer graph
  CFG_AUDIO_TYPE,        // audio output Morse/speech
  CFG_CROSSING,          // announce grid crossing 4/6 digit boundaries
  CFG_GPS,               // gps/simulator
  CFG_REBOOT,            // confirm reboot
  CFG_ROTATION,          // screen rotation
  CFG_UNITS,             // english/metric
  EVENTS_VIEW,           // Groundhog Day, Halloween, or other day-counting screen
  GRID_VIEW,
  GRID_CROSSINGS_VIEW,   // log of time in each grid
  HELP_VIEW,             // hints at startup
  SCREEN1_VIEW,          // first bootup screen
  SPLASH_VIEW,           // startup
  STATUS_VIEW,           // size and scale of this grid
  TEN_MILE_ALERT_VIEW,   // microwave rover view
  TIME_VIEW,             //
  CFG_VOLUME,            //
  GOTO_SETTINGS,         // command the state machine to show control panel
  GOTO_NEXT_VIEW,        // command the state machine to show next screen
  MAX_VIEWS,             // sentinel at end of list
};
/*const*/ int help_view      = HELP_VIEW;
/*const*/ int splash_view    = SPLASH_VIEW;
/*const*/ int screen1_view   = SCREEN1_VIEW;
/*const*/ int grid_view      = GRID_VIEW;
/*const*/ int grid_crossings_view = GRID_CROSSINGS_VIEW;
/*const*/ int events_view    = EVENTS_VIEW;
/*const*/ int goto_next_view = GOTO_NEXT_VIEW;
/*const*/ int goto_next_cfg  = GOTO_SETTINGS;

// list of objects derived from "class View", in alphabetical order
// clang-format off
View* pView;      // pointer to a derived class
// vvv sort vvv
ViewAltimeter     altimeterView(&tft, ALTIMETER_VIEW);  // alphabetical order by class name
ViewBaro          baroView(&tft, BARO_VIEW);            // instantiate derived classes
ViewCfgAudioType  cfgAudioType(&tft, CFG_AUDIO_TYPE);
ViewCfgCrossing   cfgCrossing(&tft, CFG_CROSSING);
ViewCfgGPS        cfgGPS(&tft, CFG_GPS);
ViewCfgReboot     cfgReboot(&tft, CFG_REBOOT);
ViewCfgRotation   cfgRotation(&tft, CFG_ROTATION);
ViewCfgUnits      cfgUnits(&tft, CFG_UNITS);
ViewEvents        eventsView(&tft, EVENTS_VIEW);
ViewGrid          gridView(&tft, GRID_VIEW);
ViewGridCrossings gridCrossingsView(&tft, GRID_CROSSINGS_VIEW);
ViewHelp          helpView(&tft, HELP_VIEW);
ViewScreen1       screen1View(&tft, SCREEN1_VIEW);
ViewSplash        splashView(&tft, SPLASH_VIEW);
ViewStatus        statusView(&tft, STATUS_VIEW);
ViewTenMileAlert  tenMileAlertView(&tft, TEN_MILE_ALERT_VIEW);
ViewTime          timeView(&tft, TIME_VIEW);
ViewVolume        volumeView(&tft, CFG_VOLUME);
// clang-format on

void selectNewView(int cmd) {
  // cmd = GOTO_NEXT_VIEW | GOTO_SETTINGS
  // this is a state machine to select next view, given current view and type of command
  View *viewTable[] = {
      // vvv same order as enum vvv
      &altimeterView,       // [ALTIMETER_VIEW]
      &baroView,            // [BARO_VIEW]
      &cfgAudioType,        // [CFG_AUDIO_TYPE]
      &cfgCrossing,         // [CFG_CROSSING]
      &cfgGPS,              // [CFG_GPS]
      &cfgReboot,           // [CFG_REBOOT]
      &cfgRotation,         // [CFG_ROTATION]
      &cfgUnits,            // [CFG_UNITS]
      &eventsView,          // [EVENTS_VIEW]
      &gridView,            // [GRID_VIEW]
      &gridCrossingsView,   // [GRID_CROSSINGS_VIEW]
      &helpView,            // [HELP_VIEW]
      &screen1View,         // [SCREEN1_VIEW]
      &splashView,          // [SPLASH_VIEW]
      &statusView,          // [STATUS_VIEW]
      &tenMileAlertView,    // [TEN_MILE_ALERT_VIEW]
      &timeView,            // [TIME_VIEW]
      &volumeView,          // [CFG_VOLUME]
  };

  int currentView = pView->screenID;
  int nextView    = BARO_VIEW;   // GRID_VIEW;       // default
  // clang-format off
  if (cmd == GOTO_NEXT_VIEW) {
    // operator requested the next NORMAL user view
    switch (currentView) {
      case SCREEN1_VIEW:   nextView = HELP_VIEW; break;   // skip SPLASH_VIEW (simplify startup, now that animated logo shows version number)
      case SPLASH_VIEW:    nextView = GRID_VIEW; break;
      case GRID_VIEW:      nextView = TIME_VIEW; break;
      case GRID_CROSSINGS_VIEW: nextView= GRID_VIEW; break;   // skip GRID_CROSSINGS_VIEW (not ready for prime time)
      case TIME_VIEW:      nextView = BARO_VIEW; break;
      case BARO_VIEW:      nextView = ALTIMETER_VIEW; break;
      case ALTIMETER_VIEW: nextView = STATUS_VIEW; break;
      case STATUS_VIEW:    nextView = TEN_MILE_ALERT_VIEW; break;
      case TEN_MILE_ALERT_VIEW: nextView = GRID_VIEW; break;
      case EVENTS_VIEW:    nextView = GRID_VIEW; break;   // skip EVENTS_VIEW (nobody uses it)
      // none of above: we must be showing some settings view, so go to the first normal user view
      default:             nextView = GRID_VIEW; break;
    }
  } else if (cmd == GOTO_SETTINGS) {
    // operator requested the next SETTINGS view
    switch (currentView) {
      case CFG_VOLUME:     nextView = CFG_AUDIO_TYPE; break;
      //se VOLUME2_VIEW:   nextView = CFG_AUDIO_TYPE; break;
      case CFG_AUDIO_TYPE: nextView = CFG_CROSSING; break;
      case CFG_CROSSING:   nextView = CFG_GPS; break;
      case CFG_GPS:        nextView = CFG_UNITS; break;
      case CFG_UNITS:      nextView = CFG_ROTATION; break;
      case CFG_ROTATION:   nextView = CFG_REBOOT; break;
      case CFG_REBOOT:     nextView = CFG_VOLUME; break;
      // none of above: we must be showing some normal user view, so go to the first settings view
      default:             nextView = CFG_VOLUME; break;
    }
  } else if (cmd < MAX_VIEWS) {
    // a specific view was requested, such as HELP_VIEW via a USB command
    nextView = cmd;
  } else {
    logger.error("Requested view was out of range: %d where maximum is %d", cmd, MAX_VIEWS);
  }
  // clang-format on
  logger.info("selectNewView() from %d to %d", currentView, nextView);
  pView->endScreen();                   // a goodbye-kiss to the departing view
  pView = viewTable[ nextView ];

  // Every view has an initial setup to prepare its layout
  // After initial setup the view can assume it "owns" the screen
  // and can safely repaint only the parts that change
  pView->startScreen();
  pView->updateScreen();
}
// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connection to the PC
  // and the operator takes awhile to restart the IDE console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE

  pView = &screen1View;
  screen1View.startScreen();
  while (pView == &screen1View) {
    screen1View.updateScreen();
  }  
}

//==============================================================
//
//      Controller
//      This is MVC (model-view-controller) design pattern
//
//==============================================================
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
// ----- adjust screen brightness: Feather RP2040
const int gNumLevels               = 3;
const int gaBrightness[gNumLevels] = {100, 50, 15};   // global array of preselected brightness
int gCurrentBrightnessIndex        = 0;               // current brightness
float frequency                    = 10000;
float dutyCycle                    = 100;

#include "RP2040_PWM.h"
RP2040_PWM PWM_backlight           = RP2040_PWM(TFT_BL, frequency, dutyCycle);

void adjustBrightness() {
  // increment display brightness
  gCurrentBrightnessIndex = (gCurrentBrightnessIndex + 1) % gNumLevels;   // incr index
  int brightness          = gaBrightness[gCurrentBrightnessIndex];        // look up brightness
  PWM_backlight.setPWM(TFT_BL, frequency, brightness);                    // write to hardware
  logger.info("Set brightness %d", brightness);
}
#else
// ----- adjust screen brightness: Feather M4
const int gNumLevels               = 3;
const int gaBrightness[gNumLevels] = {255, 80, 20};   // global array of preselected brightness
int gCurrentBrightnessIndex        = 0;               // current brightness

void adjustBrightness() {
  // increment display brightness
  gCurrentBrightnessIndex = (gCurrentBrightnessIndex + 1) % gNumLevels;   // incr index
  int brightness          = gaBrightness[gCurrentBrightnessIndex];        // look up brightness
  analogWrite(TFT_BL, brightness);                                        // write to hardware
  logger.info("Set brightness %d", brightness);
}
#endif

void sendMorseLostSignal() {
  // commented out -- this occurs too frequently and is distracting
  return;

  //String msg(PROSIGN_AS);             // "wait" symbol
  //dacMorse.setMessage(msg);
  //dacMorse.sendBlocking();            // TODO - use non-blocking
}

void announceGrid(const String gridName, int length) {
  // todo: change "String" class parameter into "char*" type
  char grid[7];
  strncpy(grid, gridName.c_str(), sizeof(grid));
  grid[length] = 0;   // null-terminate string to requested 4- or 6-character length
  Serial.print("Announcing grid: "); Serial.println(grid);

#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  // todo - for now, RP2040 has no DAC, no audio, no speech
  logger.error("Unsupported audio in line ", __LINE__ );
#else
  switch (cfgAudioType.selectedAudio) {
    case ViewCfgAudioType::MORSE: 
      logger.fencepost("Griduino.ino", __LINE__);   // debug
      sendMorseGrid6( grid );
      break;
    case ViewCfgAudioType::SPEECH:
      for (int ii=0; ii<strlen(grid); ii++) {

        char myfile[32];
        char letter = tolower( grid[ii] );
        logger.fencepost("Griduino.ino speech", __LINE__);   // debug
        snprintf(myfile, sizeof(myfile), "/audio/%c.wav", letter);

        if (!dacSpeech.play( myfile )) {
          // indicate error playing WAV file (probably "file not found")
          sendMorseGrid6("i");
        }
      }
      break;
    case ViewCfgAudioType::NO_AUDIO:
      // do nothing
      break;
    default:
      // should not happen
      logger.error("Internal error: unsupported audio, line ", __LINE__);
      break;
  }
#endif
}

void sendMorseGrid4(const String gridName) {
  // announce new grid by Morse code
  logger.fencepost("Griduino.ino", __LINE__);   // debug
  String grid4 = gridName.substring(0, 4);
  sendMorseGrid6( grid4 );
}

void sendMorseGrid6(const String gridName) {
  // announce new grid by Morse code
  logger.fencepost("Griduino.ino", __LINE__);   // debug
  String grid = gridName;
  grid.toLowerCase();

  dacMorse.setMessage( grid );
  dacMorse.sendBlocking();
  //spkrMorse.setMessage( grid );
  //spkrMorse.startSending();   // would prefer non-blocking but some bug causes random dashes to be too long
}

void showActivityBar(int row, uint16_t foreground, uint16_t background) {
  static int addDotX = 10;                    // current screen column, 0..319 pixels
  static int rmvDotX = 0;
  static int count = 0;
  const int SCALEF = 32;                      // how much to slow it down so it becomes visible

  count = (count + 1) % SCALEF;
  if (count == 0) {
    addDotX = (addDotX + 1) % gScreenWidth;   // advance
    rmvDotX = (rmvDotX + 1) % gScreenWidth;   // advance
    tft.drawPixel(addDotX, row, foreground);  // write new
    tft.drawPixel(rmvDotX, row, background);  // erase old
  }
}

void sayGrid(const char *name) {
  logger.fencepost("Griduino.ino say", __LINE__);   // debug
  Serial.print("Say ");
  Serial.println(name);

#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  // todo - for now, RP2040 has no DAC, no audio, no speech
  logger.error("Unsupported audio in line ", __LINE__ );
#else
  for (int ii = 0; ii < strlen(name); ii++) {
    logger.fencepost("Griduino.ino", __LINE__);   // debug

    // example: choose the filename to play
    char myfile[32];
    char letter = name[ii];
    snprintf(myfile, sizeof(myfile), "/audio/%c.wav", letter);

    // example: read WAV attributes and display it on screen while playing it
    WaveInfo info;
    dacSpeech.getInfo(&info, myfile);
    //showWaveInfo(info);             // debug

    // example: play audio through DAC
    bool rc = dacSpeech.play(myfile);
    if (!rc) {
      Serial.print("sayGrid(");
      Serial.print(letter);
      Serial.println(") failed");
    }
  }
#endif
}

//=========== setup ============================================
void setup() {
 
  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(eSCREEN_ROTATE_0);  // 1=landscape (default is 0=portrait)
  tft.fillScreen(ILI9341_BLACK);      // note that "begin()" does not clear screen

  // ----- init TFT backlight
 // pinMode(A1, INPUT);                 // Griduino PCB v7 uses pin A1 (ADC1) to measure 3v coin battery; don't load down the pin
 
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);           // start at full brightness

  // ----- init screen orientation
  Serial.println("Starting cfgRotation.loadConfig()...");
  cfgRotation.loadConfig();           // restore previous screen orientation

  // ----- init touch screen
  void initTouchScreen();

  // ----- init serial monitor (do not "Serial.print" before this, it won't show up in console)
  Serial.begin(115200);               // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);       // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name

  // ----- init Feather M4 onboard lights
  pixel.begin();
  pixel.clear();                      // turn off NeoPixel
  pinMode(RED_LED, OUTPUT);           // diagnostics RED LED
  digitalWrite(PIN_LED, LOW);         // turn off little red LED
  Serial.println("NeoPixel initialized and turned off");

  // ----- init GPS
  GPS.begin(9600);   // 9600 NMEA is the default baud rate for Adafruit MTK GPS's
  delay(50);         // is delay really needed?

  Serial.print("Set GPS baud rate to 57600: ");
  Serial.println(PMTK_SET_BAUD_57600);
  GPS.sendCommand(PMTK_SET_BAUD_57600);
  delay(50);
  GPS.begin(57600);
  delay(50);

  // init Quectel L86 chip to improve USA satellite acquisition
  GPS.sendCommand("$PMTK353,1,0,0,0,0*2A");   // search American GPS satellites only (not Russian GLONASS satellites)
  delay(50);

  Serial.print("Turn on RMC (recommended minimum) and GGA (fix data) including altitude: ");
  Serial.println(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  delay(50);

  Serial.print("Set GPS 1 Hz update rate: ");
  Serial.println(PMTK_SET_NMEA_UPDATE_1HZ);
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);              // 5 Hz update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);                // 1 Hz
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ);   // Every 5 seconds
  delay(50);

  if (0) {   // this command is saved in the GPS chip NVR, so always send one of these cmds
    Serial.print("Request antenna status: ");
    Serial.println(PGCMD_ANTENNA);    // echo command to console log
    GPS.sendCommand(PGCMD_ANTENNA);   // tell GPS to send us antenna status
                                      // expected reply: $PGTOP,11,...
  } else {
    Serial.print("Request to NOT send antenna status: ");
    Serial.println(PGCMD_NOANTENNA);    // echo command to console log
    GPS.sendCommand(PGCMD_NOANTENNA);   // tell GPS to NOT send us antena status
  }
  delay(50);

  // ----- query GPS firmware
  Serial.print("Query GPS Firmware version: ");
  Serial.println(PMTK_Q_RELEASE);    // Echo query to console
  GPS.sendCommand(PMTK_Q_RELEASE);   // Send query to GPS unit
                                     // expected reply: $PMTK705,AXN_2.10...
  delay(50);

// ----- turn on additional satellite reporting to support NMEATime2
// You can tinker with this in sandbox: \Griduino\examples\GPS_Demo_Loopback\GPS_Demo_Loopback.ino
//                                          GPGLL           Geographic Latitude longitude
//                                          | GPRMC         Recommended Minimum Coordinates
//                                          | | GPVTG       Velocity Over Ground
//                                          | | | GPGGA     GPS Fix Data
//                                          | | | | GPGSA   GPS Satellites Active
//                                          | | | | | GPGSV GPS Satellites in View
#define PMTK_SENTENCE_FREQUENCIES "$PMTK314,0,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
  Serial.print("Set sentence output frequencies: ");
  Serial.println(PMTK_SENTENCE_FREQUENCIES);    // Echo command to console
  GPS.sendCommand(PMTK_SENTENCE_FREQUENCIES);   // Send command to GPS unit
  delay(50);

  // ----- report on our memory hogs
  char temp[200];
  Serial.println("Large resources:");
  snprintf(temp, sizeof(temp), 
          ". Model.history[%d] uses %d bytes/entry = %d bytes total",
             trail.capacity, trail.recordSize, trail.totalSize);
  Serial.println(temp);
  snprintf(temp, sizeof(temp),
          ". baroModel.pressureStack[%d] uses %d bytes/entry = %d bytes total",
             maxReadings, sizeof(BaroReading), sizeof(baroModel.pressureStack));
  Serial.println(temp);

  // ----- init RTC
  // Note: See the main() loop. 
  //       The realtime clock is not available until after receiving a few NMEA sentences.

  // ----- init digital potentiometer, restore volume setting
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  // todo - Griduino PCB v7 has I2C volume control
#else
  // Griduino PCB v4 has DS1804 volume control using SPI
  pinMode(PIN_VCS, OUTPUT);           // fix bug that somehow forgets this is an output pin

  volume.unlock();                    // unlock digipot (in case someone else, like an example pgm, has locked it)
  volume.setToZero();                 // set digipot hardware to match its ctor (wiper=0) because the chip cannot be read
                                      // and all "setWiper" commands are really incr/decr pulses. This gets it sync.
  volume.setWiperPosition( gWiper );  // set default volume in digital pot
  volumeView.loadConfig();            // restore volume setting from non-volatile RAM
#endif
  cfgAudioType.loadConfig();          // restore Morse-vs-Speech setting from non-volatile RAM

  // ----- init DAC for audio/morse code
  #if defined(SAMD_SERIES)
    // Only set DAC resolution on devices that have a DAC
    analogWriteResolution(12);        // 1..32, sets DAC output resolution to 12 bit (4096 levels)
                                      // because Feather M4 maximum output resolution is 12 bit
    dacMorse.setup();                 // required Morse Code initialization
    dacMorse.dump();                  // debug
  
    dacSpeech.begin();                // required Audio_QSPI initialization
    //sayGrid("k7bwh");               // debug test 
  #endif

  // ----- init onboard LED
  pinMode(RED_LED, OUTPUT);           // diagnostics RED LED

  // ----- restore GPS driving track breadcrumb trail
  logger.fencepost("Griduino.ino restore",__LINE__);  // debug
  trail.restoreGPSBreadcrumbTrail();  // this takes noticeable time (~0.2 sec)
  model->restore();                   //
  model->gHaveGPSfix = false;         // assume no satellite signal yet
  model->gSatellites = 0;
  trail.rememberPUP();                // log a "power up" event
  logger.fencepost("Griduino.ino save after restore",__LINE__);  // debug
  trail.saveGPSBreadcrumbTrail();     // ensure its saved for posterity

  // ----- restore barometric pressure log
  if (baroModel.loadHistory()) {
    logger.info("Successfully restored barometric pressure log");
  } else {
    logger.error("Failed to load barometric pressure log, re-initializing config file");
    baroModel.saveHistory();
  }

  // ----- init barometer
  if (baroModel.begin()) {
    // success
  } else {
    // failed to initialize hardware
    tft.fillScreen(cBACKGROUND);
    tft.setCursor(0, 48);
    tft.setTextColor(cWARN);
    setFontSize(12);
    tft.println("Error!\n Unable to init\n  barometric sensor\n   check wiring");
    delay(5000);
  }

  // ----- all done with setup, show opening view screen
  pView = &gridView;
  pView->startScreen();               // start current view
  pView->updateScreen();              // update current view
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
uint32_t prevTimeGPS = millis();
uint32_t prevTimeBaro = millis();
//uint32_t prevTimeMorse = millis();
uint32_t prevCheckRTC = 0;            // timer to update time-of-day (1 second)
time_t prevTimeRTC = 0;               // timer to print RTC to serial port (1 second)
elapsedMillis autoLogTimer;           // timer to save GPS trail periodically no matter what

//time_t nextShowPressure = 0;        // timer to update displayed value (5 min), init to take a reading soon after startup
time_t nextSavePressure = 0;          // timer to log pressure reading (15 min)

// GPS_PROCESS_INTERVAL is how frequently to update the model from GPS data.
// When the model detects a change, such as updated minutes or seconds, it will
// trigger a display update. The interval should be short (50 msec) to keep the
// displayed GMT clock in close match with WWV. But very short intervals will
// make our displayed colon ":" flicker. We chose 47 msec as a compromise, allowing
// an almost-unnoticeable flicker and an almost-unnoticeable difference from WWV.
// Also, 47 msec is relatively prime compared to 200 msec (5 Hz) updates sent from
// the GPS hardware. Todo - fix the colon's flicker then reduce this interval to 10 msec.
const int GPS_PROCESS_INTERVAL =  47;   // milliseconds between updating the model's GPS data
const int RTC_PROCESS_INTERVAL = 1000;          // Timer RTC = 1 second
const uint32_t GPS_AUTOSAVE_INTERVAL = SECS_PER_10MIN * 1000; // msec between saving breadcrumb trail to file
//const int BAROMETRIC_PROCESS_INTERVAL = 15*60*1000;  // fifteen minutes in milliseconds
const int LOG_PRESSURE_INTERVAL = 15*60*1000;   // 15 minutes, in milliseconds

void loop() {

  // if our timer or system millis() wrapped around, reset it
  if (prevTimeGPS > millis()) {
    prevTimeGPS = millis();
  }
  //if (prevTimeMorse > millis()) {
  //  prevTimeMorse = millis();
  //}

  GPS.read();   // if you can, read the GPS serial port every millisecond

  if (GPS.newNMEAreceived()) {
    // optionally send NMEA sentences to Serial port, possibly for NMEATime2
    // Note: Adafruit parser doesn't handle $GPGSV (satellites in vieW) so we send all sentences regardless of content
    logger.nmea(GPS.lastNMEA());

    // sentence received -- verify checksum, parse it
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and thereby might miss other sentences
    // so be very wary if using OUTPUT_ALLDATA, which generates loads of sentences,
    // since trying to print them all out is time-consuming
    // GPS parsing: https://learn.adafruit.com/adafruit-ultimate-gps/parsed-data-output
    if (!GPS.parse(GPS.lastNMEA())) {
      // parsing failed -- restart main loop to wait for another sentence
      // this also sets the newNMEAreceived() flag to false
      return;
    }
  }

  // look for the first "setTime()" to begin the datalogger
  if (waitingForRTC && date.isDateValid(GPS.year, GPS.month, GPS.day)) {
    // found a transition from an unknown date -> correct date/time
    // assuming "class Adafruit_GPS" contains 2000-01-01 00:00 until 
    // it receives an update via NMEA sentences
    // the next step (1 second timer) will actually set the clock
    waitingForRTC = false;

    char msg[128];                    // debug
    snprintf(msg, sizeof(msg), "Received first valid GPS time: %d-%02d-%02d at %02d:%02d:%02d",
                                GPS.year,GPS.month,GPS.day, 
                                GPS.hour,GPS.minute,GPS.seconds);
    Serial.println(msg);              // debug

    // write this to the breadcrumb trail as preliminary indication of acquiring satellites
    TimeElements tm{GPS.seconds, GPS.minute, GPS.hour, 0, GPS.day, GPS.month, (byte)(2000-1970+GPS.year)};
    time_t firstTime = makeTime(tm);
    trail.rememberFirstValidTime(firstTime, GPS.satellites);
    trail.saveGPSBreadcrumbTrail();   // ensure its saved for posterity
  }

  // every 1 second update the realtime clock
  if (millis() - prevCheckRTC > RTC_PROCESS_INTERVAL) {
    prevCheckRTC = millis();

    // update RTC from GPS
    if (date.isDateValid(GPS.year, GPS.month, GPS.day)) {
      
      setTime(GPS.hour, GPS.minute, GPS.seconds, GPS.day, GPS.month, GPS.year);
      //adjustTime(offset * SECS_PER_HOUR);  // todo - adjust to local time zone. for now, we only do GMT
    }

  }

  // send RTC to a (possible) Windows program, e.g. https://github.com/barry-ha/Laptop-Griduino
  // as GMT using ISO date/time formatted string: YYYY-MM-DD[*HH[:MM[:SS[.fff[fff]]]][+HH:MM[:SS[.ffffff]]]]
  // e.g. 2022-01-02 12:34+00:00
  // Note: do not use the GPS clock, since it might not have sat lock and therefore no reports
  time_t rtc = now();
  if (second(rtc) != second(prevTimeRTC)) {    // if RTC has ticked over to the next second
    char msg[32];      // strlen("2022-01-01 21:49:49+00:00") = 25
    snprintf(msg, sizeof(msg), "%04d-%02d-%02d %02d:%02d:%02d+00:00\n",
                         year(rtc), month(rtc), day(rtc), hour(rtc), minute(rtc), second(rtc));
    logger.gmt(msg);
    prevTimeRTC = rtc;
  }
    
  // every 15 minutes read barometric pressure and save it in nonvolatile RAM
  // synchronize showReadings() on exactly 15-minute marks 
  // so the user can more easily predict when the next update will occur
  time_t rightnow = now();
  if ( rightnow >= nextSavePressure) {
    
    // log this pressure reading only if the time-of-day is correct and initialized 
    if (timeStatus() == timeSet) {
      baroModel.logPressure( rightnow );
      //redrawGraph = true;             // request draw graph
      nextSavePressure = date.nextFifteenMinuteMark( rightnow ); // production
      //nextSavePressure = date.nextOneMinuteMark( rightnow );   // debug
    }
  }

  // periodically, ask the model to process and save the current GPS location
  if (millis() - prevTimeGPS > GPS_PROCESS_INTERVAL) {
    prevTimeGPS = millis();           // restart another interval

    model->processGPS();               // update model

    // update View - call the current viewing function
    pView->updateScreen();             // update current view, eg, updateGridScreen()
    //gaUpdateView[gViewIndex](); 
  }

  //if (!spkrMorse.continueSending()) {
  //  // give processing time to SpeakerMorseSender component
  //  // "continueSending" returns false after the message finishes sending
  //}

  // if there's an alert, tell the user
  if (model->signalLost()) {
    model->indicateSignalLost();   // update model
    sendMorseLostSignal();         // announce GPS signal lost by Morse code
    // the above will also write LOS and AOS to gps history log
  }

  // if GPS enters a new grid, notify the user and draw new display screen
  char newGrid6[7];
  grid.calcLocator(newGrid6, model->gLatitude, model->gLongitude, 6);

  if (model->enteredNewGrid4()) {
    pView->startScreen();          // update display so they can see new grid while listening to audible announcement
    pView->updateScreen();
    announceGrid(newGrid6, 4);     // announce with Morse code or speech, according to user's config

    Location whereAmI;
    model->makeLocation(&whereAmI);
    trail.rememberGPS(whereAmI);
    logger.fencepost("Griduino.ino new grid4",__LINE__);  // debug
    trail.saveGPSBreadcrumbTrail();

  } else if (model->enteredNewGrid6()) {
    if (!model->compare4digits) {
      announceGrid(newGrid6, 6);   // announce with Morse code or speech, according to user's config
    }
    Location whereAmI;
    model->makeLocation(&whereAmI);
    trail.rememberGPS(whereAmI);    // when we enter a new 6-digit grid, save it in breadcrumb trail
    logger.fencepost("Griduino.ino new grid6",__LINE__);  // debug
    trail.saveGPSBreadcrumbTrail(); // because one user's home was barely in the next grid6
                                    // and we want to show his grid6 at next power up
  }

  // if we drove far enough, add this to the breadcrumb trail
  static PointGPS prevRememberedGPS{0.0, 0.0};
  PointGPS currentGPS{model->gLatitude, model->gLongitude};
  if (grid.isVisibleDistance(prevRememberedGPS, currentGPS)) {
    Location whereAmI;
    model->makeLocation(&whereAmI);
    trail.rememberGPS(whereAmI);
    prevRememberedGPS = currentGPS;

    if (0 == (trail.getHistoryCount() % trail.saveInterval)) {
      trail.saveGPSBreadcrumbTrail();
    }
  }

  // log GPS position every few minutes, to keep track of lingering in one spot
  if (autoLogTimer > GPS_AUTOSAVE_INTERVAL) {
    autoLogTimer = 0;

    Location whereAmI;
    model->makeLocation(&whereAmI);
    //logger.fencepost("Griduino.ino autolog timer",__LINE__);  // debug
    //whereAmI.printLocation();                                 // debug
    trail.rememberGPS(whereAmI);
    trail.saveGPSBreadcrumbTrail();
  }

  // if there's touchscreen input, handle it
  Point touch;
  if (newScreenTap(&touch)) {

    if (showTouchTargets) {
      showWhereTouched(touch);   // debug: show where touched
    }

    bool touchHandled = pView->onTouch(touch);

    if (!touchHandled) {
      // not handled by one of the views, so run our default action
      if (areaGear.contains(touch)) {
        selectNewView(GOTO_SETTINGS);   // advance to next settings view
      } else if (areaArrow.contains(touch)) {
        selectNewView(GOTO_NEXT_VIEW);   // advance to next normal user view
      } else if (areaBrite.contains(touch)) {
        adjustBrightness();   // change brightness
      } else {
        // nothing to do
      }
    }
  }

  // if there's text from the USB port, handle it
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');

    char cmd[24];                            // convert "String" type to character array
    command.toCharArray(cmd, sizeof(cmd));   // because it's generally safer programming
    processCommand(cmd);
  }

  // small activity bar crawls along bottom edge to give 
  // a sense of how frequently the main loop is executing
  showActivityBar(tft.height()-1, ILI9341_RED, pView->background);
}
