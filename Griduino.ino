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
#include <Resistive_Touch_Screen.h>   // Touchscreen built in to 3.2" Adafruit TFT display
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
#include "view_battery.h"             // coin battery
#include "view_baro.h"                // barometric pressure graph
#include "view_events.h"              // counting days to/from calendar events
#include "view_grid_crossings.h"      // list of time spent in each grid
#include "view_help.h"                // help screen
#include "view_sat_count.h"           // show number of satellites acquired
#include "view_screen1.h"             // starting screen animation
#include "view_splash.h"              // splash screen
#include "view_status.h"              // status screen 
#include "view_ten_mile_alert.h"      // microwave rover screen
#include "view_time.h"                // GMT time screen 

#include "cfg_audio_type.h"           // config audio Morse/speech
#include "cfg_crossing.h"             // config 4/6 digit crossing
#include "cfg_gps.h"                  // config GPS (alphabetical order)
#include "cfg_gps_reset.h"            // config GPS reset to factory
#include "cfg_nmea.h"                 // config NMEA broadcasting
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
//extern bool newScreenTap(Point* pPoint);       // Touch.cpp
//extern uint16_t myPressure(void);              // Touch.cpp
//void initTouchScreen(void);                    // Touch.cpp
//extern void setFontSize(int font);           // TextField.cpp
void processCommand(char *cmd);                // commands.cpp

// ---------- TFT display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- Touch Screen
Resistive_Touch_Screen tsn(PIN_XP, PIN_YP, PIN_XM, PIN_YM, XP_XM_OHMS);

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
bool showCenterline   = false;

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

