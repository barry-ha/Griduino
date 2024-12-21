// Please format this file with clang before check-in to GitHub
/*
  File:     morse_dac.cpp

  Date:     2019-12-30 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Generate Morse Code through a speaker using onboard
            DAC (digital to analog converter) that plays a pure
            tone from a waveform table.

            All input should be uppercase.
            Prosigns (SK, KN, etc) have special character values #defined.

  See also:
            Non-blocking Morse by Mark Fickett, KB3JCY
                https://github.com/markfickett/arduinomorse
            Morse decoder (using binary tree):
                http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1289074596/15
            Generator (on playground):
                http://www.arduino.cc/playground/Code/Morse
*/

#include <Arduino.h>     // for max() function
#include <stdlib.h>      // for dtostrf() function
#include "logger.h"      // conditional printing to Serial port
#include "morse_dac.h"   // Morse sending class

#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
// todo - for now, RP2040 has no DAC, no audio, no speech
#else
// ========== extern ===========================================
extern Logger logger;   // Griduino.ino

// ------------ definitions
#define N_MORSE (sizeof(morsetable) / sizeof(morsetable[0]))

// Set granularity of waveform
const int sizeWavetable = 24;
unsigned int waveform[sizeWavetable];   // waveform lookup table is computed once and saved thereafter

// ----- global variables
// none, all variables are member of the class

// Unit Test:
//      For smooth 700 Hz, a popular CW tone, with 50 steps, we should have:
//      dacSampleTime = 28.57;  // = 1E6 / 700 Hz / 50 samples
//      gStep = 0.1256;         // = 2pi / 50 samples
// Results:
//      Measured f = 634 Hz  with 50 samples
//          f = 667 Hz  with 25 samples
//          f = 680 Hz  with 15 samples
//          f = 687 Hz  with 13 samples
//          f = 630! Hz with 12 samples
//          f = 631! Hz with 11 samples
//          f = 689 Hz  with 10 samples
//          f = 621! Hz with  9 samples
// FYI: dot time = 1200 / wpm
//      dot time at 20 wpm is 60.00 msec
//      dot time at 18 wpm is 66.67 msec
//      dot time at 13 wpm is 92.31 msec

struct t_mtab {
  char c, pat;
};

struct t_mtab morsetable[] = {
    // read lsb-to-msb: 0=dit, 1=dah, msb is sentinel
    {'.', 0b1101010},
    {',', 0b10011011},
    {'?', 0b1001100},
    {'/', 0b101001},
    {'a', 0b110},
    {'b', 0b10001},
    {'c', 0b10101},
    {'d', 0b1001},
    {'e', 0b10},
    {'f', 0b10100},
    {'g', 0b1011},
    {'h', 0b10000},
    {'i', 0b100},
    {'j', 0b11110},
    {'k', 0b1101},
    {'l', 0b10010},
    {'m', 0b111},
    {'n', 0b101},
    {'o', 0b1111},
    {'p', 0b10110},
    {'q', 0b11011},
    {'r', 0b1010},
    {'s', 0b1000},
    {'t', 0b11},
    {'u', 0b1100},
    {'v', 0b11000},
    {'w', 0b1110},
    {'x', 0b11001},
    {'y', 0b11101},
    {'z', 0b10011},
    {'1', 0b111110},
    {'2', 0b111100},
    {'3', 0b111000},
    {'4', 0b110000},
    {'5', 0b100000},
    {'6', 0b100001},
    {'7', 0b100011},
    {'8', 0b100111},
    {'9', 0b101111},
    {'0', 0b111111},
    {PROSIGN_SK, 0b1101000},
    {PROSIGN_KN, 0b101101},
    {PROSIGN_BT, 0b110001},
    {PROSIGN_AS, 0b100010},
};

