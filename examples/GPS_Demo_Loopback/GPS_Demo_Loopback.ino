// Please format this file with clang before check-in to GitHub
/*
  Arduino Demo GPS Loopback

  Version history:
            2021-06-09 updated for testing with new Quectel GPS chip
            2020-05-18 added NeoPixel control to illustrate technique
            2019-11-04 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Source:   https://learn.adafruit.com/adafruit-ultimate-gps/direct-computer-wiring

            https://learn.adafruit.com/adafruit-ultimate-gps

  Tested with:
         1. Arduino Feather M4 Express (120 MHz SAMD51)     https://www.adafruit.com/product/3857

         2. Adafruit Ultimate GPS                           https://www.adafruit.com/product/746
               Tested and works great with the Adafruit Ultimate GPS module (Mediatek MTK33x9 chipset)
         3. Quectel LC86L GPS chip on a breakout board of our own design
               L86 is an ultra compact GNSS POT (Patch on Top) module (MediaTek MT3333 chipset)
               High sensitivity: -167dBm @ Tracking, -149dBm @ Acquisition

  NMEA references:
         https://gpsd.gitlab.io/gpsd/NMEA.html - "NMEA Revealed" by Eric S. Raymond, comprehensive guide
         https://www.nmea.org/content/STANDARDS/NMEA_0183_Standard - NMEA Standards, bullshit $2,000 for standards
         https://www.nmea.org/Assets/NMEA%200183%20Talker%20Identifier%20Mnemonics.pdf - NMEA Talker ID
         https://www.sparkfun.com/datasheets/GPS/Modules/PMTK_Protocol.pdf - $PMTK packet protocol quick guide
         https://www.rhydolabz.com/documents/25/NMEA-CommandManual_%28FTX-HW-13002%29.pdf - $PMTK packet protocol

  Quectel LC86 references:
         https://www.quectel.com/product/gnss-l86/ - LC86 product guide
         https://sixfab.com/wp-content/uploads/2018/10/Quectel_GNSS_SDK_Commands_Manual_V1.4.pdf - Quectel NMEA extensions

  Usage:
         Run this program in the Arduino IDE; open Tools > Serial Monitor, 115200 baud
         Then run "u-center" from www.u-blox.com to decode and display the NMEA packets

  Examples: Adafruit Ultimate GPS
         $GPRMC,193606.305,V,         , ,          , ,0.00,  0.00,021119,,,N*4A
         $GPRMC,194807.000,A,4745.2351,N,12217.0479,W,0.18,210.96,021119,,,A*7A
          |  |  |          | |         | |          |  |    |     |         |
          |  |  |          | |         | |          |  |    |     |         checksum
          |  |  |          | |         | |          |  |    |     date DDMMYY
          |  |  |          | |         | |          |  |    compass direction of travel
          |  |  |          | |         | |          |  speed (knots)
          |  |  |          | |         | |          E/W
          |  |  |          | |         | DDMM.MMMM
          |  |  |          | |         N/S
          |  |  |          | DDMM.MMMM
          |  |  |          A = active, lock (V = void, no lock)
          |  |  193606.305 = time GMT, 7:36 pm, 06 seconds, 305 msec
          |  RMC=recommended minimum GNSS data, GGA=global positioning system fix data
          Talker ID, GP=GPS-receiver, GL=GLONASS, GA=Galileo, GN=multi GNSS, P=Proprietary

  Examples: Quectel LC86L GPS
         $GNRMC,140936.092,V,         , ,          , ,0.00,  0.00,090621, , ,N,V*27
          |  |  |          | |         | |          |  |    |     |      | |    |
          |  |  |          | |         | |          |  |    |     |      | |    checksum
          |  |  |          | |         | |          |  |    |     |      | mode A=autonomous, D=differential
          |  |  |          | |         | |          |  |    |     |      magnetic variation
          |  |  |          | |         | |          |  |    |     date DDMMYY
          |  |  |          | |         | |          |  |    compass direction of travel
          |  |  |          | |         | |          |  speed (knots)
          |  |  |          | |         | |          E/W
          |  |  |          | |         | DDMM.MMMM
          |  |  |          | |         N/S
          |  |  |          | DDMM.MMMM
          |  |  |          A = active, lock (V = void, no lock)
          |  |  hhmmss.sss
          |  RMC=recommended minimum GNSS data, GGA=global positioning system fix data
          Talker ID, GP=GPS-receiver, GL=GLONASS, GA=Galileo, GN=multi GNSS, P=Proprietary

         $GPTXT,01,01,01,ANTENNA OPEN*25
         $GPTXT,01,01,02,ANTSTATUS=OPEN*2B
                |  |  |  |             |
                |  |  |  |             checksum
                |  |  |  any ascii text
                |  |  severity, 0=error, 1=warning, 2=notice, 7=user
                |  message number
                total number of messages

         $PMTK001,314,3*36
         $PMTK001,220,3*30
          | |   |  |  |  |
          | |   |  |  |  checksum
          | |   |  |  flag, 0=invalid packet, 1=unsupported packet, 2=valid packet action failed, 3=success
          | |   | 220 = cmd received, 220=update rate, 314=turn on RMC
          | | 001 = acknowledgment of PMTK command received
          | MTK=MediaTek protocol
          Talker ID, P=Proprietary

Adafruit: $PMTK705,AXN_2.31_3339_13101700,5632,PA6H,1.0*6B
           | |  |  |                      |    |    |  checksum
           | |  |  |                      |    |    chip version
           | |  |  |                      |    chip name
           | |  |  |                      firmware build ID
           | |  |  firmware name and version
           | |  705 = firmware release message
           | MTK=MediaTek protocol
           Talker ID, P=Proprietary

Quectel:  $PMTK705,AG3331_AXN5.1.9_MODULE_STD_F8_P2,000B,Quectel-LC86L,1.0*26
           | |  |  |                                |    |       |      |  checksum
           | |  |  |                                |    |       |      chip version
           | |  |  |                                |    |       chip name
           | |  |  |                                |    mfr name
           | |  |  |                                firmware build ID
           | |  |  firmware name and version
           | |  705 = firmware release message
           | MTK=MediaTek protocol
           Talker ID, P=Proprietary

         $GPGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5*02
         $BDGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5*13
          | |   | |              |    |    |  checksum
          | |   | |              |    |   Vertical DOP
          | |   | |              |   Horizontal DOP
          | |   | |             Position DOP
          | |   | Mode 1=fix not available, 2=2D, 3=3D
          | |   A = automatic, M = manual
          | GPS DPO and active satellites
          Talker ID

         $PMTK314,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29
          | | |   | | | | | | |                       | checksum
          | | |   | | | | | | |                       PMTKCHN interval – GPS channel status
          | | |   | | | | | | Reserved
          | | |   | | | | | GPGSV interval - GNSS Satellites in View
          | | |   | | | | GPGSA interval - GNSS DOPS and Active Satellites
          | | |   | | | GPGGA interval - GPS Fix Data
          | | |   | | GPVTG interval - Course over Ground and Ground Speed
          | | |   | GPRMC interval - Recommended Minimum Specific GNSS Sentence
          | | |   GPGLL interval - Geographic Position - Latitude longitude
          | | 314 = Set NMEA sentence output frequencies
          | MTK=MediaTek protocol
          Talker ID, P=Proprietary
          Supported frequency settings are:
            0 - Disabled or not supported sentence
            1 - Output once every one position fix
            2 - Output once every two position fixes
            3 - Output once every three position fixes
            4 - Output once every four position fixes
            5 - Output once every five position fixes

*/