void showWhereTouched(ScreenPoint touch) {
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
//      for both Griduino and the Baroduino example
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
  ALTIMETER_VIEW = 0,    // 0 altimeter
  BARO_VIEW,             // 1 barometer graph
  BATTERY_VIEW,          // 2 coin battery voltage
  CFG_AUDIO_TYPE,        // 3 audio output Morse/speech
  CFG_CROSSING,          // 4 announce grid crossing 4/6 digit boundaries
  CFG_GPS,               // 5 gps/simulator
  CFG_GPS_RESET,         // 6 factory reset GPS
  CFG_NMEA,              // 7 broadcast NMEA
  CFG_REBOOT,            // 8 confirm reboot
  CFG_ROTATION,          // 9 screen rotation
  CFG_UNITS,             // 10 english/metric
  EVENTS_VIEW,           // 11 Groundhog Day, Halloween, or other day-counting screen
  GRID_VIEW,             // 12 <-- this is the primary navigation view
  GRID_CROSSINGS_VIEW,   // 13 log of time in each grid
  HELP_VIEW,             // 14 hints at startup
  SAT_COUNT_VIEW,        // 15 number of satellites acquired
  SCREEN1_VIEW,          // 16 first bootup screen
  SPLASH_VIEW,           // 17 startup
  STATUS_VIEW,           // 18 size and scale of this grid
  TEN_MILE_ALERT_VIEW,   // 19 microwave rover view
  TIME_VIEW,             // 20
  CFG_VOLUME,            // 21
  GOTO_SETTINGS,         // 22 command the state machine to show control panel
  GOTO_NEXT_VIEW,        // 23 command the state machine to show next screen
  MAX_VIEWS,             // 24 sentinel at end of list
};
/*const*/ int help_view      = HELP_VIEW;
/*const*/ int sat_count_view = SAT_COUNT_VIEW;
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
ViewBattery       batteryView(&tft, BATTERY_VIEW);
ViewCfgAudioType  cfgAudioType(&tft, CFG_AUDIO_TYPE);
ViewCfgCrossing   cfgCrossing(&tft, CFG_CROSSING);
ViewCfgGPS        cfgGPS(&tft, CFG_GPS);
ViewCfgGpsReset   cfgGpsReset(&tft, CFG_GPS_RESET);
ViewCfgNMEA       cfgNMEA(&tft, CFG_NMEA);
ViewCfgReboot     cfgReboot(&tft, CFG_REBOOT);
ViewCfgRotation   cfgRotation(&tft, CFG_ROTATION);
ViewCfgUnits      cfgUnits(&tft, CFG_UNITS);
ViewEvents        eventsView(&tft, EVENTS_VIEW);
ViewGrid          gridView(&tft, GRID_VIEW);
ViewGridCrossings gridCrossingsView(&tft, GRID_CROSSINGS_VIEW);
ViewHelp          helpView(&tft, HELP_VIEW);
ViewSatCount      satCountView(&tft, SAT_COUNT_VIEW);
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
      &batteryView,         // [BATTERY_VIEW]
      &cfgAudioType,        // [CFG_AUDIO_TYPE]
      &cfgCrossing,         // [CFG_CROSSING]
      &cfgGPS,              // [CFG_GPS]
      &cfgGpsReset,         // [CFG_GPS_RESET]
      &cfgNMEA,             // [CFG_NMEA]
      &cfgReboot,           // [CFG_REBOOT]
      &cfgRotation,         // [CFG_ROTATION]
      &cfgUnits,            // [CFG_UNITS]
      &eventsView,          // [EVENTS_VIEW]
      &gridView,            // [GRID_VIEW]
      &gridCrossingsView,   // [GRID_CROSSINGS_VIEW]
      &helpView,            // [HELP_VIEW]
      &satCountView,        // [SAT_COUNT_VIEW]
      &screen1View,         // [SCREEN1_VIEW]
      &splashView,          // [SPLASH_VIEW]
      &statusView,          // [STATUS_VIEW]
      &tenMileAlertView,    // [TEN_MILE_ALERT_VIEW]
      &timeView,            // [TIME_VIEW]
      &volumeView,          // [CFG_VOLUME]
  };

  int currentView = pView->screenID;
  int nextView;
  // clang-format off
  if (cmd == GOTO_NEXT_VIEW) {
    // operator requested the next NORMAL user view
    switch (currentView) {
      case SCREEN1_VIEW:   nextView = HELP_VIEW; break;   // skip SPLASH_VIEW (simplify startup, now that animated logo shows version number)
      case SPLASH_VIEW:    nextView = GRID_VIEW; break;
      case GRID_VIEW:      nextView = TIME_VIEW; break;
      case GRID_CROSSINGS_VIEW: nextView= GRID_VIEW; break;   // skip GRID_CROSSINGS_VIEW (not ready for prime time)
      case TIME_VIEW:      nextView = SAT_COUNT_VIEW; break;
      case SAT_COUNT_VIEW: nextView = BARO_VIEW; break;
      case BARO_VIEW:      nextView = ALTIMETER_VIEW; break;
      case ALTIMETER_VIEW: nextView = STATUS_VIEW; break;
      case STATUS_VIEW:    nextView = BATTERY_VIEW; break;
      case BATTERY_VIEW:   nextView = TEN_MILE_ALERT_VIEW; break;
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
      case CFG_GPS:        nextView = CFG_NMEA; break;
      case CFG_NMEA:       nextView = CFG_GPS_RESET; break;
      case CFG_GPS_RESET:  nextView = CFG_UNITS; break;
      case CFG_UNITS:      nextView = CFG_ROTATION; break;
#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
      case CFG_ROTATION:   nextView = CFG_REBOOT; break;
#else
      case CFG_ROTATION:   nextView = GRID_VIEW; break;   // at end of settings views, return to normal view
#endif
      case CFG_REBOOT:     nextView = CFG_VOLUME; break;
      // none of above: we must be showing some normal user view, so go to the first settings view
      default:             nextView = CFG_VOLUME; break;
    }
  } else if (cmd < MAX_VIEWS) {
    // a specific view was requested, such as HELP_VIEW via a USB command
    nextView = cmd;
  } else {
    logger.log(CONFIG, ERROR, "Requested view is out of range: %d where maximum is %d", cmd, MAX_VIEWS);
  }
  // clang-format on
  logger.log(CONFIG, INFO, "selectNewView() from %d to %d", currentView, nextView);
  if (currentView != nextView) {
    pView->endScreen();                   // a goodbye-kiss to the departing view
    pView = viewTable[ nextView ];

    // Every view has an initial setup to prepare its layout
    // After initial setup the view can assume it "owns" the screen
    // and thereafter safely repaint only the parts that change
    pView->startScreen();
    pView->updateScreen();
  }
}

