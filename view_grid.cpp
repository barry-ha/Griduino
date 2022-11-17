/*
  File:     view_grid.cpp

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  This is a GPS display for your vehicle's dashboard to
            show your position in the Maidenhead Grid Square, with distances
            to nearby squares. This is intended for ham radio rovers.

            +-------------------------------------------+
            |    -124        CN88  30.1 mi     -122     |...gTopRowY
            |   48 +-----------------------------+......|...gMarginY
            |      |                          *  |      |
            | CN77 |         CN87                | CN97 |...gMiddleRowY
            | 61.2 |          us                 | 37.1 |
            |      |                             |      |
            |   47 +-----------------------------+      |
            | 123' :         CN86  39.0 mi       :  75° |...gBottomGridY
            | 47.5644, -122.0378                 :   5# |...gMessageRowY
            +------:---------:-------------------:------+
                   :         :                   :
                 +gMarginX  gIndentX          -gMarginX
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>         // TFT color display library
#include "constants.h"                // Griduino constants and colors
#include "logger.h"                   // conditional printing to Serial port
#include "grid_helper.h"              // lat/long conversion routines
#include "model_gps.h"                // Model of a GPS for model-view-controller
#include "model_baro.h"               // Model of a barometer that measures temperature
#include "TextField.h"                // Optimize TFT display text for proportional fonts
#include "view.h"                     // Base class for all views

// ========== extern ===========================================
extern Logger logger;                 // Griduino.ino
extern Grids grid;                    // grid_helper.h
extern Adafruit_ILI9341 tft;          // Griduino.ino
extern Model* model;                  // "model" portion of model-view-controller
extern BarometerModel baroModel;      // singleton instance of the barometer model

extern Location history[];            // GPS breadcrumb trail (Griduino.ino)
extern const int numHistory;          // Griduino.ino

extern void showDefaultTouchTargets();  // Griduino.ino
extern void setFontSize(int font);      // TextField.cpp
extern void floatToCharArray(char* result, int maxlen, double fValue, int decimalPlaces);  // Griduino.ino

// ============== constants ====================================
const int gMarginX = 70;              // define space for grid outline on screen
const int gMarginY = 26;              // and position text relative to this outline

// ========== helpers ==========================================
const double minLong = gridWidthDegrees / gBoxWidth;  // longitude degrees from one pixel to the next (minimum visible movement)
const double minLat = gridHeightDegrees / gBoxHeight; // latitude degrees from one pixel to the next

bool isVisibleDistance(const PointGPS from, const PointGPS to) {
  // has the vehicle moved some minimum amount, enough to be visible?

  if (abs(from.lat - to.lat) >= minLat) {
    return true;
  } else if (abs(from.lng - to.lng) >= minLong) {
    return true;
  } else {
    return false;
  }
}
void drawGridOutline() {
  tft.drawRect(gMarginX, gMarginY, gBoxWidth, gBoxHeight, ILI9341_CYAN);
}

  // ========== text screen layout ===================================
  // these are names for the array indexes, must be named in same order as array below
enum txtIndex {
  GRID4=0, GRID6, LATLONG, ALTITUDE, NUMSAT, TEMPERATURE,
  N_COMPASS,  S_COMPASS,  E_COMPASS,  W_COMPASS,
  N_DISTANCE, S_DISTANCE, E_DISTANCE, W_DISTANCE,
  N_GRIDNAME, S_GRIDNAME, E_GRIDNAME, W_GRIDNAME,
  N_BOX_LAT,  S_BOX_LAT,  E_BOX_LONG, W_BOX_LONG,
};

    // ----- dynamic screen text
TextField txtGrid[] = {
  //         text      x,y     color
  TextField("CN77",  101,101,  cGRIDNAME),      // GRID4: center of screen
  TextField("tt",    138,141,  cGRIDNAME),      // GRID6: center of screen
  TextField("47.1234,-123.4567", 4,223, cSTATUS), // LATLONG: left-adj on bottom row
  TextField("123'",   62,196,  cSTATUS, ALIGNRIGHT),  // ALTITUDE: just above bottom row
  TextField("99#",   313,221,  cSTATUS, ALIGNRIGHT),  // NUMSAT: lower right corner
  TextField("75F",   313,196,  cSTATUS, ALIGNRIGHT),  // TEMPERATURE
  TextField( "N",    156, 47,  cCOMPASS ),      // N_COMPASS: centered left-right
  TextField( "S",    156,181,  cCOMPASS ),      // S_COMPASS
  TextField( "E",    232,114,  cCOMPASS ),      // E_COMPASS: centered top-bottom
  TextField( "W",     73,114,  cCOMPASS ),      // W_COMPASS
  TextField("17.1",  180, 20,  cDISTANCE),      // N_DISTANCE
  TextField("52.0",  180,207,  cDISTANCE),      // S_DISTANCE
  TextField("13.2",  256,130,  cDISTANCE),      // E_DISTANCE
  TextField("79.7",    0,130,  cDISTANCE),      // W_DISTANCE
  TextField("CN88",  102, 20,  cGRIDNAME),      // N_GRIDNAME
  TextField("CN86",  102,207,  cGRIDNAME),      // S_GRIDNAME
  TextField("CN97",  256,102,  cGRIDNAME),      // E_GRIDNAME
  TextField("CN77",    0,102,  cGRIDNAME),      // W_GRIDNAME
  TextField("48",     56, 44,  cBOXDEGREES, ALIGNRIGHT),  // N_BOX_LAT
  TextField("47",     56,190,  cBOXDEGREES, ALIGNRIGHT),  // S_BOX_LAT
  TextField("122",   243, 20,  cBOXDEGREES),              // E_BOX_LONG
  TextField("124",    72, 20,  cBOXDEGREES, ALIGNRIGHT),  // W_BOX_LONG
};
const int numTextGrid = sizeof(txtGrid)/sizeof(TextField);

void drawGridName(String newGridName) {
  // huge lettering of current grid square
  // two lines: "CN87" and "us" below it

  setFontSize(24);

  String grid1_4 = newGridName.substring(0, 4);
  String grid5_6 = newGridName.substring(4, 6);

  txtGrid[GRID4].print(grid1_4);
  txtGrid[GRID6].print(grid5_6);
}

void drawPositionLL(double fLat, double fLong) {
  setFontSize(0);

  // the message line shows either or a position (lat,long) or a message (waiting for GPS)
  char sTemp[27];       // why 27? Small system font will fit 26 characters on one row (smallest fits >32)
  if (model->gHaveGPSfix) {
    char latitude[10], longitude[10];
    floatToCharArray(latitude, sizeof(latitude), fLat, 4);
    floatToCharArray(longitude, sizeof(longitude), fLong, 4);
    snprintf(sTemp, sizeof(sTemp), "%s, %s", latitude, longitude);
  } else {
    strcpy(sTemp, "Waiting for GPS");
  }
  txtGrid[LATLONG].print(sTemp);      // latitude-longitude

}

void drawTemperature(float celsius) {
  setFontSize(0);
  float temperature; 
  char units[2] = "?";
  if (model->gMetric) {
    temperature = celsius;
    units[0] = 'c';
  } else {
    temperature = celsius * 9/5 + 32;
    units[0] = 'F';
  }
  if (celsius < 43) {   // 43C = 110F. Try this awhile and see how it goes.
    txtGrid[TEMPERATURE].color = cSTATUS; // normal temperature
  } else {
    txtGrid[TEMPERATURE].color = cWARN; // internal case temperature warning
  }
  char sFloat[8];    // strlen("123F") = 4
  floatToCharArray(sFloat, sizeof(sFloat)-sizeof(units), temperature, 0);
  strcat(sFloat, units);
  txtGrid[TEMPERATURE].print(sFloat);   // Griduino's internal temperature
}

void drawNumSatellites() {
  setFontSize(0);

  char sTemp[4];    // strlen("12#") = 3
  if (model->gSatellites<10) {
    snprintf(sTemp, sizeof(sTemp), " %d#", model->gSatellites);
  } else {
    snprintf(sTemp, sizeof(sTemp), "%d#", model->gSatellites);
  }
  txtGrid[NUMSAT].print(sTemp);       // number of satellites
 
}

void drawAltitude() {
  setFontSize(0);

  char sTemp[8];                      // strlen("12345'") = 6
  if (model->gMetric) {
    int altMeters = model->gAltitude;
    snprintf(sTemp, sizeof(sTemp), "%dm", altMeters);
  } else {
    int altFeet = model->gAltitude * feetPerMeters;
    snprintf(sTemp, sizeof(sTemp), "%d'", altFeet);
  }
  txtGrid[ALTITUDE].print(sTemp);     // altitude
}

void drawCompassPoints() {
  setFontSize(12);
  for (int ii=N_COMPASS; ii<N_COMPASS+4; ii++) {
    txtGrid[ii].print();
  }
}

void drawBoxLatLong() {
  setFontSize(12);
  txtGrid[N_BOX_LAT].print( ceil(model->gLatitude) );    // latitude of N,S box edges
  txtGrid[S_BOX_LAT].print( floor(model->gLatitude) );
  txtGrid[E_BOX_LONG].print( grid.nextGridLineEast( model->gLongitude ) ); // longitude of E,W box edges
  txtGrid[W_BOX_LONG].print( grid.nextGridLineWest( model->gLongitude ) );
  
  int radius = 3;
  // draw "degree" symbol at:       x                        y        r     color
  tft.drawCircle(txtGrid[N_BOX_LAT].x+7,  txtGrid[N_BOX_LAT].y-14,  radius, cBOXDEGREES); // draw circle to represent "degrees"
  tft.drawCircle(txtGrid[S_BOX_LAT].x+7,  txtGrid[S_BOX_LAT].y-14,  radius, cBOXDEGREES);
  //t.drawCircle(txtGrid[E_BOX_LONG].x+7, txtGrid[E_BOX_LONG].y-14, radius, cBOXDEGREES); // no room for "degrees" on ALIGNLEFT number?
  tft.drawCircle(txtGrid[W_BOX_LONG].x+7, txtGrid[W_BOX_LONG].y-14, radius, cBOXDEGREES);
}

void drawNeighborGridNames() {
  setFontSize(12);
  char nGrid[5], sGrid[5], eGrid[5], wGrid[5];

  grid.calcLocator(nGrid, model->gLatitude+1.0, model->gLongitude, 4);
  grid.calcLocator(sGrid, model->gLatitude-1.0, model->gLongitude, 4);
  grid.calcLocator(eGrid, model->gLatitude, model->gLongitude+2.0, 4);
  grid.calcLocator(wGrid, model->gLatitude, model->gLongitude-2.0, 4);

  txtGrid[N_GRIDNAME].print(nGrid);
  txtGrid[S_GRIDNAME].print(sGrid);
  txtGrid[E_GRIDNAME].print(eGrid);
  txtGrid[W_GRIDNAME].print(wGrid);
}

void drawNeighborDistances() {
  setFontSize(12);

  // N-S: grid lines occur on nearest INTEGER degree
  float fNorth = grid.calcDistanceLat(model->gLatitude, ceil(model->gLatitude), model->gMetric);
  if (fNorth < 2.0) {
    txtGrid[N_DISTANCE].print( fNorth, 2 );
  } else {
    txtGrid[N_DISTANCE].print( fNorth, 1 );
  }
  float fSouth = grid.calcDistanceLat(model->gLatitude, floor(model->gLatitude), model->gMetric);
  if (fSouth < 2.0) {
    txtGrid[S_DISTANCE].print( fSouth, 2 );
  } else {
    txtGrid[S_DISTANCE].print( fSouth, 1 );
  }
  
  // E-W: grid lines occur on nearest EVEN degrees
  int eastLine = grid.nextGridLineEast( model->gLongitude );
  int westLine = grid.nextGridLineWest( model->gLongitude );
  float fEast = grid.calcDistanceLong(model->gLatitude, model->gLongitude, eastLine, model->gMetric);
  float fWest = grid.calcDistanceLong(model->gLatitude, model->gLongitude, westLine, model->gMetric);
  if (fEast < 2.0) {
    txtGrid[E_DISTANCE].print( fEast, 2 );
  } else {
    txtGrid[E_DISTANCE].print( fEast, 1 );
  }
  if (fWest < 2.0) {
    txtGrid[W_DISTANCE].print( fWest, 2 );
  } else {
    txtGrid[W_DISTANCE].print( fWest, 1 );
  }
}

// =============================================================
void translateGPStoScreen(Point* result, const PointGPS loc, const PointGPS origin) {
  // result = screen coordinates of given GPS coordinates
  // loc    = GPS coordinates of target
  // origin = GPS coordinates of currently displayed grid square, lower left corner
  // 
  // Example calculations
  //         -124                                 -122
  //         48 +----------------------------------+- - y=0
  //            |                                  |
  //            |       +------------------+ - - - |- - gMarginY
  //            |       |                  |       |
  //            |       |      CN87        |       |
  //            |       |       us         |       |
  //            |       +------------------+ - - - |- - gMarginY+gBoxHeight
  //            |       :                  :       |
  //         47 +-------:------------------:-------+- - y=240
  //            :       :                  :       :
  // Screen   x=0   gMarginX   gMarginX+gBoxWidth  gScreenWidth  
  //                    :                  :
  // Longitude =    degreesX   degreesX+gWidthDegrees
    
  result->x = -1;            // assume result is off-screen
  result->y = -1;

  const float xPixelsPerDegree = gBoxWidth / gridWidthDegrees;    // one grid square = 2.0 degrees wide E-W
  const float yPixelsPerDegree = gBoxHeight / gridHeightDegrees;  // one grid square = 1.0 degrees high N-S

  result->x = gMarginX + (int)( (loc.lng - origin.lng)*xPixelsPerDegree );
  result->y = gMarginY + gBoxHeight - (int)( (loc.lat - origin.lat)*yPixelsPerDegree );

  //Serial.print("~ From ("); Serial.print(loc.lat,3); Serial.print(","); Serial.print(loc.lng,3); Serial.print(")");
  //Serial.print(" to ("); Serial.print(result->x); Serial.print(","); Serial.print(result->y); Serial.print(")");
  //Serial.print(" using grid corner("); Serial.print(origin.lat,1); Serial.print(","); Serial.print(origin.lng,1); Serial.print(")");
  //Serial.println("");
}
// =============================================================
void plotRoute(Location* marker, const int numMarkers, const PointGPS origin) {
  // show route track history bread crumb trail
  //Serial.print("plotRoute() at line "); Serial.println(__LINE__);   // debug
  //Serial.print("~ Plot relative to origin("); Serial.print(origin.lat); Serial.print(","); Serial.print(origin.lng); Serial.println(")");
  //model->dumpHistory();    // debug

  Point prevPixel{0,0};     // keep track of previous dot plotted

  for (int ii=0; ii<numMarkers; ii++) {     // loop through Location[] array of history
    Location mark = marker[ii];
    if (!mark.isEmpty()) {
      Point screen;
      PointGPS spot{mark.loc.lat, mark.loc.lng};
      translateGPStoScreen(&screen, spot, origin);
      
      // erase a few dots around this to make it more visible
      // but! which dots to erase depend on what direction we're moving
      // let's try detecting the giant green grid letters, and selectively erasing them
      //       (fail - there is no API to read a pixel)
      // let's try detecting the direction of travel
      if (prevPixel.x == screen.x
       && prevPixel.y == screen.y) {
        // nothing changed, erase nothing
      } else {
        /*
         * this works great for simple horiz/vert movement
         * but not so much for diagonal lines
         */
        if (prevPixel.y == screen.y) {
          // horizontal movement
          tft.drawPixel(screen.x, screen.y-1, ILI9341_BLACK);
          tft.drawPixel(screen.x, screen.y+1, ILI9341_BLACK);
        } else if (prevPixel.x == screen.x) {
          // vertical movement
          tft.drawPixel(screen.x-1, screen.y, ILI9341_BLACK);
          tft.drawPixel(screen.x+1, screen.y, ILI9341_BLACK);
        } else {
          // let's try constructing the perpendicular and drawing a 3-pixel long line
          // y = mx+b, where m is the slope, b is the y-intercept
          /* 
           * Result: the below is ugly because it uses fine details that are not 
           * smooth nor anti-aliased. It works but it's commented out.
           */
          /*
          float slope = (screen.y - prevPixel.y)/(screen.x - prevPixel.x);
          float m = -1.0 / slope;
          float b = float(screen.y) - (m * screen.x);

          if (abs(m) > 1.0) {
            // the perpendicular line is closer to vertical
            // so erase in x-direction a little left/right
            int xLeft = screen.x-1;
            int xRight = screen.x+1;
            int yLeft = (int)m*xLeft + b; // y=mx+b
            int yRight = (int)m*xRight + b;
            tft.drawLine(xLeft,yLeft, xRight,yRight, ILI9341_BLACK);
          } else {
            // the perpendicular line is closer to horizontal
            // so erase in y-direction a little above/below
            int yTop = screen.y-1;
            int yBot = screen.y+1;
            int xTop = (int)(yTop-b)/m; // x=(y-b)/m
            int xBot = (int)(yBot-b)/m;
            tft.drawLine(xTop,yTop, xBot,yBot, ILI9341_BLACK);
          }
          */
        }
      }

      // plot this location
      tft.drawPixel(screen.x, screen.y, cBREADCRUMB);
      prevPixel = screen;
    }
  }
}
// =============================================================
void plotVehicle(const Point car, uint16_t carColor) {
  // put a symbol on the screen to represent the vehicle
  // choose what shape you like at compile-time
  // if you're an over-achiever, write new code so a triangle indicates direction of travel
  int radius, size, w, h;
  switch (3) {
    case 1:   // ----- circle
      radius = 3;
      tft.fillCircle(car.x, car.y, radius-1, ILI9341_BLACK);  // erase the circle's background
      tft.drawCircle(car.x, car.y, radius, carColor);         // draw new circle
      break;

    case 2:   // ----- triangle
      size = 4;
      tft.drawTriangle(car.x-size, car.y+size,
                       car.x+size, car.y+size,
                       car.x,      car.y-size,
                       carColor);
      break;

    case 3:   // ----- square
      w = h = 8;
      tft.drawRect(car.x-w/2, car.y-h/2, w, h, carColor);
      break;
  }
}
// =============================================================
Point prevVehicle;
void plotCurrentPosition(const PointGPS loc, const PointGPS origin) {
  // drop a bread crumb inside the grid's box, proportional to your position within the grid
  // input:  loc    = double precision float, GPS coordinates of current position
  //         origin = GPS coordinates of currently displayed grid square, lower left corner

  if (loc.lat == 0.0) return;                       // ignore uninitialized lat/long

  float degreesX = loc.lng - origin.lng;            // longitude: distance from left edge of grid (degrees)
  float degreesY = loc.lat - origin.lat;            // latitude: distance from bottom edge of grid

  float fracGridX = degreesX / gridWidthDegrees;    // E-W position as fraction of grid width, 0-1
  float fracGridY = degreesY / gridHeightDegrees;   // N-S position as fraction of grid height, 0-1

  // our drawing canvas is the entire screen
  Point result;
  translateGPStoScreen(&result, loc, origin);
  if ((result.x != prevVehicle.x) || (result.y != prevVehicle.y)) {
    plotVehicle( prevVehicle, ILI9341_BLACK );      // erase old vehicle
    plotVehicle( result, ILI9341_CYAN );            // plot new vehicle
  }
  prevVehicle = result;
}
// ========== class ViewGrid
void ViewGrid::updateScreen() {
  // called on every pass through main()

  // coordinates of lower-left corner of currently displayed grid square
  PointGPS gridOrigin{ grid.nextGridLineSouth(model->gLatitude), grid.nextGridLineWest(model->gLatitude) };

  PointGPS myLocation{ model->gLatitude, model->gLongitude }; // current location
  
  char grid6[7];
  grid.calcLocator(grid6, model->gLatitude, model->gLongitude, 6);
  drawGridName(grid6);                // huge letters centered on screen
  drawAltitude();                     // height above sea level
  drawNumSatellites();
  drawTemperature(baroModel.getTemperature()); // query temperature from the C++ object, which is updated only by "performReading()"
  drawPositionLL(model->gLatitude, model->gLongitude);  // lat-long of current position
  //drawCompassPoints();              // show N-S-E-W compass points (disabled, it makes the screen too busy)
  //drawBoxLatLong();                 // show coordinates of box (disabled, it makes the screen too busy)
  drawNeighborGridNames();            // show 4-digit names of nearby squares
  drawNeighborDistances();            // this is the main goal of the whole project
  plotCurrentPosition(myLocation, gridOrigin);    // show current pushpin
  plotRoute(history, numHistory, gridOrigin);   // show route track
}

