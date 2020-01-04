#ifndef _GRIDUINO_MORSE_DAC_H
#define _GRIDUINO_MORSE_DAC_H

/**
 * Send Morse Code with a sine wave using onboard DAC to a speaker.
 * This version sends in a blocking manner (does not return until message complete).
 *
 * All input should be lowercase.
 * Prosigns (SK, KN, etc) have special character values #defined.
 *
 * This was inspired by the fine example by Mark Fickett, KB3JCY, morse package.
 *        https://github.com/markfickett/arduinomorse
 */

// Punctuation and Prosigns
#define PROSIGN_SK  'S'   // end of transmission
#define PROSIGN_KN  'K'   // slash
#define PROSIGN_BT  'B'   // dash
#define PROSIGN_AS  'A'   // wait
#define PROSIGN_IMI 'Q'   // question
#define PROSIGN_MIM 'C'   // comma
#define PROSIGN_AAA '.'   // period

class DACMorseSender {
  private:
    String message;

    // DAC settings
    int dacPin;           // identifies DAC output port
    float fFrequency;     // Hz
    float dacSampleTime;  // microseconds to hold each waveform sample

    const int dacAmplitude = 2040;  // = slightly less than half of 2^12, to prevent overflow from rounding errors
    const int dacOffset = 2048;     // = exactly half of 2^12
                                    // Requirement is: dacOffset +/- dacAmplitude = voltage range of DAC
    // Morse settings
    float wpm;            // words per minute

    float fDitDuration;   // seconds per dit
    int   iDitDuration;   // msec    per dit
    float letterSpace;    // typ 3x ditDuration (seconds)
    float wordSpace;      // typ 7x ditDuration (seconds)

    int cyclesPerDit;     // number of waveforms to fill an entire DIT
    int cyclesPerDah;     // number of waveforms to fill an entire DAH

  public:
	/**
	 * Create a sender for the given DAC port, audio frequency, and sending rate.
	 */
    DACMorseSender(int outputPin, int iFreq, float fWPM) {
      dacPin = outputPin;
      fFrequency = iFreq;
      wpm = fWPM;
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
    void dump();
    void send_dit();
    void send_dah();
    void send_dit_space();
    void send_letter_space();
    void send_word_space();

  private:
    void send(char c);
};

#endif // _GRIDUINO_MORSE_DAC_H