// ----- console Serial port helper AND animated splash screen
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connection to the PC
  // and the operator takes awhile to restart the IDE console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE

  pView = &screen1View;   // select very first screen shown at startup
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
  logger.log(CONFIG, INFO, "Set brightness %d", brightness);
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
  logger.log(CONFIG, INFO, "Set brightness %d", brightness);
}
#endif

void sendMorseLostSignal() {
  String msg(PROSIGN_AS);             // "wait" symbol
  dacMorse.setMessage(msg);
  dacMorse.sendBlocking();            // TODO - use non-blocking
}

void announceGrid(const String gridName, int length) {
  // todo: change "String" class parameter into "char*" type
  char grid[7];
  strncpy(grid, gridName.c_str(), sizeof(grid));
  grid[length] = 0;   // null-terminate string to requested 4- or 6-character length
  logger.log(AUDIO, INFO, "Announcing grid: %s", grid);

#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  // todo - for now, RP2040 has no DAC, no audio, no speech
  logger.log(AUDIO, ERROR, "Unsupported audio in line ", __LINE__ );
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
      logger.log(AUDIO, ERROR, "Internal error: unsupported audio, line ", __LINE__);
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
  logger.log(AUDIO, INFO, "Say ", name);

#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
  // todo - for now, RP2040 has no DAC, no audio, no speech
  logger.log(AUDIO, ERROR, "Unsupported audio in line ", __LINE__ );
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
      char out[128];
      snprintf(out, sizeof(out), "sayGrid(%s) failed", letter);
      logger.log(AUDIO, ERROR, out);
    }
  }
#endif
}

elapsedSeconds viewHelpTimer;         // timer to show Help screen for only a few seconds at startup
uint viewHelpTimeout = 10;            // seconds to show Help screen at startup
                                      // startup: initialized to about 10 seconds
                                      // command: initialized to a long time, several minutes

