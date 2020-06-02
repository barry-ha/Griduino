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
//	Original Notes:
//	Touch screen library with X Y and Z (pressure) readings as well
//	as oversampling to avoid 'bouncing'
//	(c) ladyada / adafruit
//	Code under MIT License
//------------------------------------------------------------------------------
#include "Arduino.h"
#include "pins_arduino.h"
#include "TouchScreen.h"

#define	NUMSAMPLES		4

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//	class: TSPoint
//	Encapsulates touch screen point
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//	First constructor:
//	No parameters.
//------------------------------------------------------------------------------
TSPoint::TSPoint(void) { x = y = z = 0; }

//------------------------------------------------------------------------------
//	Second constructor:
//	All three values provided.
//------------------------------------------------------------------------------
TSPoint::TSPoint(int16_t x0, int16_t y0, int16_t z0)
{
	x = x0;
	y = y0;
	z = z0;
}

//------------------------------------------------------------------------------
//	Operator == equal
//
//	Check if the current point is equivalent to another point
//	p1 The point being checked for equivalence
//
//	Returns true if the points are equal
//------------------------------------------------------------------------------
bool TSPoint::operator==(TSPoint p1)
{
  return ((p1.x == x) && (p1.y == y) && (p1.z == z));
}

//------------------------------------------------------------------------------
//	Operator != equal
//
//	Check if the current point is not equivalent to another point
//	p1 The point being checked for equivalence
//
//	Returns true if the points are not equal
//------------------------------------------------------------------------------
bool TSPoint::operator!=(TSPoint p1)
{
  return ((p1.x != x) || (p1.y != y) || (p1.z != z));
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//	class: TouchScreen
//	Encapsulates touch screen
//
//	Constructor takes the following parameters:
//	xp:	X+ pin. Must be an analog pin
//	yp:	Y+ pin. Must be an analog pin
//	xm:	X- pin. Can be a digital pin
//	ym:	Y- pin. Can be a digital pin
//	rv: Pressure Threshold
//	rx:	The resistance in ohms between X+ and X- to calibrate pressure sensing
//------------------------------------------------------------------------------
TouchScreen::TouchScreen(uint8_t xp, uint8_t yp, uint8_t xm, uint8_t ym, uint16_t tv = 0, uint16_t rxplate = 0)
{
	_yp = yp;
	_xm = xm;
	_ym = ym;
	_xp = xp;
	_pres = tv;
	_rxplate = rxplate;

	xp_port = portOutputRegister(digitalPinToPort(_xp));
	yp_port = portOutputRegister(digitalPinToPort(_yp));
	xm_port = portOutputRegister(digitalPinToPort(_xm));
	ym_port = portOutputRegister(digitalPinToPort(_ym));
	
	xp_pin = digitalPinToBitMask(_xp);
	yp_pin = digitalPinToBitMask(_yp);
	xm_pin = digitalPinToBitMask(_xm);
	ym_pin = digitalPinToBitMask(_ym);
}

//------------------------------------------------------------------------------
//	Set / Get for pressure threshold
//------------------------------------------------------------------------------
void TouchScreen::setThreshold(uint16_t tv)
{
	_pres = tv;
}

uint16_t TouchScreen::getThreshold(void)
{
	return _pres;
}

//------------------------------------------------------------------------------
//	Measure the X, Y, and pressure and return a TSPoint with the measurements
//	
//	TSPoint The measured X, Y, and Z/pressure values
//------------------------------------------------------------------------------
TSPoint TouchScreen::getPoint(void)
{
	int x=0, y=0, z=0;
	uint32_t sample;
	uint8_t i;

	pinMode(_yp, INPUT);														//	prepare to read x value
	pinMode(_ym, INPUT);
	pinMode(_xp, OUTPUT);
	pinMode(_xm, OUTPUT);
	*xp_port |= xp_pin;
	*xm_port &= ~xm_pin;
	delayMicroseconds(20);

	sample = 0;																	//	read x values
	for (i = 0; i < NUMSAMPLES; i++) {
		sample += analogRead(_yp);
	}
	x = (1023 - sample/NUMSAMPLES);												//	calculate x value

	pinMode(_xp, INPUT);														//	prepare to read y value
	pinMode(_xm, INPUT);
	pinMode(_yp, OUTPUT);
	pinMode(_ym, OUTPUT);
	*ym_port &= ~ym_pin;
	*yp_port |= yp_pin;
	delayMicroseconds(20);

	sample = 0;																	//	read x values
	for (i = 0; i < NUMSAMPLES; i++) {
		sample += analogRead(_xm);
	}
	y = (1023 - sample/NUMSAMPLES);												//	calculate x value

	return TSPoint(x, y, z);
}

//------------------------------------------------------------------------------
//	Function: readPressure
//	Configure resistive touch-screen to read from x direction, read x pressure
//	Configure resistive touch-screen to read from y direction, read y pressure
//	Average values together and return.
//
//	Parameters
//		None
//
//	Returns
//		Average value of the pressure
//
//------------------------------------------------------------------------------
uint16_t TouchScreen::readPressure(void)
{
	int z1, z2;

	pinMode(_xp, OUTPUT);														//	Set X+ to ground
	digitalWrite(_xp, LOW);
	pinMode(_ym, OUTPUT);														// Set Y- to VCC
	digitalWrite(_ym, HIGH);

	digitalWrite(_xm, LOW);														// Hi-Z X-
	pinMode(_xm, INPUT);
	digitalWrite(_yp, LOW);														// Hi-Z Y+
	pinMode(_yp, INPUT);

	z1 = analogRead(_xm);
	z2 = 1023-analogRead(_yp);

	return (uint16_t)((z1+z2)/2);
}

//------------------------------------------------------------------------------
//	Function: TouchScreen::isTouching
//	Uses pressure reading to determine touch state.
//	isTouching() is defined in TouchScreen.h but is not implemented
//	in Adafruit's TouchScreen library (grrr)
//
//	Parameters
//		None
//
//	Returns
//		Screen touch status
//
//------------------------------------------------------------------------------
bool TouchScreen::isTouching(void)
{
	static bool button_state = false;
	uint16_t pres_val = readPressure();

	if ((button_state == false) && (pres_val > _pres)) {
		button_state = true;
	}

	if ((button_state == true) && (pres_val < _pres)) {
		button_state = false;
	}

	return button_state;
}