// Convert float to string
// Replacement 'dtostrf()' so we can print floating point varables
// From: https://forum.arduino.cc/index.php?topic=349764.15
// input: val = floating point number to format
//        width = minimum field width of the output string
//        prec = precision, number of digits after the decimal point
//        sout = output string
// todo:  replace this "dtostrf" with "floatToCharArray" in Griduino.ino
char *dtostrf(double val, signed char width, unsigned char prec, char *sout) {
  uint32_t iPart = (uint32_t)val;
  sprintf(sout, "%d", iPart);
  if (prec > 0) {
    uint8_t pos    = strlen(sout);
    sout[pos++]    = '.';
    uint32_t dPart = (uint32_t)((val - (double)iPart) * pow(10, prec));
    for (uint8_t i = (prec - 1); i > 0; i--) {
      size_t pow10 = pow(10, i);
      if (dPart < pow10) {
        sout[pos++] = '0';
      } else {
        sout[pos++] = '0' + dPart / pow10;
        dPart       = dPart % pow10;
      }
    }
    sout[pos++] = '0' + dPart;
    sout[pos]   = '\0';
  }
  return sout;
}

void DACMorseSender::setup() {
  // ----- DAC settings
  dacSampleTime      = 1E6 / fFrequency / sizeWavetable;   // microseconds for DAC to hold each sample
  double gStep       = (2.0 * PI) / sizeWavetable;         // radians to advance after each sample
  float waveDuration = 1.0 / fFrequency;                   //

  // multiplier, slightly less than half of 2^12, to prevent overflow from rounding errors
  const int dacAmplitude = 2040;
  const int dacOffset    = 2048;   // constant adder
                                   // = exactly half of 2^12
                                   // such that "dacOffset +/- gDacVolume" = "voltage range of DAC"

  // ----- Morse settings
  // Timing: at 18 WPM, each dit = 1200/wpm = 66.7 msec
  // Timing: at 13 WPM, each dit = 1200/wpm = 92.3 msec
  fDitDuration = (1.2 / max(5.0f, wpm));       // seconds per dit "on" time
  iDitDuration = (int)1000.0 * fDitDuration;   // msec    per dit "on" time
  letterSpace  = fDitDuration * 3;
  wordSpace    = fDitDuration * 5;

  cyclesPerDit = fDitDuration / waveDuration;       // waveforms per DIT
  cyclesPerDah = fDitDuration / waveDuration * 3;   // waveforms per DAH

  // ----- DAC waveform lookup table
  float twopi  = 2.0 * PI;
  double phase = 0.0;
  for (int ii = 0; ii < sizeWavetable; ii++) {
    waveform[ii] = dacAmplitude * sin(phase) + dacOffset;
    phase        = phase + gStep;
    if (phase > twopi) {
      // should not happen
      char msg[256];
      snprintf(msg, 256, "!!! Phase overflow in morse_dac.cpp [%d] on loop %d",
               __LINE__, ii);
      Serial.println(msg);
    }
  }
}

void DACMorseSender::send_dit() {
  float val;

  // send an entire waveform over and over until the DIT time is done
  for (int count = 0; count < cyclesPerDit; count++) {

    // step through one waveform
    for (int kk = 0; kk < sizeWavetable; kk++) {
      val = waveform[kk];   // lookup DAC setting from the table
      analogWrite(dacPin, (int)val);
      delayMicroseconds(dacSampleTime);
    }
  }
  send_dit_space();
}

void DACMorseSender::send_dah() {
  float val;

  // send an entire waveform over and over until the DAH time is done
  for (int count = 0; count < cyclesPerDah; count++) {

    // step through one waveform
    for (int kk = 0; kk < sizeWavetable; kk++) {
      val = waveform[kk];   // lookup DAC setting from the table
      analogWrite(dacPin, (int)val);
      delayMicroseconds(dacSampleTime);
    }
  }
  send_dit_space();
}

void DACMorseSender::send_dit_space() {
  analogWrite(dacPin, dacOffset);
  delay(iDitDuration);
}

