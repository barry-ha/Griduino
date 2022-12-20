/**************************************************************************/
/*!
    @file     TPL0401.h
    @author   Barry Hansen K7BWH

    I2C Driver for Texas Instruments TPL0401 digital potentiometer

    @copyright This is in the public domain.
*/
/**************************************************************************/

#pragma once

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Wire.h>

#define TPL0401A_I2CADDR_DEFAULT (0x2E)   ///< Default i2c address
#define TPL0401B_I2CADDR_DEFAULT (0x3E)   ///< Default i2c address
#define TPL0401_CMD_WRITEDAC     (0x00)   ///< Writes data to the DAC

/**************************************************************************/
/*!
    @brief  Class for communicating with an MCP4725 DAC
*/
/**************************************************************************/
class TPL0401 {
public:
  TPL0401();
  bool begin(uint8_t i2c_address = TPL0401A_I2CADDR_DEFAULT,
             TwoWire *wire       = &Wire);
  bool setWiper(uint16_t output, uint32_t dac_frequency = 400000);

private:
  Adafruit_I2CDevice *i2c_dev = NULL;
};
