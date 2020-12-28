/*
  DAC Timer v2 -- basic audio tone using interrupt level to control DAC

  Change log:
            2020-12-24 created

  Software: Barry Hansen, K7BWH, barry@k7bwh.com, Seattle, WA
  Hardware: John Vanderbeck, KM7O, Seattle, WA

  Purpose:  Generate an audio sine wave in the speaker from an ISR.
            Provide an interface to change the tone's frequency.
            This is another small step toward playing WAV files with 8 kHz encoding.

  Guess'n check table size:
            * 800 Hz = each complete cycle takes 1/800 = 1,250 usec
            * 10 steps/wave = 1250/10 = 125 usec per interrupt = 8 kHz

            Note the waveform lookup table size varies for the desired audio frequency.
            * For 1600 Hz: table size = (8000 Hz)/(1600 Hz) =  5.00 entries
            * For 1200 Hz: table size = (8000 Hz)/(1200 Hz) =  6.67 entries
            * For 1100 Hz: table size = (8000 Hz)/(1100 Hz) =  7.27 entries
            * For 1000 Hz: table size = (8000 Hz)/(1000 Hz) =  8.00 entries
            * For  800 Hz: table size = (8000 Hz)/( 800 Hz) = 10.00 entries 
            * For  700 Hz: table size = (8000 Hz)/( 700 Hz) = 11.43 entries
            * For  400 Hz: table size = (8000 Hz)/( 400 Hz) = 20.00 entries

  Note:     Does not control the backlight.
            Does not initialize or use the TFT display at all.

  Setup:
            Unplug the display screen and set it aside. Attach an oscilloscope to test points.
            Open serial console to read status messages.

  Conclusion: 
            For a fixed interrupt rate of 8 kHz, the small number of audio 
            frequencies available (12) is unacceptable for Morse code sidetone. 
            Do not use a fixed 8 kHz interrupt for self-generated waveforms.

            For a fixed number of samples/waveform (variable interrupt rate), 
            the best audio quality should have 10 steps per sine wave.
            Programs seem to run fine at higher ISR frequencies, tested up 
            to 1,600 Hz audio = 16 kHz interrupt rate.

            Going forward from here, based on results heard, I plan to choose 
            a single non-adjustable Griduino sidetone frequency of 800 Hz
            and a lookup table of 10 steps/waveform at an ISR rate of 8 kHz.
            This is also convenient for playing WAV samples at 8 kHz.

  Inspired by:
            Mark Fickett, KB3JCY, morse package.
            https://github.com/markfickett/arduinomorse

  Requires: 
            Dennis-van-Gils/SAMD51_InterruptTimer
            https://github.com/Dennis-van-Gils/SAMD51_InterruptTimer 
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>         // TFT color display library
#include <DS1804.h>                   // DS1804 digital potentiometer library
#include "morse_isr.h"                // Morse Code using digital-audio converter DAC0

// ------- Identity for splash screen and console --------
#define PROGRAM_TITLE   "DAC Timer v2"
#define PROGRAM_VERSION "v0.30"
#define PROGRAM_LINE1   "Barry K7BWH"
#define PROGRAM_LINE2   "John KM7O"
#define PROGRAM_COMPILED __DATE__ " " __TIME__

// ------------ Audio output
#define DAC_PIN      DAC0             // onboard DAC0 == pin A0
#define PIN_SPEAKER  DAC0             // uses DAC

// Adafruit Feather M4 Express pin definitions
#define PIN_VCS      A1               // volume chip select
#define PIN_VINC      6               // volume increment
#define PIN_VUD      A2               // volume up/down

// ctor         DS1804( ChipSel pin, Incr pin,  U/D pin,  maxResistance (K) )
DS1804 volume = DS1804( PIN_VCS,     PIN_VINC,  PIN_VUD,  DS1804_TEN );
int gWiper = 15;                      // initial digital potentiometer wiper position, 0..99

// ------------ definitions
const int howLongToWait = 4;          // max number of seconds at startup waiting for Serial port to console

// ----- console Serial port helper
void waitForSerial(int howLong) {
  // Adafruit Feather M4 Express takes awhile to restore its USB connx to the PC
  // and the operator takes awhile to restart the console (Tools > Serial Monitor)
  // so give them a few seconds for this to settle before sending messages to IDE
  unsigned long targetTime = millis() + howLong*1000;
  while (millis() < targetTime) {
    if (Serial) break;
  }
}

//=========== setup ============================================
void setup() {

  // ----- init TFT backlight
  //pinMode(TFT_BL, OUTPUT);          // <------ THIS CAUSES PROGRAM CRASH 
  //analogWrite(TFT_BL, 255);         // <------ THIS CAUSES PROGRAM CRASH 
                                      // Symptom: screen is dark, LED stops flashing, USB port non-responsive
                                      // To recover, double-click Feather's "Reset" button and load another pgm

  // ----- init serial monitor
  Serial.begin(115200);               // init for debuggging in the Arduino IDE
  waitForSerial(howLongToWait);       // wait for developer to connect debugging console

  // now that Serial is ready and connected (or we gave up)...
  Serial.println(PROGRAM_TITLE " " PROGRAM_VERSION);  // Report our program name to console
  Serial.println("Compiled " PROGRAM_COMPILED);       // Report our compiled date
  Serial.println(__FILE__);                           // Report our source code file name
  Serial.println("CPU Frequency = " + String(F_CPU / 1000000) + " MHz");

  // ----- init digital potentiometer
  volume.unlock();                    // unlock digipot (in case someone else, like an example pgm, has locked it)
  volume.setToZero();                 // set digipot hardware to match its ctor (wiper=0) because the chip cannot be read
                                      // and all "setWiper" commands are really incr/decr pulses. This gets it sync.
  volume.setWiperPosition(20);        // set default volume in digital pot, '20' is reasonable

  // ----- init DAC for audio/morse code
  #if defined(SAMD_SERIES)
    // Only set DAC resolution on devices that have a DAC
    analogWriteResolution(12);        // 1..32, sets DAC output resolution to 12 bit (4096 levels)
                                      // because Feather M4 maximum output resolution is 12 bit
  #endif

  // ---------- Morse Code ----------
  int gFrequency = 800;               // initial Morse code sidetone pitch
  int gWordsPerMinute = 15;           // initial Morse code sending speed

  // ---------- Interrupt handler ---
  int nPause = 2000;                  // pause to listen to each tone, msec

  // --------------------------------------------------------------------------------- //
  //           Test range of tones at constant 8 KHz rate                              //
  // --------------------------------------------------------------------------------- //
  // Conclusions:
  //      The small choice of audio frequencies available is unacceptable
  //      for Morse code sidetone. Do not use a fixed 8 kHz interrupt for
  //      self-generated waveforms.

  Serial.println("\n***** Testing Fixed 8 kHz Interrupt Rate (variable number samples/waveform *****\n");

  for (int kk=16; kk>=3; kk--)
  {
    const unsigned int isrFrequency = 8000;  // 8 kHz interrupt rate to play WAV files
    DACMorseSenderISR dacTone(DAC_PIN, isrFrequency);

    unsigned int toneFreq = 8000 / kk;

    Serial.print("----- "); Serial.print(kk); Serial.print("samples/cycle, "); Serial.print(toneFreq); Serial.println(" Hz -----");
    dacTone.setup(toneFreq);
    //dacTone.dump();
    dacTone.start_tone();
    delay(nPause);
    dacTone.stop_tone();
    delay(25);
  }

  // --------------------------------------------------------------------------------- //
  //           Test range of tones at constant N samples/waveform                      //
  // --------------------------------------------------------------------------------- //
  // Conclusions:
  //       4 samples/wave is too crude and sounds very raspy
  //       6 samples/wave is crude, sounds raspy, has an overtone below 650 Hz
  //       8 samples/wave is better, has an overtone below 480 Hz
  //      10 samples/wave sounds fairly pure, has no overtone
  //      This speaker does not respond <400 Hz no matter how smooth the waveform.

  Serial.println("\n***** Testing Fixed Number of Samples/Waveform (variable interrupt rate) *****\n");

  for (int ii=0; ii<3; ii++) {
    Serial.print("======================"); Serial.println(ii);
    nPause = 750;
    for (unsigned int audioFreq=400; audioFreq<=800; audioFreq=audioFreq*51/50) {
      int interruptFreq = 4 * audioFreq;
      Serial.print("----- Tone "); Serial.print(audioFreq); Serial.println(" Hz -----");
      DACMorseSenderISR dacTone(DAC_PIN, interruptFreq);
      dacTone.setup(audioFreq);
      dacTone.start_tone();
      delay(nPause);
      dacTone.stop_tone();
    }
    delay(2000);                  // a short break between series
    
  }
}

//=========== main work loop ===================================
void loop() {

}
