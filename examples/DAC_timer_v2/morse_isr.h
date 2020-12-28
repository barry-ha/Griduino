#pragma once

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
#define PROSIGN_SK  'S'               // end of transmission
#define PROSIGN_KN  'K'               // slash
#define PROSIGN_BT  'B'               // dash
#define PROSIGN_AS  'A'               // wait
#define PROSIGN_IMI 'Q'               // '?' question
#define PROSIGN_MIM 'C'               // comma
#define PROSIGN_AAA '.'               // '.' period

class DACMorseSenderISR {
  private:
    String message;

    // DAC settings
    int dacPin;                       // identifies DAC output port
    int isrFreq;                      // interrupt service request rate, Hz
    float dacSampleTime;              // time to hold each waveform sample, microseconds
    unsigned int nSamples;            // number of entries in waveform lookup table
    unsigned int* pSamples;           // ptr to waveform lookup table

    float toneFreq = 800;             // audio frequency, Hz (default 800 Hz)
    const int dacAmplitude = 2040;    // = slightly less than half of 2^12, to prevent overflow from rounding errors
    const int dacOffset = 2048;       // = exactly half of 2^12
                                      // Requirement is: dacOffset +/- dacAmplitude = voltage range of DAC
    // Morse settings
    float wpm;                        // words per minute

    float fDitDuration;               // seconds per dit
    int   iDitDuration;               // msec    per dit
    float letterSpace;                // typ 3x ditDuration (seconds)
    float wordSpace;                  // typ 7x ditDuration (seconds)

    int cyclesPerDit;                 // number of waveforms to fill an entire DIT
    int cyclesPerDah;                 // number of waveforms to fill an entire DAH

  public:
	/**
	 * Create a sender for the given DAC port, audio frequency, and sending rate.
   * isrFreq = 8000 Hz to play WAV files
	 */
    DACMorseSenderISR(int outputPin, unsigned int isrFrequency) {
      dacPin = outputPin;
      isrFreq = isrFrequency;
      dacSampleTime = 1E6 / isrFreq;  // usec between interrupts
      nSamples = isrFreq / toneFreq;  // number of waveform table entries needed
      pSamples = (unsigned int*) malloc(nSamples * sizeof(unsigned int));
    }

    ~DACMorseSenderISR() {
      free(pSamples);
    }
  
    /**
     * To be called during the Arduino setup().
     * Create the waveform lookup table.
     * toneFrequency = min(100 Hz), max(1200 Hz), if you exceed this the output is undefined 
     */
    void setup(int toneFrequency = 800);

    /**
     * Set the words per minute (based on PARIS timing).
     */
    void setWPM(float wpm);

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
    void start_tone();                  // asynchronous ISR, non-blocking
    void stop_tone();                   // asynchronous ISR, non-blocking
    void send_dit();
    void send_dah();
    void send_dit_space();
    void send_letter_space();
    void send_word_space();

  private:
    void send(char c);
};