#include <Adafruit_ILI9341.h>   // TFT color display library
#include <Adafruit_GPS.h>       // Ultimate GPS library
#include "hardware.h"           // Griduino pin definitions

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE    "GPS Demo Loopback"
#define PROGRAM_VERSION  "v1.15"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

#define SCREEN_ROTATION 1   // 1=landscape, 3=landscape 180-degrees

// ---------- Hardware Wiring ----------
// Same as Griduino platform - see hardware.h

// create an instance of the TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// ---------- GPS ----------
/* "Ultimate GPS" pin wiring is connected to a dedicated hardware serial port
    available on an Arduino Mega, Arduino Feather and others.

    Adafruit GPS LED indicates status:
        1-sec blink = searching for satellites
        15-sec blink = position fix found
*/

// Hardware serial port for GPS
Adafruit_GPS GPS(&Serial1);

// ------------ definitions
const int howLongToWait = 8;   // max number of seconds at startup waiting for Serial port to console

// ----- Griduino color scheme
// RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
#define cBACKGROUND 0x00A   // 0,   0,  10 = darker than ILI9341_NAVY, but not black

// ========== splash screen helpers ============================
// splash screen layout
#define indent 10
#define yRow1  18           // program title & version
#define yRow2  yRow1 + 22   // message line 1