//=========== setup ============================================
void setup() {

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);   // set backlight to full brightness

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(LANDSCAPE);         // 1=landscape (default is 0=portrait)
  tft.fillScreen(ILI9341_BLACK);      // note that "begin()" did not clear screen

  // ----- init screen orientation
  cfgRotation.loadConfig();           // restore previous screen orientation

  // ----- init touchscreen
  tsn.setScreenSize(tft.width(), tft.height());                                         // required
  tsn.setResistanceRange(X_MIN_OHMS, X_MAX_OHMS, Y_MIN_OHMS, Y_MAX_OHMS, XP_XM_OHMS);   // optional, for overriding defaults
  tsn.setThreshhold(START_TOUCH_PRESSURE, END_TOUCH_PRESSURE);                          // optional, for overriding defaults

  // ----- init Feather M4 onboard lights
  pixel.begin();
  pixel.clear();                      // turn off NeoPixel
  pinMode(RED_LED, OUTPUT);           // diagnostics RED LED
  digitalWrite(PIN_LED, LOW);         // turn off little red LED

  // ----- init serial monitor (do not "Serial.print" or "logger.log" before this, it won't show up in console)
  Serial.begin(115200);               // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);       // display very first screen, an animation splash at startup
                                      // AND wait for developer to connect debugging console

  pView = &helpView;                  // select very second screen shown at startup
  viewHelpTimer = 0;                  // start counting time for user to read the hint screen
  pView->startScreen();
  pView->updateScreen();

  // now that Serial is ready and connected (or we gave up)...
  logger.log(CONFIG, INFO,"NeoPixel initialized and turned off");
  logger.log(CONFIG, INFO, PROGRAM_TITLE " " PROGRAM_VERSION " " HARDWARE_VERSION);  // Report our program name to console
  logger.log(CONFIG, INFO, "Compiled " PROGRAM_COMPILED);       // Report our compiled date
  logger.log(CONFIG, INFO, __FILE__);                           // Report our source code file name

  // ----- init NMEA broadcasting on/off
  logger.log(CONFIG, INFO, "Starting cfgNMEA.loadConfig()...");
  cfgNMEA.loadConfig();

  // ----- init GPS
  GPS.begin(9600);   // 9600 NMEA is the default baud rate for Adafruit MTK GPS's
  delay(50);         // is delay really needed?

  // baud rate for GlobalTop GPS
  /*
  #define PMTK_SET_BAUD_38400  "$PMTK251,38400*27"
  logger.log(GPS_SETUP, INFO, "Set GPS baud rate to 38400: ");
  logger.log(GPS_SETUP, INFO, PMTK_SET_BAUD_38400);
  GPS.sendCommand(PMTK_SET_BAUD_38400);
  delay(50);
  GPS.begin(38400);
  delay(50);
  */

  /* ***** 576000 is for Adafruit Ultimate GPS only 
  logger.log(GPS_SETUP, INFO, "Set GPS baud rate to 57600: ");
  logger.log(GPS_SETUP, INFO, PMTK_SET_BAUD_57600);
  GPS.sendCommand(PMTK_SET_BAUD_57600);
  delay(50);
  GPS.begin(57600);
  delay(50);
  ***** */

  // init Quectel L86 chip to improve USA satellite acquisition
  logger.log(GPS_SETUP, INFO, "$PMTK353,1,0,0,0,0*2A");
  GPS.sendCommand("$PMTK353,1,0,0,0,0*2A");   // search American GPS satellites only (not Russian GLONASS satellites)
  delay(50);

  logger.log(GPS_SETUP, INFO, "Turn on RMC (recommended minimum) and GGA (fix data) including altitude: ");
  logger.log(GPS_SETUP, INFO, PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  delay(50);

  logger.log(GPS_SETUP, INFO, "Set GPS 1 Hz update rate: ");
  logger.log(GPS_SETUP, INFO, PMTK_SET_NMEA_UPDATE_1HZ);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);                // 1 Hz
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ);   // Every 5 seconds
  delay(50);

  if (0) {   // this command is saved in the GPS chip NVR, so always send one of these cmds
    logger.log(GPS_SETUP, INFO, "Request antenna status: ");
    logger.log(GPS_SETUP, INFO, PGCMD_ANTENNA);    // echo command to console log
    GPS.sendCommand(PGCMD_ANTENNA);   // tell GPS to send us antenna status
                                      // expected reply: $PGTOP,11,...
  } else {
    logger.log(GPS_SETUP, INFO, "Request to NOT send antenna status: ");
    logger.log(GPS_SETUP, INFO, PGCMD_NOANTENNA);    // echo command to console log
    GPS.sendCommand(PGCMD_NOANTENNA);   // tell GPS to NOT send us antena status
  }
  delay(50);

  // ----- query GPS firmware
  logger.log(GPS_SETUP, INFO, "Query GPS Firmware version: ");
  logger.log(GPS_SETUP, INFO, PMTK_Q_RELEASE);    // Echo query to console
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
  logger.log(GPS_SETUP, INFO, "Set sentence output frequencies: ");
  logger.log(GPS_SETUP, INFO, PMTK_SENTENCE_FREQUENCIES);    // Echo command to console
  GPS.sendCommand(PMTK_SENTENCE_FREQUENCIES);   // Send command to GPS unit
  delay(50);

  // ----- report on our memory hogs
  char temp[200];
  logger.log(CONFIG, INFO, "Large resources:");
  snprintf(temp, sizeof(temp), 
          ". Model.history[%d] uses %d bytes/entry = %d bytes total",
             trail.capacity, trail.recordSize, trail.totalSize);
  logger.log(CONFIG, INFO, temp);
  snprintf(temp, sizeof(temp),
          ". baroModel.pressureStack[%d] uses %d bytes/entry = %d bytes total",
             maxReadings, sizeof(BaroReading), sizeof(baroModel.pressureStack));
  logger.log(CONFIG, INFO, temp);

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
  trail.restoreGPSBreadcrumbTrail();  // this takes noticeable time (~0.2 sec)
  model->restore();                   //
  model->gHaveGPSfix = false;         // assume no satellite signal yet
  model->gSatellites = 0;
  trail.rememberPUP();                // log a "power up" event
  trail.saveGPSBreadcrumbTrail();     // ensure its saved for posterity

  // ----- restore barometric pressure history
  if (baroModel.loadHistory()) {
    logger.log(CONFIG, INFO, "Successfully restored barometric pressure log");
  } else {
    logger.log(CONFIG, WARNING, "Failed to load barometric pressure log, re-initializing file");
    baroModel.saveHistory();
  }

  // ----- init BMP388 or BMP390 barometer
  if (baroModel.begin()) {
    // success
  } else {
    // failed to initialize hardware
    tft.fillScreen(cBACKGROUND);
    tft.setCursor(0, 48);
    tft.setTextColor(cWARN);
    setFontSize(12);
    tft.println("Error!\n Unable to init\n  barometric sensor");
    delay(5000);
  }

  // ----- all done with setup, show opening view screen
  // at this point, we finished showing the splash screen
  // next up is the Help screen which is already in progress
  // by virtue of waitForSerial()
}

