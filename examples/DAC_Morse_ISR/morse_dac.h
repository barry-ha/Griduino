#pragma once   // Please format this file with clang before check-in to GitHub
/*
  File:     morse_dac.h

  Purpose:  Send Morse Code with a sine wave using onboard DAC to a speaker.

  Two ways to send:
    sendBlocking()  - original method, does not return until message complete.
    startSending()  - non-blocking, returns immediately. A TC4 timer interrupt
                      generates the audio samples and the dit/dah timing while
                      the main loop keeps running. Poll isBusy() to know when done.
  
  Note:
    SAMD51 (Feather M4) only. Uses timer TC4, which is not used by 
    Griduino's TFT backlight PWM (which uses timer TC3 on pin D4).
  Spacing:
    Morse code spacing is selected by constants in morse_dac.cpp. 
    Although PARIS standard is 1/3/7, we prefer 1/4/10 for better copy in a noisy vehicle cabin.
    Look for constants ELEMENT_GAP, LETTER_GAP, and WORD_GAP in morse_dac.cpp.
  DAC Output: 
    This implementation is hard-coded to DAC0 (pin A0), the Griduino speaker output pin.
    Both sendBlocking() and the non-blocking ISR write to DAC0.
 
  All input should be lowercase.
  Prosigns (SK, KN, etc) have special character values #defined.
 
  This was inspired by the fine example by Mark Fickett, KB3JCY, morse package.
         https://github.com/markfickett/arduinomorse

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

*/

// Punctuation and Prosigns
#define PROSIGN_SK  'S'   // end of transmission
#define PROSIGN_KN  'K'   // slash
#define PROSIGN_BT  'B'   // dash
#define PROSIGN_AS  'A'   // wait
#define PROSIGN_IMI 'Q'   // question
#define PROSIGN_MIM 'C'   // comma
#define PROSIGN_AAA '.'   // period

// ========== class DACMorsender ==================================
class DACMorseSender {
private:
  String message;

  // DAC settings
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
   * Create a sender for the given audio frequency and sending rate.
   * Output is always DAC0 (pin A0).
   */
  DACMorseSender(int iFreq, float fWPM) {
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
   * Non-blocking: start sending the current message and return immediately.
   * Audio and Morse timing run at interrupt level, so the main loop keeps running.
   * If another message is already being sent, it is stopped and replaced.
   * Spacing: 1 dit between elements, 4 between letters, 10 between words (not PARIS 1/3/7).
   */
  void startSending();

  /**
   * True while a non-blocking message is still being sent.
   */
  static bool isBusy();

  /**
   * Stop a non-blocking message immediately (with a short fade to avoid a click).
   */
  static void stop();

  /**
   * Deprecated: use these for unit test
   */
  void dump();
  void send_dit();
  void send_dah();
  void send_dit_space();
  void send_letter_space();
  void send_word_space();

private:
  void send(char c);
};   // end class DACMorseSender