#define yRow4 yRow2 + 36   // echo GPS sentence, even
#define yRow5 yRow4 + 60   // echo GPS sentence, odd

void clearScreen(int color = cBACKGROUND) {
  tft.fillScreen(color);
}

void startSplashScreen() {
  clearScreen();
  tft.setTextSize(2);
  tft.setCursor(indent, yRow1);
  tft.print(PROGRAM_TITLE);
  tft.print("  ");
  tft.print(PROGRAM_VERSION);
  tft.setCursor(indent, yRow2);
  tft.print("See IDE console monitor");
}

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connection to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong * 1000;
  while (millis() < targetTime) {
    if (Serial)
      break;
  }
}

//=========== setup ============================================
void setup() {

  // ----- init TFT display
  tft.begin();                        // initialize TFT display
  tft.setRotation(SCREEN_ROTATION);   // 1=landscape (default is 0=portrait)
  clearScreen();

  // ----- init TFT backlight
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);   // set backlight to full brightness

  // ----- init Feather M4 onboard lights
  digitalWrite(PIN_LED, LOW);   // turn off little red LED

  // ----- announce ourselves
  startSplashScreen();
  tft.setCursor(indent, yRow4);   // prepare for the first NMEA sentence to show on screen

  // ----- init serial monitor
  Serial.begin(115200);           // init for debugging in the Arduino IDE
  waitForSerial(howLongToWait);   // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println("\n-------------------------------");
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);   // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);        // Report our compiled date
  Serial.println(__FILE__);                            // Report our source code file name
  Serial.println("NeoPixel initialized and turned off");

  // ----- init GPS
  /* test: what happens in John Vanderbeck's GPS if there is NO init baud rate all? */
  GPS.begin(9600);   // 9600 NMEA is the default baud rate for Adafruit MTK GPS's
  delay(10);         // is delay really needed?

  // ----- query GPS
  Serial.print("Query GPS firmware version: ");
  Serial.println(PMTK_Q_RELEASE);
  GPS.sendCommand(PMTK_Q_RELEASE);
  delay(200);
  /* end test */

  init_Adafruit_GPS();

  // init_Quectel_GPS();   // todo, after we figure out what special init is needed
}

//=========== Adafruit Ultimate GPS ============================

