// Please format this file with clang before check-in to GitHub
/*
  File:     morse_dac.cpp

  Date:     2026-11-25 updated for non-blocking Morse code
            2019-12-30 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Generate Morse Code through a speaker using onboard
            DAC (digital to analog converter) that plays a pure
            tone from a waveform table.

            sendBlocking() is the original blocking method.
            startSending() is non-blocking: a TC4 timer interrupt at 16 kHz
            produces each DAC sample from a sine table (phase accumulator,
            so any audio frequency works) and also counts samples to time
            the dits, dahs and spaces. Each element fades in and out over
            5 msec to avoid key clicks.
            We avoid timer TC3 because it is used by the TFT backlight PWM.

            All input should be uppercase.
            Prosigns (SK, KN, etc) have special character values #defined.

  Hardware:
            Hard-coded to DAC0 (pin A0) for both blocking and non-blocking output.

  See also:
            Non-blocking Morse by Mark Fickett, KB3JCY
                https://github.com/markfickett/arduinomorse
            Morse decoder (using binary tree):
                http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1289074596/15
            Generator (on playground):
                http://www.arduino.cc/playground/Code/Morse
            Wave Player using ISR
                https://github.com/lrusso/ArduinoWavePlayer/tree/master/Sample
*/

#include <Arduino.h>     // for max() function
#include <stdlib.h>      // for dtostrf() function
#include "logger.h"      // conditional printing to Serial port
#include "morse_dac.h"   // Morse sending class

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
  sprintf(sout, "%ld", iPart);
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
  DACMorseSender::stop();   // don't let the ISR and blocking code both drive the DAC
  float val;

  // send an entire waveform over and over until the DIT time is done
  for (int count = 0; count < cyclesPerDit; count++) {

    // step through one waveform
    for (int kk = 0; kk < sizeWavetable; kk++) {
      val = waveform[kk];   // lookup DAC setting from the table
      analogWrite(DAC0, (int)val);
      delayMicroseconds(dacSampleTime);
    }
  }
  send_dit_space();
}

void DACMorseSender::send_dah() {
  DACMorseSender::stop();   // don't let the ISR and blocking code both drive the DAC
  float val;

  // send an entire waveform over and over until the DAH time is done
  for (int count = 0; count < cyclesPerDah; count++) {

    // step through one waveform
    for (int kk = 0; kk < sizeWavetable; kk++) {
      val = waveform[kk];   // lookup DAC setting from the table
      analogWrite(DAC0, (int)val);
      delayMicroseconds(dacSampleTime);
    }
  }
  send_dit_space();
}

void DACMorseSender::send_dit_space() {
  analogWrite(DAC0, dacOffset);
  delay(iDitDuration);
}

void DACMorseSender::send_letter_space() {
  analogWrite(DAC0, dacOffset);
  int msec = (int)1000.0 * letterSpace;
  delay(msec);
}

void DACMorseSender::send_word_space() {
  analogWrite(DAC0, dacOffset);
  int msec = (int)1000.0 * wordSpace;
  delay(msec);
}