//=========== main work loop ===================================
// "millis()" is number of milliseconds since the Arduino began running the current program.
// This number will overflow after about 50 days.
//uint32_t prevTimeGPS = millis();
elapsedSeconds saveGpsTimer;          // timer to process and save the current GPS location
elapsedSeconds autoLogTimer;          // timer to save GPS trail periodically no matter what
elapsedSeconds batteryTimer;          // timer to log the coin battery voltage
uint32_t prevTimeBaro = millis();
time_t prevTimeRTC = 0;               // timer to print RTC to serial port (1 second)
elapsedMillis displayClockTimer;      // timer to update time-of-day display (1 second)
elapsedMillis losTimer;               // timer for Loss Of Signal

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
const int GPS_PROCESS_INTERVAL =  13;   // seconds between updating the model's GPS data
const int CLOCK_DISPLAY_INTERVAL = 1000;   // refresh clock display every 1 second (1,000 msec)
const uint32_t GPS_AUTOSAVE_INTERVAL = SECS_PER_10MIN; // seconds between saving breadcrumb trail to file
//const int BAROMETRIC_PROCESS_INTERVAL = 15*60*1000;  // fifteen minutes in milliseconds
const uint LOG_PRESSURE_INTERVAL = 15*60*1000;   // 15 minutes, in milliseconds
const uint LOS_ANNOUNCEMENT_INTERVAL = SECS_PER_5MIN * 1000;   // msec between LOS announcements
const int  LOG_COIN_BATTERY_INTERVAL = 2 * SECS_PER_1MIN;      // seconds between logging the coin battery voltage

