#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     morse_dac.h

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Send Morse Code with a sine wave using onboard DAC to a speaker.
            This version sends in a blocking manner (does not return until message complete).
            All input should be lowercase.
            Prosigns (SK, KN, etc) have special character values #defined.

            This was inspired by the fine example by Mark Fickett, KB3JCY, morse package.
            https://github.com/markfickett/arduinomorse
*/

// Punctuation and Prosigns
#define PROSIGN_SK  'S'   // end of transmission
#define PROSIGN_KN  'K'   // slash
#define PROSIGN_BT  'B'   // dash
#define PROSIGN_AS  'A'   // wait
#define PROSIGN_IMI 'Q'   // question
#define PROSIGN_MIM 'C'   // comma
#define PROSIGN_AAA '.'   // period

#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040)
// todo - for now, RP2040 has no DAC, no audio, no speech
// ========== class DACMorsender ==================================
class DACMorseSender {
public:
  DACMorseSender(int outputPin, int iFreq, float fWPM) {}
  void setup() {
    pinMode(A0, INPUT);   // Griduino v6 uses DAC0 (A0) to measure 3v coin battery; don't load down the pin
  }
  void setMessage(const String newMessage) {}
  void sendBlocking() {}
  void unit_test() {}
  void dump() {}
  void send_dit() {}
  void send_dah() {}
  void send_dit_space() {}
  void send_letter_space() {}
  void send_word_space() {}
};
#else

// ========== class DACMorsender ==================================
class DACMorseSender {
private:
  String message;

  // DAC settings
  int dacPin;            // identifies DAC output port
  float fFrequency;      // Hz
  float dacSampleTime;   // microseconds to hold each waveform sample

  const int dacAmplitude = 2040;   // = slightly less than half of 2^12, to prevent overflow from rounding errors
  const int dacOffset    = 2048;   // = exactly half of 2^12
                                   // Requirement is: dacOffset +/- dacAmplitude = voltage range of DAC
  // Morse settings
  float wpm;            // words per minute
                        //
  float fDitDuration;   // seconds per dit
  int iDitDuration;     // msec    per dit
  float letterSpace;    // typically 3x ditDuration (seconds)
  float wordSpace;      // typically 7x ditDuration (seconds)
                        //
  int cyclesPerDit;     // number of waveforms to fill an entire DIT
  int cyclesPerDah;     // number of waveforms to fill an entire DAH

public:
  /**
   * Create a sender for the given DAC port, audio frequency, and sending rate.
   */
  DACMorseSender(int outputPin, int iFreq, float fWPM) {
    dacPin     = outputPin;
    fFrequency = iFreq;
    wpm        = fWPM;
  }

  /**
   * To be called during the Arduino setup().
   * Create the waveform lookup table.
   */
  void setup();

  /**
   * Set the message to be sent.
   */
  void setMessage(const String newMessage);

  /**
   * Send the entirety of the current message before returning.
   */
  void sendBlocking();

  /**
   * Deprecated: use these for unit test
   */
  void unit_test();
  void dump();
  void send_dit();
  void send_dah();
  void send_dit_space();
  void send_letter_space();
  void send_word_space();

private:
  void send(char c);
};   // end class DACMorseSender
#endif   // ARDUINO_ADAFRUIT_FEATHER_RP2040