char getPattern(char c) {
  for (unsigned int ii = 0; ii < N_MORSE; ii++) {
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
  DACMorseSender::stop();   // don't let the ISR and blocking code both drive the DAC
  logger.logFloat(AUDIO, INFO, "Sending morse %s wpm", wpm, 1);
  if (dacSampleTime < 1) {
    // note "delayMicroseconds()" only works reliably down to 3 usec
    logger.log(AUDIO, ERROR, "DAC dacSampleTime < 1 usec. Did you call setup()?");
  }

  for (unsigned int ii = 0; ii < message.length(); ii++) {
    send(message.charAt(ii));
  }
  send_word_space();
}

void DACMorseSender::dump() {
  // dump our guts to the IDE console for debugging
  if (dacSampleTime < 1) {
    // note "delayMicroseconds()" only works reliably down to 3 usec
    Serial.println("!!! DAC dacSampleTime < 1 usec. Did you call setup()?");
  }

  logger.log(AUDIO, INFO, "Begin DAC Morse settings:");
  logger.log(AUDIO, INFO, ". DAC sizeWavetable(%d)", sizeWavetable);
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

// =============================================================
//    Non-blocking sender, driven by TC4 timer interrupt
// =============================================================
// Everything below this line runs on SAMD51 only (Feather M4).
// The ISR touches only the DAC and the variables in this section.
// It never uses Serial, String, SPI, flash, or the heap.

#if !defined(__SAMD51__)
#error "Non-blocking Morse requires a SAMD51 processor (e.g. Adafruit Feather M4)"
#endif

namespace isr {

const uint32_t SAMPLE_RATE  = 16000;                    // Hz, timer interrupt rate
const uint32_t TIMER_CLOCK  = 48000000;                 // Hz, GCLK1 on Adafruit SAMD51 core
const int SINE_SIZE         = 256;                      // entries in sine table (indexed by top 8 bits of phase)
const int RAMP_SAMPLES      = SAMPLE_RATE * 5 / 1000;   // 5 msec fade in/out = 80 samples
const int DAC_MIDPOINT      = 2048;                     // = exactly half of 2^12
const int DAC_AMPLITUDE     = 2040;                     // slightly less than half of 2^12
const int MAX_MESSAGE       = 64;                       // longest message for startSending()
const uint8_t PATTERN_SPACE = 0;                        // marker for a space between words

// Spacing, in dits. Not PARIS timing (which is 1/3/7): Griduino uses longer gaps.
const uint32_t ELEMENT_GAP = 1;    // between dits and dahs within a letter
const uint32_t LETTER_GAP  = 4;    // between letters
const uint32_t WORD_GAP    = 10;   // between words

int16_t sineTable[SINE_SIZE];   // one cycle of sine wave, +/- DAC_AMPLITUDE
bool timerReady = false;

// ----- state shared between startSending() and the ISR
volatile bool active      = false;   // true from startSending() until the final fade-out is done
volatile bool keyDown     = false;   // true = tone on, false = silent
volatile bool stopRequest = false;

uint8_t patterns[MAX_MESSAGE];          // message converted to Morse patterns, read lsb-to-msb, msb is sentinel
volatile int numPatterns     = 0;       // number of entries in patterns[]
volatile int nextIndex       = 0;       // next entry in patterns[] to send
volatile uint8_t pattern     = 1;       // elements remaining in current character (1 = none left)
volatile bool afterElement   = false;   // true = an element just finished, a gap comes next
volatile uint32_t remaining  = 0;       // samples left in the current dit, dah or space
volatile uint32_t ditSamples = 0;       // samples per dit at the current speed
volatile uint32_t phase      = 0;       // phase accumulator: 2^32 = one full cycle
volatile uint32_t phaseStep  = 0;       // phase increment per sample = freq * 2^32 / SAMPLE_RATE
volatile int envelope        = 0;       // 0..RAMP_SAMPLES, fades the tone in and out

inline void writeDAC(uint16_t value) {
  while (DAC->SYNCBUSY.bit.DATA0) {
  }   // wait for previous write (normally already done)
  DAC->DATA[0].reg = value;
}

void stopTimer() {
  TC4->COUNT16.CTRLA.bit.ENABLE = 0;
  while (TC4->COUNT16.SYNCBUSY.bit.ENABLE) {
  }
}

void startTimer() {
  TC4->COUNT16.COUNT.reg = 0;
  while (TC4->COUNT16.SYNCBUSY.bit.COUNT) {
  }
  TC4->COUNT16.CTRLA.bit.ENABLE = 1;
  while (TC4->COUNT16.SYNCBUSY.bit.ENABLE) {
  }
}

void initTimer() {
  // build sine table once
  for (int ii = 0; ii < SINE_SIZE; ii++) {
    sineTable[ii] = (int16_t)(DAC_AMPLITUDE * sin(2.0 * PI * ii / SINE_SIZE));
  }

  // let the Arduino core enable the DAC on our pin, at its idle midpoint
  analogWriteResolution(12);
  analogWrite(DAC0, DAC_MIDPOINT);

  // clock TC4 from GCLK1 (48 MHz)
  GCLK->PCHCTRL[TC4_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1 | GCLK_PCHCTRL_CHEN;
  while (!(GCLK->PCHCTRL[TC4_GCLK_ID].reg & GCLK_PCHCTRL_CHEN)) {
  }
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_TC4;

  // reset, then 16-bit counter that restarts at CC0 ("match frequency" mode)
  TC4->COUNT16.CTRLA.bit.SWRST = 1;
  while (TC4->COUNT16.SYNCBUSY.bit.SWRST) {
  }
  TC4->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1;
  TC4->COUNT16.WAVE.reg  = TC_WAVE_WAVEGEN_MFRQ;
  TC4->COUNT16.CC[0].reg = TIMER_CLOCK / SAMPLE_RATE - 1;   // 2999 => 16 kHz
  while (TC4->COUNT16.SYNCBUSY.bit.CC0) {
  }
  TC4->COUNT16.INTENSET.reg = TC_INTENSET_MC0;   // interrupt on each match

  NVIC_SetPriority(TC4_IRQn, 2);   // 0 is highest priority; leave 0..1 for more urgent interrupts
  NVIC_ClearPendingIRQ(TC4_IRQn);
  NVIC_EnableIRQ(TC4_IRQn);

  timerReady = true;
}

// Advance to the next dit, dah, or space. Called from ISR when "remaining" reaches zero.
inline void nextSegment() {
  if (afterElement) {
    afterElement = false;
    keyDown      = false;
    // gap within a letter, else gap between letters
    remaining = (pattern > 1) ? ELEMENT_GAP * ditSamples : LETTER_GAP * ditSamples;
    return;
  }
  if (pattern <= 1) {
    // current character is finished: load the next one
    if (nextIndex >= numPatterns) {
      keyDown     = false;   // message done; ISR finishes after the fade-out
      remaining   = 0;
      numPatterns = 0;
      return;
    }
    uint8_t p = patterns[nextIndex++];
    if (p == PATTERN_SPACE) {
      // letter gap already sent, add the rest to make a word gap
      keyDown   = false;
      remaining = (WORD_GAP - LETTER_GAP) * ditSamples;
      return;
    }
    pattern = p;
  }
  // send one element: 0=dit, 1=dah
  keyDown      = true;
  remaining    = (pattern & 1) ? 3 * ditSamples : ditSamples;
  pattern      = pattern >> 1;
  afterElement = true;
}

}   // namespace isr

// ========== interrupt service routine ========================
extern "C" void TC4_Handler(void) {
  using namespace isr;
  TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_MC0;   // acknowledge interrupt

  // ----- Morse timing
  if (stopRequest) {
    keyDown      = false;
    numPatterns  = 0;
    pattern      = 1;
    afterElement = false;
    remaining    = 0;
    stopRequest  = false;
  } else if (remaining > 0) {
    remaining--;
  } else if (numPatterns > 0 || pattern > 1 || afterElement) {
    nextSegment();
  }

  // ----- envelope: fade in while key is down, fade out while key is up
  if (keyDown) {
    if (envelope < RAMP_SAMPLES)
      envelope++;
  } else {
    if (envelope > 0)
      envelope--;
  }

  // ----- audio sample
  if (envelope > 0) {
    phase += phaseStep;
    int32_t sample = sineTable[phase >> 24];
    writeDAC(DAC_MIDPOINT + (sample * envelope) / RAMP_SAMPLES);
  } else {
    phase = 0;   // start each tone at zero crossing
    writeDAC(DAC_MIDPOINT);
    if (!keyDown && remaining == 0 && numPatterns == 0 && pattern <= 1 && !afterElement) {
      // finished: silence, and stop interrupting the main loop
      active                        = false;
      TC4->COUNT16.CTRLA.bit.ENABLE = 0;
    }
  }
}
// ========== end interrupt service routine ====================

void DACMorseSender::startSending() {
  using namespace isr;
  if (!timerReady) {
    initTimer();
  }

  // stop anything in progress (fades out over 5 msec, then the ISR stops the timer)
  DACMorseSender::stop();
  stopTimer();
  writeDAC(DAC_MIDPOINT);

  // convert message to Morse patterns here, so the ISR never searches tables or prints
  int count = 0;
  for (unsigned int ii = 0; ii < message.length() && count < MAX_MESSAGE; ii++) {
    char c            = message.charAt(ii);
    patterns[count++] = (c == ' ') ? PATTERN_SPACE : (uint8_t)getPattern(c);
  }
  if ((int)message.length() > MAX_MESSAGE) {
    Serial.print("!!! Morse message truncated to ");
    Serial.print(MAX_MESSAGE);
    Serial.println(" characters");
  }

  Serial.print("Start sending morse ");
  Serial.print(wpm, 1);
  Serial.print(" wpm, ");
  Serial.print(fFrequency, 0);
  Serial.print(" Hz: ");
  Serial.println(message);

  // load ISR state (timer is stopped, so no race)
  ditSamples   = (uint32_t)(SAMPLE_RATE * 1.2f / max(5.0f, wpm));   // dit = 1200/wpm msec
  phaseStep    = (uint32_t)(fFrequency * 4294967296.0 / SAMPLE_RATE);
  phase        = 0;
  envelope     = 0;
  keyDown      = false;
  stopRequest  = false;
  pattern      = 1;
  afterElement = false;
  remaining    = 0;
  nextIndex    = 0;
  numPatterns  = count;
  active       = (count > 0);

  if (active) {
    startTimer();
  }
}

bool DACMorseSender::isBusy() {
  return isr::active;
}

void DACMorseSender::stop() {
  if (!isr::active) {
    return;
  }
  isr::stopRequest = true;   // ISR fades out (5 msec) then stops the timer
  while (isr::active) {
  }
}