void ViewGrid::startScreen() {
  // called once each time this view becomes active
  this->clearScreen(this->background);                  // clear screen
  txtGrid[0].setBackground(this->background);           // set background for all TextFields in this view
  TextField::setTextDirty( txtGrid, numTextGrid );

  double lngMiles = grid.calcDistanceLong(model->gLatitude, 0.0, minLong, model->gMetric);
  if (logger.print_info) {
    Serial.print("Minimum visible E-W movement x=long="); 
    Serial.print(minLong,6); Serial.print(" degrees = "); 
    Serial.print(lngMiles,2); Serial.println(" miles");
  
    double latMiles = grid.calcDistanceLat(0.0, minLat, model->gMetric);
    Serial.print("Minimum visible N-S movement y=lat="); 
    Serial.print(minLat,6); Serial.print(" degrees = "); 
    Serial.print(latMiles,2); Serial.println(" miles");
  }

  setFontSize(12);
  drawGridOutline();                  // box outline around grid
  drawAllIcons();                     // draw gear (settings) and arrow (next screen)
  showDefaultTouchTargets();          // optionally draw boxes around button-touch area
  // showMyTouchTargets(gridButtons, nGridButtons);   // no buttons on this screen
  showScreenBorder();                 // optionally outline visible area

  updateScreen();                     // fill in values immediately, don't wait for the main loop to eventually get around to it
}

bool ViewGrid::onTouch(Point touch) {
  logger.info("->->-> Touched grid detail screen.");
  return false;                       // true=handled, false=controller uses default action
}
