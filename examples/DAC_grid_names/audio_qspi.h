// Please format this file with clang before check-in to GitHub
/*
  File:     audio_qspi.h 

  Purpose:  Open and play Microsoft WAV files from Quad-SPI memory chip on Feather M4 Express

            This project documents everything needed for the whole process from 
            adjusting audio in Audacity, creating the WAV file, transferring your 
            WAV file to the QSPI chip on a Feather M4 board, opening the file in FatSd, 
            and playing the sampled audio on the DAC output. 

            The SAMD51 on the Feather M4 Express has large program space, but 
            it's not nearly enough to hold more than a few total seconds of 
            precompiled audio. Hence, I developed this interface to play audio 
            clips from the 2MB Quad-SPI memory chip.

            "Express" boards from Adafruit indicate it carries a memory chip alongside 
            the processor. It is accessed via SPI interface; it is not part of the instruction set 
            memory space. 

  Preparing audio files:
            Prepare your WAV file to 16 kHz mono:
            1. Open Audacity
            2. Open a project, e.g. \Documents\Arduino\Griduino\work_in_progress\Spoken Word Originals\Barry
            3. Select "Project rate" of 16000 Hz
            4. Select an audio fragment, such as "Charlie"
            5. Menu bar > Effect > Normalize 
               a. Remove DC offset
               b. Normalize peaks -1.0 dB
            5. Menu bar > File > Export > Export as WAV
               a. Save as type: WAV (Microsoft)
               b. Encoding: Signed 16-bit PCM
               c. Filename = e.g. "c_bwh_16.wav"
            6. The output file contains 2-byte integer numbers in the range -32767 to +32767

  Mono Audio: The DAC on the SAMD51 is a 12-bit output, from 0 - 3.3v.
            The largest 12-bit number is 4,096:
            * Writing 0 will set the DAC to minimum (0.0 v) output.
            * Writing 4096 sets the DAC to maximum (3.3 v) output.

  Relevant background:
            http://www.lightlink.com/tjweber/StripWav/WAVE.txt
            http://www.lightlink.com/tjweber/StripWav/Canon.html
            https://www.instructables.com/Playing-Wave-file-using-arduino/
            https://www.arduino.cc/en/Tutorial/SimpleAudioPlayer
            https://learn.adafruit.com/introducing-itsy-bitsy-m0/using-spi-flash
            https://www.arduino.cc/en/reference/SD
            https://www.arduino.cc/en/Reference/FileRead
            https://forum.arduino.cc/index.php?topic=695228.0
            https://github.com/PaulStoffregen/Audio
*/
#pragma once
#include <SdFat.h>   // for FAT file systems on Flash and Micro SD cards

struct WaveInfo {
  // Everything a caller wants to know about a single "Microsoft WAV" sampled sound file.
  // This is an easy way to know what's playing without opening files and parsing
  // the canonical WAV header yourself. It brings file- and error- handling into one spot.
  char filename[32];    // filename,                           e.g. "/male/c_bwh_16.wav"
  int filesize;         // total number of bytes in this WAV file
  int samplesPerSec;    // bitrate;                            e.g. '16000'
  int bytesPerSample;   // number of bytes in each sample      e.g. '2' for 16-bit int
  int numSamples;       // number of samples in this wave file
  int holdtime;         // microseconds to hold each sample    e.g. 1e6/bitrate = 62 usec
};

class AudioQSPI {
public:
  AudioQSPI(void) {}
  bool begin(void);
  bool play(const char *filename);   // blocking: does not return until playback is finished
  bool getInfo(WaveInfo *pInfo, const char *audioFileName);
#define MAXBUFFERSIZE 32000   // max = 32K @ 16 khz = max 2.0 seconds

protected:
  // ----- file handling helpers open/read/close
  int openFlash(void);
  bool getWaveData(WaveInfo *pInfo, int16_t pBuffer[MAXBUFFERSIZE], const char audioFileName[32]);
  File open(const char *audioFileName);

  // ----- audio helpers to output PCM sampled audio to DAC
  int audioFloatToInt(float item);
  void playAudioFloat(const float *audio, unsigned int audiosize, int holdTime);
  unsigned int scale16BitToDAC(int sample);
  void playAudio16Bit(const int16_t audio[MAXBUFFERSIZE], const int audiosize, const int holdTime);
  void playAudio8bit(const unsigned char *audio, int audiosize, int holdTime);
};
