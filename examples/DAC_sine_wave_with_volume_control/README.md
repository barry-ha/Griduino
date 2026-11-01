# DAC Sine Wave with Volume Control

This example plays a steady 800 Hz sine wave through the Griduino speaker and slowly steps the volume up using the DS1804 digital potentiometer. 

This sketch is a quick way to check the audio chain on a Griduino board: DAC, digital pot, LM386 amplifier and speaker.

The sketch has no user inputs. Plug a speaker into the Griduino jack for its output. It shows its status on the TFT display and the Serial Monitor.

![DAC sine wave, 2020-03-14](2020-03-14%20DAC%20Sine%20Wave%20img6245.jpg)

## Hardware

* Adafruit Feather M4 Express (SAMD51)
* Griduino PCB v4 with the DS1804 volume control
* ILI9341 TFT 320x240 display, https://www.adafruit.com/product/1743
* LM386 audio amplifier
* Speaker, such as https://www.adafruit.com/product/3351 

Pin assignments are in [hardware.h](hardware.h). The pins we used are:

| Signal | Pin | Purpose |
|---|---|---|
| DAC_PIN | A0 (DAC0) | sine wave output to the amplifier |
| PIN_VCS | A1 | DS1804 chip select |
| PIN_VUD | A2 | DS1804 up/down |
| PIN_VINC | 6 (Feather) or 2 (ItsyBitsy) | DS1804 increment |
| TFT_CS, TFT_DC, TFT_BL | 5, 12, 4 | display |

## Libraries

Install these with the Arduino Library Manager:

* [Adafruit_ILI9341](https://github.com/adafruit/Adafruit_ILI9341) by Adafruit
* [DS1804](https://github.com/RobTillaart/DS1804) by Rob Tillaart
* [elapsedMillis](https://github.com/pfeerick/elapsedMillis/wiki) by Paul Stoffregen

## What it does

1. Shows a splash screen with the program name, version, compile date and pitch.
2. Sets the DS1804 wiper to 0 (silent).
3. Writes sine wave samples to the 12-bit DAC, 25 samples per cycle at 800 Hz.
4. Periodically raises the wiper by one step.
5. When the wiper reaches `maxVolume` (40), it goes back to 0 and starts over.

You should hear the tone get louder in small steps, then go quiet and repeat.

## Settings you can change

All of these are near the top of the .ino file:

| Setting | Description |
|---|---|
| `gFrequency` = 800.0 | pitch of the tone, in Hz |
| `gSamplesPerWaveform` = 25.0 | DAC samples in each cycle; more samples make a smoother wave but limit the highest pitch |
| `gDacVolume` = 1200 | peak DAC amplitude around the 2048 midpoint; this was picked by ear so the LM386 does not clip |
| `maxVolume` = 40 | highest wiper position, out of 0..99; the speaker starts to sound distorted around 26 |
| `VOLUME_CHANGE_INTERVAL` = 600 | milliseconds between volume steps |

## Notes

* The DS1804 can't report its wiper position, so the sketch keeps track of it in `gWiperNow`. Each volume change moves the wiper only by the difference from where it is now. Moving it back to zero first would make the volume drop out on every step.
* The sine wave stops while the sketch updates the display and console, which can cause a small click at each step. The messages are kept short so the gap is as small as possible.
* The DS1804 has linear steps of resistance, so each step does not sound like the same change in loudness. To change volume by about 3 dB you would roughly double or halve the resistance. This sketch steps by 1 so you can hear every position.

## Credits

* Software: Barry Hansen, K7BWH
* Hardware: John Vanderbeck, KM7O