void loop() {

  GPS.read();   // if you can, read the GPS serial port every millisecond

  if (GPS.newNMEAreceived()) {
    // optionally send NMEA sentences to Serial port, possibly for NMEATime2
    // Note: Adafruit parser doesn't handle $GPGSV (satellites in vieW) so we send all sentences regardless of content
    logger.nmea(CONSOLE, GPS.lastNMEA());

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
    logger.log(CONFIG, INFO, msg);    // debug

    // write this to the breadcrumb trail as preliminary indication of acquiring satellites
    TimeElements tm{GPS.seconds, GPS.minute, GPS.hour, 0, GPS.day, GPS.month, (byte)(2000-1970+GPS.year)};
    time_t firstTime = makeTime(tm);
    trail.rememberFirstValidTime(firstTime, GPS.satellites);
    trail.saveGPSBreadcrumbTrail();   // ensure its saved for posterity
  }

  // every 1 second update the realtime clock
  if (displayClockTimer > CLOCK_DISPLAY_INTERVAL) {
    displayClockTimer = 0;

    // update RTC from GPS
    if (date.isDateValid(GPS.year, GPS.month, GPS.day)) {

      setTime(GPS.hour, GPS.minute, GPS.seconds, GPS.day, GPS.month, GPS.year);
      // adjustTime(offset * SECS_PER_HOUR);  // todo - adjust to local time zone. for now, we only do GMT
    }
    pView->updateScreen();                   // update time on current view
  }

  // periodically save the number of satellites acquired for later analysis
  static elapsedSeconds satCountTimer;       // timer for saving number of GPS satellites
  const int SAT_SAVE_INTERVAL = 30;          // every thirty seconds
  if (satCountTimer > SAT_SAVE_INTERVAL) {
    satCountTimer = 0;

    satCountView.push(now(), model->gSatellites);
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
    logger.log(GMT, INFO, msg);
    prevTimeRTC = rtc;
  }

  // every 15 minutes read barometric pressure and save it in nonvolatile RAM
  // synchronize showReadings() on exactly 15-minute marks 
  // so the user can more easily predict when the next update will occur
  time_t rightnow = now();
  if (rightnow >= nextSavePressure) {

    // log this pressure reading only if the time-of-day is correct and initialized
    if (timeStatus() == timeSet) {
      baroModel.logPressure(rightnow);
      //redrawGraph = true;             // request draw graph
      nextSavePressure = date.nextFifteenMinuteMark( rightnow ); // production
      //nextSavePressure = date.nextOneMinuteMark( rightnow );   // debug
    }
  }

  // periodically, ask the model to process and save the current GPS location
  if (saveGpsTimer > GPS_PROCESS_INTERVAL) {
    saveGpsTimer = 0;
//  if (millis() - prevTimeGPS > GPS_PROCESS_INTERVAL) {
//    prevTimeGPS = millis();           // restart another interval

    model->processGPS();               // update model

    // update View
    pView->updateScreen();             // update current view, eg, updateGridScreen()
  }

  //if (!spkrMorse.continueSending()) {
  //  // give processing time to SpeakerMorseSender component
  //  // "continueSending" returns false after the message finishes sending
  //}

  // check for transition to "loss of signal"
  if (model->signalLost()) {   // test and write LOS and AOS to gps history log
    losTimer = 0;              // start timer for announcing LOS
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
    trail.saveGPSBreadcrumbTrail();   // entered new 4-digit grid

    model->save();                 // entered new 4-digit grid

  } else if (model->enteredNewGrid6()) {
    if (!model->compare4digits) {
      announceGrid(newGrid6, 6);   // announce with Morse code or speech, according to user's config
    }
    Location whereAmI;
    model->makeLocation(&whereAmI);
    trail.rememberGPS(whereAmI);    // when we enter a new 6-digit grid, save it in breadcrumb trail
    logger.fencepost("Griduino.ino new grid6",__LINE__);  // debug
    trail.saveGPSBreadcrumbTrail();   // entered new 6-digit grid 
    // one user's home was barely in the next grid6
    // and we want to show his grid6 at next power up
    // ALSO when entering a 6-digit grid: tell the model to save itself!
    // It's the _model_ that puts up the starting location at next power-up 
    model->save();                  // entered new 6-digit grid 
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
      logger.log(GPS_SETUP, DEBUG, "Moved a visible distance (line %d)", __LINE__);  // debug
      trail.saveGPSBreadcrumbTrail();   // have moved a visible distance
    }
  }

  // if we lost GPS signal for a long time, issue audible announcement
  if (model->gHaveGPSfix) {
    // satellites are in view, business as usual, restart LOS timer
    losTimer = 0;
  } else {
    // no GPS position available, let's wait see if it's reacquired soon
    if (losTimer > LOS_ANNOUNCEMENT_INTERVAL) {
      // it's been a few minutes without GPS
      sendMorseLostSignal();    // announce GPS signal lost by Morse code
      losTimer = 0;             // restart LOS timer
    }
  }

  // periodically log the coin battery's voltage 
  // todo: sensor exists only on PCB v11+
  if (batteryTimer > LOG_COIN_BATTERY_INTERVAL) {
    batteryTimer = 0;

    const float analogRef     = 3.3;    // analog reference voltage
    const uint16_t analogBits = 1024;   // ADC resolution is 10 bits

    int coin_adc       = analogRead(BATTERY_ADC);
    float coin_voltage = (float)coin_adc * analogRef / analogBits;
    logger.log(BATTERY, INFO, "Coin battery = %s", coin_voltage, 3);

    trail.rememberBAT(coin_voltage);
  }

  // log GPS position every few minutes, to keep track of lingering in one spot
  if (autoLogTimer > GPS_AUTOSAVE_INTERVAL) {
    autoLogTimer = 0;

    Location whereAmI;
    model->makeLocation(&whereAmI);
    logger.log(GPS_SETUP, DEBUG, "Griduino.ino autolog timer (line %d)", __LINE__);  // debug
    //whereAmI.printLocation();                                 // debug
    trail.rememberGPS(whereAmI);
    trail.saveGPSBreadcrumbTrail();   // autosave timer
  }

  if ((pView->screenID == HELP_VIEW) && (viewHelpTimer > viewHelpTimeout)) {
    selectNewView(grid_view);
  }

  // if there's touchscreen input, handle it
  ScreenPoint screenLoc;
  if (tsn.newScreenTap(&screenLoc, tft.getRotation())) {

    if (showTouchTargets) {
      showWhereTouched(screenLoc);   // debug: show where touched
    }

    Point touch = {screenLoc.x, screenLoc.y};
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
