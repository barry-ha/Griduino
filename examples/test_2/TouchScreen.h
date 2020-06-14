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
#pragma once
#include <stdint.h>

typedef volatile uint32_t RwReg;

#if defined(__AVR__) || defined(TEENSYDUINO) || defined(ARDUINO_ARCH_SAMD)
#define USE_FAST_PINIO
#endif

//------------------------------------------------------------------------------
//	class: TSPoint
//	Encapsulates touch screen point
//------------------------------------------------------------------------------
class TSPoint {
public:
	TSPoint(void);
	TSPoint(int16_t x, int16_t y, int16_t z);

	bool operator==(TSPoint);
	bool operator!=(TSPoint);

	int16_t x;																	//	x-axis value
	int16_t y;																    //	y-axis value
    int16_t	z;																	//	x-axis value
};

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
class TouchScreen {
public:
	TouchScreen(uint8_t xp, uint8_t yp, uint8_t xm, uint8_t ym, uint16_t tv, uint16_t rx);
	TSPoint		getPoint(void);
	void		setThreshold(uint16_t tv);
	uint16_t	getThreshold(void);
	uint16_t	readPressure(void);
	bool		isTouching(void);

private:
	uint8_t		_yp, _ym, _xm, _xp;
	uint16_t	_rxplate;
	uint16_t	_pres;

	volatile RwReg *xp_port, *yp_port, *xm_port, *ym_port;
	RwReg xp_pin, xm_pin, yp_pin, ym_pin;
};
