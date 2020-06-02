//------------------------------------------------------------------------------
//	The MIT License (MIT)
//
//	Copyright (c) 2020 A.C. Verbeck
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.
//------------------------------------------------------------------------------
#include <stdint.h>

#include "SPI.h"																// Serial Peripheral Interface
#include "Adafruit_GFX.h"														// Core graphics display library
#include "Adafruit_ILI9341.h"													// TFT color display library
#include "TouchScreen.h"														// Touchscreen built in to 3.2" Adafruit TFT display

#include "sm.h"
#include "point.h"
#include "sm_button.h"

//------------------------------------------------------------------------------
//	Startup information
//------------------------------------------------------------------------------
#define PROGRAM_TITLE		"Touch Screen Demo"
#define PROGRAM_VERSION		"v0.9"
#define PROGRAM_LINE1		"Barry K7BWH"
#define PROGRAM_LINE2		"ACV"
#define PROGRAM_COMPILED	__DATE__ " " __TIME__

//------------------------------------------------------------------------------
//	Screen Rotation:
//	1 - landscape
//	2 - right edge is top
//	3 - landscape inverted
//	4 - left edge is top
//
//	Note: TouchScreen is aligned only for landscape (1)
//------------------------------------------------------------------------------
#define SCREEN_ROTATION		1

//------------------------------------------------------------------------------
//	Screen touch pressure
//------------------------------------------------------------------------------
#define TOUCHPRESSURE		100													// Minimum pressure we consider true pressing

//------------------------------------------------------------------------------
//	TFT display and SD card share the hardware SPI interface, and have
//	separate 'select' pins to identify the active device on the bus.
//
//	Adafruit Feather M4 Express pin definitions
//	To compile for Feather M0/M4, install "additional boards manager"
//	https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51/setup
//------------------------------------------------------------------------------
#if defined(SAMD_SERIES)
	#define TFT_BL	 4															// TFT backlight
	#define TFT_CS	 5															// TFT chip select pin
	#define TFT_DC	12															// TFT display/command pin
#elif defined(ARDUINO_AVR_MEGA2560)
	#define TFT_BL	 6															// TFT backlight
	#define TFT_DC	 9															// TFT display/command pin
	#define TFT_CS	10															// TFT chip select pin
#else
	#warning You need to define pins for your hardware
#endif

//------------------------------------------------------------------------------
//	Touch Screen
//	For touch point precision, we need to know the resistance
//	between X+ and X- Use any multimeter to read it
//	The demo program used 300 ohms across the X plate
//	Barry's display, ILI-9341, measured 295 ohms across the X plate.
//------------------------------------------------------------------------------
#if defined(SAMD_SERIES)														// Adafruit Feather M4 Express pin definitions
	#define PIN_XP	A3															// Touchscreen X+ can be a digital pin
	#define PIN_XM	A4															// Touchscreen X- must be an analog pin, use "An" notation
	#define PIN_YP	A5															// Touchscreen Y+ must be an analog pin, use "An" notation
	#define PIN_YM	 9															// Touchscreen Y- can be a digital pin
#elif defined(ARDUINO_AVR_MEGA2560)												// Arduino Mega 2560 and others
	#define PIN_XP	 4															// Touchscreen X+ can be a digital pin
	#define PIN_XM	A3															// Touchscreen X- must be an analog pin, use "An" notation
	#define PIN_YP	A2															// Touchscreen Y+ must be an analog pin, use "An" notation
	#define PIN_YM	 5															// Touchscreen Y- can be a digital pin
#else
	#warning You need to define pins for your hardware
#endif

//------------------------------------------------------------------------------
//	Screen layout
//	When using default system fonts, screen pixel coordinates will identify
//	top left of character cell
//------------------------------------------------------------------------------
// splash screen layout
//------------------------------------------------------------------------------
#define yRow1	0				  												// title: "Touchscreen Demo"
#define yRow2	yRow1 + 40		  												// program version
#define yRow3	yRow2 + 20		  												// compiled date
#define yRow4	yRow3 + 20		  												// author line 1
#define yRow5	yRow4 + 20		  												// author line 2
#define yRow6	yRow5 + 40		  												// "Pressure threshold = "
#define yRow7	yRow6 + 40		  												// x / y pressure

//------------------------------------------------------------------------------
//	Color scheme
//	RGB 565 color code: http://www.barth-dev.de/online/rgb565-color-picker/
//------------------------------------------------------------------------------
#define cBACKGROUND			0x00A			  									// 0, 0,   10 = darker than ILI9341_NAVY, but not black
#define cTEXTCOLOR			ILI9341_CYAN	  									// 0, 255, 255
#define cTEXTFAINT			0x514			  									// 0, 160, 160 = blue, between CYAN and DARKCYAN
#define cLABEL				ILI9341_GREEN
#define cINPUT				ILI9341_WHITE
#define cBUTTONFILL			ILI9341_NAVY
#define cBUTTONOUTLINE		ILI9341_BLUE										// 0,	  0, 255 = darker than cyan
#define cWARN				0xF844												// brighter than ILI9341_RED but not pink

