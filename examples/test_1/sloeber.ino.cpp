#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2020-06-01 23:40:51

#include "Arduino.h"
#include <stdint.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "TouchScreen.h"
#include "sm.h"
#include "point.h"
#include "sm_button.h"

static void mapTouchToScreen(TSPoint touch, Point* screen) ;
void btn_thread_start(void) ;
btn_status_t btn_thread(void) ;
static void SplashScreen(void) ;
static void clearScreen() ;
static void showActivityBar(int row, uint16_t foreground, uint16_t background) ;
void setup(void) ;
void loop(void) ;

#include "test_1.ino"


#endif