void init_Adafruit_GPS() {

  /*
  Serial.print("Set GPS baud rate to 57600: ");
  Serial.println(PMTK_SET_BAUD_57600);    // baud rate (to console)
  GPS.sendCommand(PMTK_SET_BAUD_57600);   // baud rate (to GPS chip)
  delay(10);

  GPS.begin(57600);
  delay(10);
  */

  // init Quectel L86 chip to improve USA satellite acquisition
  /*
  GPS.sendCommand("$PMTK353,1,0,0,0,0*2A");   // search American GPS satellites only (not Russian GLONASS satellites)
  delay(10);
  */

  Serial.print("Turn on RMC (recommended minimum) and GGA (fix data) including altitude: ");
  Serial.println(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  delay(10);

  if (0) {
    Serial.print("Set GPS 1 Hz update rate: ");
    Serial.println(PMTK_SET_NMEA_UPDATE_1HZ);    // 1 Hz (to console)
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz (to GPS chip)
  } else {
    Serial.print("Set GPS update rate to every 5 seconds: ");
    Serial.println(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ);    // Every 5 seconds (to console)
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ);   // Every 5 seconds (to GPS chip)
  }

  delay(10);

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
  delay(10);

  // ----- query GPS firmware
  Serial.print("Sending command to query GPS Firmware version: ");
  Serial.println(PMTK_Q_RELEASE);    // Echo query to console
  GPS.sendCommand(PMTK_Q_RELEASE);   // Send query to GPS unit
                                     // expected reply: $PMTK705,AXN_2.10...
  delay(10);

// ----- turn on additional satellite reporting to support NMEATime2
//                                          GPGLL              Geographic Latitude longitude
//                                          | GPRMC            Recommended Minimum Coordinates
//                                          | | GPVTG          Velocity Over Ground
//                                          | | | GPGGA        GPS Fix Data
//                                          | | | | GPGSA      GNSS DOPS and Active Satellites
//                                          | | | | | GPGSV    GNSS Satellites in View
//                                          | | | | | |
#define PMTK_SENTENCE_FREQUENCIES "$PMTK314,0,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
  //                                                      | | | | | PMTKDBG  MTK debug information
  //                                                      | | | | MTKDGP   GPS differential correction information
  //                                                      | | | PMTKEPH  GPS ephemeris information
  //                                                      | | PMTKALM  GPS almanac information
  //                                                      | GPGST  GNSS Pseudorange Errors Statistics
  //                                                      GPGRS  GNSS Range Residuals
  Serial.print("Sending command to set sentence output frequencies: ");
  Serial.println(PMTK_SENTENCE_FREQUENCIES);    // Echo command to console
  GPS.sendCommand(PMTK_SENTENCE_FREQUENCIES);   // Send command to GPS unit
  delay(10);
}

/* Example: Typical output from Adafruit Ultimate GPS
 *  This works fine with NMEATime2
  $GPGGA,160619.000,4745.1791,N,12217.0746,W,1,06,1.87,27.7,M,-17.2,M,,*6A    -- GPS fix data
  $GPGSA,A,3,11,20,02,25,12,29,,,,,,,2.11,1.87,0.97*0F                        -- GNSS DOPS
  $GPGSV,3,1,11,02,73,065,18,25,68,273,20,11,60,062,21,12,59,161,13*70        -- GNSS satellites in view
  $GPGSV,3,2,11,20,46,117,21,29,38,282,17,51,33,160,,05,25,155,*72            --   "
  $GPGSV,3,3,11,06,23,051,13,31,18,315,,18,01,227,*4B                         --   "
  $GPRMC,160619.000,A,4745.1791,N,12217.0746,W,0.27,28.81,260722,,,A*4E       -- Recommended minimum coordinates
*/

//=========== main work loop ===================================
int count = 0;   // number of NMEA sentences received

void loop() {
  if (Serial1.available()) {   // read from GPS
    char c = Serial1.read();   //
    Serial.write(c);           // write to console

    if (c == '$') {
      int yy = (count % 2) ? yRow4 : yRow5;
      tft.setCursor(indent, yy);
      tft.fillRect(0, yy, tft.width(), 48, cBACKGROUND);
      count++;
    }
    tft.print(c);
  }
  if (Serial.available()) {   // read from console
    char c = Serial.read();   //
    Serial1.write(c);         // write to GPS
  }
}