//------------------------------------------------------------------------------
//	System event defines
//------------------------------------------------------------------------------
#define	SYS_EVT_BUTTON		(1ul<<0)

//------------------------------------------------------------------------------
//	Local data
//------------------------------------------------------------------------------
static Adafruit_ILI9341	tft = Adafruit_ILI9341(TFT_CS, TFT_DC);					// create an instance of the TFT Display
static TouchScreen		ts = TouchScreen(PIN_XP, PIN_YP, PIN_XM, PIN_YM, TOUCHPRESSURE, 295);
static const int		xLabel = 8;												// indent labels, slight margin on left edge of screen
static uint32_t			gScreenRotation = SCREEN_ROTATION;						// default screen rotation
static uint32_t			sys_events = 0ul;

//------------------------------------------------------------------------------
//	Function: mapTouchToScreen
//
//	Uses pressure reading to determine touch state.
//	isTouching() is defined in TouchScreen.h but is not implemented
//	in Adafruit's TouchScreen library (grrr)
//
// convert from X+,Y+ resistance measurements to screen coordinates
// param touch = resistance readings from touchscreen
// param screen = result of converting touch into screen coordinates
//
// Measured readings in Barry's landscape orientation were:
//	 +---------------------+ X=876
//	 |					   |
//	 |					   |
//	 |					   |
//	 +---------------------+ X=160
//	Y=110				 Y=892
//
// Typical measured pressures=200..549
//
//	Parameters
//		touch -- incoming touch value
//		screen -- output to screen
//
//	Returns
//		None
//
//------------------------------------------------------------------------------
static void mapTouchToScreen(TSPoint touch, Point* screen)
{
	Point tmpPoint;

	screen->x = map(touch.y,  225,825,		0, tft.width());
	screen->y = map(touch.x,  800,300,		0, tft.height());

	switch (gScreenRotation) {
	case 1:																		//	if display is normal, no translation required
		break;

	case 2:																		//	90-degree rotation: swap x / y
		tmpPoint = *screen;
		screen->y = tft.height() - tmpPoint.x;
		screen->x = tft.width() - tmpPoint.y;
		break;

	case 3:																		// if display is flipped, then also flip both x,y touchscreen coords
		screen->x = tft.width() - screen->x;
		screen->y = tft.height() - screen->y;
		break;

	case 4:																		//
		screen->y = tft.width() - screen->x;
		screen->x = tft.height() - screen->y;
		break;

	default:
		break;
	}
}

//------------------------------------------------------------------------------
//	Function: btn_thread_start
//	The button thread should not be started before initialization
//	is complete.
//
//	Parameters
//		None
//
//	Returns
//		None
//
//------------------------------------------------------------------------------
static bool			btn_thread_enable = false;
static Point		touch;

void btn_thread_start(void)
{
	btn_init();
	btn_thread_enable = true;
}

//------------------------------------------------------------------------------
//	Function: btn_thread
//	Button thread manages all the button processing.
//	Important: this task must not be run from interrupt context.
//
//	Parameters
//		None
//
//	Returns
//		Current button status
//
//------------------------------------------------------------------------------
btn_status_t btn_thread(void)
{
	static bool ts_curr_state = false;
	static bool ts_old_state = false;
	btn_status_t ret_val;
	BTN_EVENT_T  btn_event;
	TSPoint		 ptRaw;

	if (btn_thread_enable == false) return btn_idle;							//	Don't run the task

	ts_curr_state = ts.isTouching();

	if (ts_curr_state != ts_old_state)
    {
		btn_event = ts_curr_state ? EVENT_BUTTON_DN : EVENT_BUTTON_UP;
        btn_process(btn_event);
        if (btn_event == EVENT_BUTTON_DN) {
			ptRaw = ts.getPoint();												// TouchScreen point object has (x,y,z) coordinates, where z = pressure
			mapTouchToScreen(ptRaw, &touch);									// convert resistance measurements into screen pixel coordinates
        }
        ts_old_state = ts_curr_state;
    }
    btn_process(EVENT_100MS);
    ret_val = btn_status_get();
    btn_status_reset();

    return ret_val;
}

//------------------------------------------------------------------------------
//	Function: sysTickHook
//	This hooks the SysTick interrupt.
//	In arduino, this is set to execute at 1kHz
//
//	Important: This is called from interrupt context.
//
//	Parameters
//		None
//
//	Returns
//		Current button status
//
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

