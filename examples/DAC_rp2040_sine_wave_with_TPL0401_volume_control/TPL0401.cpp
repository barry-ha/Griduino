/**************************************************************************/
/*!
    @file     TPL0401.cpp
    @author   Barry Hansen K7BWH

    I2C Driver for Texas Instruments TPL0401 digital potentiometer

    @copyright This is in the public domain.
*/
/**************************************************************************/

#include "TPL0401.h"

/**************************************************************************/
/*!
    @brief  Instantiates a new TPL0401 class
*/
/**************************************************************************/
TPL0401::TPL0401() {}

/**************************************************************************/
/*!
    @brief  Setups the hardware and checks the digital potentiometer was found
    @param i2c_address The I2C address of the DAC, defaults to 0x2E
    @param wire The I2C TwoWire object to use, defaults to &Wire
    @returns True if DAC was found on the I2C address.
*/
/**************************************************************************/
bool TPL0401::begin(uint8_t i2c_address, TwoWire *wire) {
  if (i2c_dev) {
    delete i2c_dev;
  }

  i2c_dev = new Adafruit_I2CDevice(i2c_address, wire);

  if (!i2c_dev->begin()) {
    return false;
  }

  return true;
}

/**************************************************************************/
/*!
    @brief  Sets the output voltage to a fraction of source vref.  (Value
            can be 0..127)

    @param[in]  output
                The 7-bit value representing the relationship between the
                digital potentiometer's input voltage and its output voltage.
    @param i2c_frequency What we should set the I2C clock to when writing
                to the DAC, defaults to 400 KHz
    @returns True if able to write the value over I2C
*/
/**************************************************************************/
bool TPL0401::setWiper(uint16_t output, uint32_t i2c_frequency) {
  i2c_dev->setSpeed(i2c_frequency);   // Set I2C frequency to desired speed

  uint8_t packet[3];

  packet[0] = TPL0401_CMD_WRITEDAC;
  packet[1] = output;   // wiper position
  packet[2] = 0;        // unused

  if (!i2c_dev->write(packet, 2)) {
    return false;
  }

  i2c_dev->setSpeed(100000);   // reset I2C clock to default
  return true;
}