void DACMorseSender::send_letter_space() {
  analogWrite(dacPin, dacOffset);
  int msec = (int)1000.0 * letterSpace;
  delay(msec);
}

void DACMorseSender::send_word_space() {
  analogWrite(dacPin, dacOffset);
  int msec = (int)1000.0 * wordSpace;
  delay(msec);
}

char getPattern(char c) {
  for (int ii = 0; ii < N_MORSE; ii++) {
    if (morsetable[ii].c == c) {
      return morsetable[ii].pat;
    }
  }
  // character not found in morse code table - should not happen
  char cc[2];
  snprintf(cc, sizeof(cc), "%c", c);
  logger.log(AUDIO, ERROR, "!!! character '%s' not found in morsetable[]", cc);
  return 0b1001100;   // = "?"
}

void DACMorseSender::send(char c) {
  if (c == ' ') {
    send_word_space();
  } else {
    char p = getPattern(c);
    while (p != 1) {
      if (p & 1) {
        send_dah();
      } else {
        send_dit();
      }
      p = p / 2;
    }
  }
  send_letter_space();
}

void DACMorseSender::setMessage(const String newMessage) {
  message = newMessage;
}

void DACMorseSender::sendBlocking() {
  logger.logFloat(AUDIO, INFO, "Sending morse %s wpm", wpm, 1);
  if (dacSampleTime < 1) {
    // note "delayMicroseconds()" only works reliably down to 3 usec
    logger.log(AUDIO, ERROR, "DAC dacSampleTime < 1 usec. Did you call setup()?");
  }

  for (int ii = 0; ii < message.length(); ii++) {
    send(message.charAt(ii));
  }
  send_word_space();
}

void DACMorseSender::dump() {
  // dump our guts to the IDE console for debugging
  char msg[256];
  char sFloat[16];
  if (dacSampleTime < 1) {
    // note "delayMicroseconds()" only works reliably down to 3 usec
    Serial.println("!!! DAC dacSampleTime < 1 usec. Did you call setup()?");
  }
}

void DACMorseSender::unit_test() {
  // unit test routine (todo - not currently called from anywhere, 2022-08)
  char msg[256];
  logger.log(AUDIO, INFO, "Begin DAC Morse settings:");
  logger.log(AUDIO, INFO, ". DAC sizeWavetable(%d)", sizeWavetable);
  logger.log(AUDIO, INFO, ". DAC dacPin(%d)", dacPin);
  logger.log(AUDIO, INFO, ". DAC dacAmplitude(%d)", dacAmplitude);
  logger.log(AUDIO, INFO, ". DAC dacOffset(%d)", dacOffset);
  logger.log(AUDIO, INFO, ". DAC fFrequency(%s Hz)", fFrequency, 1);
  logger.log(AUDIO, INFO, ". DAC sizeWavetable(%d samples)", sizeWavetable);
  logger.log(AUDIO, INFO, ". DAC dacSampleTime(%s usec)", dacSampleTime, 1);
  logger.log(AUDIO, INFO, ". Morse WPM(%s)", wpm, 1);
  logger.log(AUDIO, INFO, ". Morse fDitDuration(%s sec)", fDitDuration, 5);
  logger.log(AUDIO, INFO, ". Morse iDitDuration(%d msec)", iDitDuration);
  logger.log(AUDIO, INFO, ". Morse letterSpace(%s sec)", letterSpace, 5);
  logger.log(AUDIO, INFO, ". Morse wordSpace float(%s sec)", wordSpace, 3);
  int msec = (int)1000.0 * wordSpace;
  logger.log(AUDIO, INFO, ". Morse wordSpace int (%d msec)", msec);
  logger.log(AUDIO, INFO, ". Morse cyclesPerDit(%d)", cyclesPerDit);
  logger.log(AUDIO, INFO, ". Morse cyclesPerDah(%d)", cyclesPerDah);
  logger.log(AUDIO, INFO, "End settings.");
}
#endif   // ARDUINO_ADAFRUIT_FEATHER_RP2040