int sysTickHook()
{
	static uint32_t ctr=0ul;

	ctr += 1;
	if (ctr == 90) {
		sys_events |= SYS_EVT_BUTTON;
		ctr = 0;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

//------------------------------------------------------------------------------
//	Function: SplashScreen
//	Displays the splash screen.
//
//	Parameters
//		None
//
//	Returns
//		None
//
//------------------------------------------------------------------------------
static void SplashScreen(void)
{
	tft.setTextSize(2);

	tft.setCursor(xLabel, yRow1);												//	Program Title
	tft.setTextColor(cTEXTCOLOR);
	tft.print(PROGRAM_TITLE);

	tft.setCursor(xLabel, yRow2);												//	Program Version
	tft.setTextColor(cLABEL);
	tft.print(PROGRAM_VERSION);

	tft.setCursor(xLabel, yRow3);												//	Date / Time
	tft.print(__DATE__ " " __TIME__);

	tft.setCursor(xLabel, yRow4);												//	Line one (Barry)
	tft.println(PROGRAM_LINE1);

	tft.setCursor(xLabel, yRow5);												//	Line two (others)
	tft.println(PROGRAM_LINE2);
}

//------------------------------------------------------------------------------
//	Function: clearScreen
//	Clears the screen.
//
//	Parameters
//		None
//
//	Returns
//		None
//
//------------------------------------------------------------------------------
static void clearScreen()
{
	tft.fillScreen(cBACKGROUND);
}

//------------------------------------------------------------------------------
//	Function: showActivityBar
//	Displays activity bar on the last line.
//
//	Parameters
//		row        - row to display activity bar
//		foreground - foreground color
//		background - background color
//
//	Returns
//		None
//
//------------------------------------------------------------------------------
static void showActivityBar(int row, uint16_t foreground, uint16_t background)
{
	static int addDotX = 10;													// current screen column, 0..319 pixels
	static int rmvDotX = 0;
	static int count = 0;
	const int SCALEF = 512;														// how much to slow it down so it becomes visible

	count = (count + 1) % SCALEF;
	if (count == 0) {
		addDotX = (addDotX + 1) % tft.width();	  								// advance
		rmvDotX = (rmvDotX + 1) % tft.width();									// advance
		tft.drawPixel(addDotX, row, foreground);								// write new
		tft.drawPixel(rmvDotX, row, background);								// erase old
	}
}

//------------------------------------------------------------------------------
//	Function: showActivityBar
//	Displays activity bar on the last line.
//
//	Parameters
//		row        - row to display activity bar
//		foreground - foreground color
//		background - background color
//
//	Returns
//		None
//
//------------------------------------------------------------------------------
void setup(void)
{
	Serial.begin(115200);														// start serial port
	Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);							// Report our program name to console
	Serial.println("Compiled " PROGRAM_COMPILED);								// Report our compiled date
	Serial.println(__FILE__);													// Report our source code file name

	// ----- init TFT backlight
	pinMode(TFT_BL, OUTPUT);													// set backlight pin
	analogWrite(TFT_BL, 255);													// start at full brightness

	// ----- init TFT display
	tft.begin();																// initialize TFT display
	tft.setRotation(gScreenRotation);											// landscape (default is portrait)
	clearScreen();																// clear the screen

	// ----- announce ourselves
	SplashScreen();

	// ----- initialize button state machine
	btn_thread_start();
}

//=========== main work loop ===================================
void loop(void)
{
	if (sys_events & SYS_EVT_BUTTON) {
	    switch (btn_thread())
	    {
		case btn_idle:
			break;

		case btn_click:
			Serial.println("btn_click");
			tft.fillCircle(touch.x, touch.y, 3, ILI9341_YELLOW);
			break;

		case btn_press:
			Serial.println("btn_press");
			clearScreen();
			SplashScreen();
			break;

		case btn_long_press:
			Serial.println("btn_long_press");
			gScreenRotation = (gScreenRotation==1) ? 3 : 1;
			tft.setRotation(gScreenRotation);											// landscape (default is portrait)
			clearScreen();
			SplashScreen();
			break;

		case btn_double_click:
			Serial.println("btn_double_click");
			tft.fillCircle(touch.x, touch.y, 3, ILI9341_MAGENTA);
			break;

		case btn_click_press:
			Serial.println("btn_click_press");
			break;

		case btn_click_long_press:
			Serial.println("btn_click_long_press");
			break;

		default:
			Serial.println("unknown");
			break;
	    }
	}
	sys_events = 0;

	showActivityBar(239, ILI9341_RED, ILI9341_BLACK);							//	activity bar
}
