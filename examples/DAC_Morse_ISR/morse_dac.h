#pragma once
/**
 * Send Morse Code with a sine wave using onboard DAC to a speaker.
 *
 * Two ways to send:
 *   sendBlocking()  - original method, does not return until message complete.
 *   startSending()  - non-blocking, returns immediately. A TC4 timer interrupt
 *                     generates the audio samples and the dit/dah timing while
 *                     the main loop keeps running. Poll isBusy() to know when done.
 *
 * SAMD51 (Feather M4) only. Uses timer TC4, which is not used by the
 * TFT backlight PWM (D4 = TC3) or by the Arduino core.
 *
 * All input should be lowercase.
 * Prosigns (SK, KN, etc) have special character values #defined.
 *
 * This was inspired by the fine example by Mark Fickett, KB3JCY, morse package.
 *        https://github.com/markfickett/arduinomorse
 */

// Punctuation and Prosigns
#define PROSIGN_SK 'S'
#define PROSIGN_KN 'K'
#define PROSIGN_BT 'B'
#define PROSIGN_AS 'A'

class DACMorseSender {
private:
  String message;

  // DAC settings
  int dacPin;            // identifies DAC output port
  int dacChannel;        // DAC channel for dacPin: 0 = DAC0 (A0), 1 = DAC1 (A1), -1 = not a DAC pin
  float fFrequency;      // Hz
  float dacSampleTime;   // microseconds to hold each waveform sample

  const int dacAmplitude = 2040;   // = slightly less than half of 2^12, to prevent overflow from rounding errors
  const int dacOffset    = 2048;   // = exactly half of 2^12
                                   // Requirement is: dacOffset +/- dacAmplitude = voltage range of DAC
  // Morse settings
  float wpm;   // words per minute

  float fDitDuration;   // seconds per dit
  int iDitDuration;     // msec    per dit
  float letterSpace;    // = ditDuration * 10 (seconds)
  float wordSpace;      // = ditDuration * 24 (seconds)

  int cyclesPerDit;   // number of waveforms to fill an entire DIT
  int cyclesPerDah;   // number of waveforms to fill an entire DAH

public:
  /**
   * Create a sender for the given DAC port, audio frequency, and sending rate.
   */
  DACMorseSender(int outputPin, int iFreq, float fWPM) {
    dacPin     = outputPin;
    dacChannel = (outputPin == DAC0) ? 0 : (outputPin == DAC1) ? 1
                                                               : -1;
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
};
