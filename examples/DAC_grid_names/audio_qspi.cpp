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

#include <SdFat.h>               // for FAT file systems on Flash and Micro SD cards
#include <Adafruit_SPIFlash.h>   // for FAT file systems on SPI flash chips
#include <elapsedMillis.h>       // for short-interval timing functions
#include "audio_qspi.h"

// canonical WAV header
struct CanonicalWaveHeader {
  // https://www.lightlink.com/tjweber/StripWav/Canon.html
  // https://wavefilegem.com/how_wave_files_work.html
  //   Offset  Length   Contents
  char riff[4];             //    0       4 bytes  'RIFF'         // "RIFF" id string
  uint32_t chunk1Size;      //    4       4 bytes  <byte count>   // RIFF chunk size = <length of file - 8>
  char wave[4];             //    8       4 bytes  'WAVE'         // WAVE header
                            // - "fmt" chunk 1 -
  char format[4];           //    12      4 bytes  'fmt '         // "fmt " id string
  uint32_t fmtlen;          //    16      4 bytes  0x00000010     // fmt chunk size = always 16 bytes
  uint16_t fmttag;          //    20      2 bytes  0x0001         // Audio format: 1=PCM, 2=ADPCM, 3=floatPCM, 6=mulaw, 7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
  uint16_t channels;        //    22      2 bytes  <channels>     // Number of channels: 1=mono, 2=stereo
  uint32_t samplesPerSec;   //    24      4 bytes  <sample rate>  // Samples per second, Hz: e.g., 16000 or 44100
  uint32_t bytesPerSec;     //    28      4 bytes  <bytes/second> // sample rate * block align
  uint16_t blockAlign;      //    32      2 bytes  <block align>  // channels * bits/sample / 8: 2=16bit mono, 4=16bit stereo
  uint16_t bitsPerSample;   //    34      2 bytes  <bits/sample>  // 8 or 16
                            // - "data" chunk 2 -
  char chunk2id[4];         //    36      4 bytes  'data'         // "data" id string
  uint32_t chunk2size;      //    40      4 bytes  <byte count>   // data chunk size N = <length of data block>
                            //    44      N bytes  <sample data>

public:
  bool isValid(int filesize) {
    bool rc = true;   // assume success
    if (strncmp(this->riff, "RIFF", 4) != 0) {
      Serial.println(". DATA error, file does not begin with 'RIFF'");
      rc = false;
    }

    if (strncmp(this->wave, "WAVE", 4) != 0) {
      Serial.println(". WAVE data error, file does not contain 'WAVE'");
      rc = false;
    }

    if (this->fmttag != 1) {
      Serial.println(". WAV data error, does not contain samples in PCM");
      rc = false;
    }

    if (this->channels != 1) {
      Serial.println(". WAV data error, not a Mono format file");
      rc = false;
    }

    if (this->bitsPerSample != 16) {
      Serial.println(". WAV data error, not 16 bits per sample");
      rc = false;
    }

    if (strncmp(this->chunk2id, "data", 4) != 0) {
      Serial.println(". WAV data error, does not contain 'data' chunk");
      rc = false;
    }

    int expectedDataLength = this->chunk2size;
    if (this->chunk2size > filesize) {
      Serial.println(". Error, WAV header claims more data than total file size");
      rc = false;

      // override this bogus value from the WAV file header
      // to make sure we don't overrun our 32k memory buffer
      this->chunk2size = min(filesize - 44, MAXBUFFERSIZE);
    }

    if (true /*!rc*/) {
      dump();
    }
    return rc;
  }
  void dump() {   // diagnostic
    Serial.print(". Chunk 1 id: ");
    Serial.print(riff);
    Serial.print(" Size: ");
    Serial.println(chunk1Size);
    Serial.print("  wave: ");
    Serial.println(wave);
    Serial.print("  format: ");
    Serial.println(format);
    Serial.print("  fmtlen: ");
    Serial.println(fmtlen);
    Serial.print("  fmttag: ");
    Serial.println(fmttag);
    Serial.print("  channels: ");
    Serial.println(channels);
    Serial.print("  samplesPerSec: ");
    Serial.println(samplesPerSec);
    Serial.print("  bytesPerSec: ");
    Serial.println(bytesPerSec);
    Serial.print("  blockAlign: ");
    Serial.println(blockAlign);
    Serial.print("  bitsPerSample: ");
    Serial.println(bitsPerSample);
    Serial.print(". Chunk 2 id: ");
    Serial.print(chunk2id);
    Serial.print(" Size: ");
    Serial.println(chunk2size);
  }
};

// ---------- Flash chip
// SD card shares the hardware SPI interface with TFT display, and has
// separate 'select' pins to identify the active device on the bus.
const int chipSelectPin = 7;
const int chipDetectPin = 8;

// ------------ Audio output
#define DAC_PIN     DAC0   // onboard DAC0 == pin A0
#define PIN_SPEAKER DAC0   // uses DAC

Adafruit_FlashTransport_QSPI gFlashTransport;   // Quad-SPI 2MB memory chip
Adafruit_SPIFlash gFlash(&gFlashTransport);     //
FatFileSystem gFatfs;                           // file system object from SdFat

// ========== file system helpers ==============================
#define MAXBUFFERSIZE 32000   // max = 32K @ 16 khz = max 2.0 seconds
int AudioQSPI::openFlash(void) {
  // returns 1=success, 0=failure

  // Initialize flash library and check its chip ID.
  if (!gFlash.begin()) {
    Serial.println("Error, failed to initialize onboard memory.");
    return 0;
  }
  Serial.print(". Flash chip JEDEC ID: 0x");
  Serial.println(gFlash.getJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!gFatfs.begin(&gFlash)) {
    Serial.println("Error, failed to mount filesystem");
    Serial.println("Was the flash chip formatted with the SdFat_format example?");
    return 0;
  }
  Serial.println(". Mounted SPI flash filesystem");
  return 1;
}

bool AudioQSPI::begin(void) {
  Serial.print("Detecting Flash memory using pin ");
  Serial.println(chipDetectPin);
  pinMode(chipDetectPin, INPUT_PULLUP);               // use internal pullup resistor
  bool isCardDetected = digitalRead(chipDetectPin);   // HIGH = no card; LOW = card detected
  if (isCardDetected) {
    Serial.println(". Success - found a memory chip");
  } else {
    Serial.println(". Failed - no memory chip found");
  }

  // ----- init FLASH memory chip on the Feather M4 board
  Serial.println("Initializing interface to Flash memory...");
  int result = openFlash();   // open file system

// ----- init onboard DAC
#if defined(SAMD_SERIES)
      // Only set DAC resolution on devices that have a DAC
  analogWriteResolution(12);   // 1..32, sets DAC output resolution to 12 bit (4096 levels)
                               // because Feather M4 maximum output resolution is 12 bit
#endif
  delay(100);   // settling time

  return isCardDetected && result;
}

// ----- helpers to output PCM sampled audio to DAC
int AudioQSPI::audioFloatToInt(float item) {   // scale floating point sample to DAC integer
  // input:  (-1 ... +1)
  // output: (0 ... 2^12)
  return int((item + 1.0) * 2040.0);
}
void AudioQSPI::playAudioFloat(const float *audio, unsigned int numSamples, int holdTime) {
  for (int ii = 0; ii < numSamples; ii++) {
    int value = audioFloatToInt(audio[ii]);
    analogWrite(DAC0, value);
    delayMicroseconds(holdTime);   // hold the sample value for the sample time
  }
  // reduce speaker click by setting resting output sample to midpoint
  int midpoint = audioFloatToInt((audio[0] + audio[numSamples - 1]) / 2.0);
  analogWrite(DAC0, midpoint);
}
unsigned int AudioQSPI::scale16BitToDAC(int sample) {
  // input:  (-32767 ... +32767)
  // output: (0 ... 2^12)
  return map(sample, -32767, +32767, 0, 4096);
}
void AudioQSPI::playAudio16Bit(const int16_t audio[MAXBUFFERSIZE], const int numSamples, const int holdTime) {
  for (int ii = 0; ii < numSamples; ii++) {
    int value = scale16BitToDAC(audio[ii]);
    analogWrite(DAC0, value);
    delayMicroseconds(holdTime);   // hold the sample value for the sample time
  }
}
void AudioQSPI::playAudio8bit(const unsigned char *audio, int numSamples, int holdTime) {
  for (int ii = 0; ii < numSamples; ii++) {
    int value = audio[ii] << 4;   // max sample is 2^8, max DAC output 2^12, so shift left by 4
    analogWrite(DAC0, value);
    delayMicroseconds(holdTime);   // hold the sample value for the sample time
  }
}

// ===== file handling helpers open/read/close
File AudioQSPI::open(const char *audioFileName) {
  Serial.print("Opening file: ");
  Serial.println(audioFileName);

  if (gFatfs.exists(audioFileName)) {
    // success
  } else {
    Serial.println(". EXISTS error, file not found");
  }

  File fileHandle = gFatfs.open(audioFileName);
  return fileHandle;
}

// ----- read WAV meta data: a simple interface to query WAV file attributes
bool AudioQSPI::getInfo(WaveInfo *pInfo, const char *audioFileName) {
  // input:  filename, limited to 32 bytes
  // output: WaveInfo filled in with WAV metadata
  //         uses Serial.print() for error messages
  //         all files and file handles are closed
  //         returns true=success, false=failure

  // init default values
  bool rc = true;   // assume success
  strncpy(pInfo->filename, audioFileName, sizeof(pInfo->filename));
  pInfo->samplesPerSec  = 16000;                        // default to 16 kHz
  pInfo->filesize       = 0;                            // init to 'no data'
  pInfo->bytesPerSample = 2;                            // default to 2 bytes/sample   todo: read from WAV file
  pInfo->numSamples     = 0;                            // init to 'no samples'
  pInfo->holdtime       = 1E6 / pInfo->samplesPerSec;   // typ: 62 usec

  // open WAV file from 2MB flash memory and check for errors
  File myFile = this->open(audioFileName);
  if (myFile) {
    // read meta data
    pInfo->filesize = myFile.size();
    Serial.print(". File size: ");
    Serial.println(pInfo->filesize);

    char header[44];
    int bytesread = myFile.read(header, sizeof(header));
    if (bytesread == sizeof(header)) {
      // process meta data
      CanonicalWaveHeader *pHdr = (CanonicalWaveHeader *)&header;   // superimpose the canonical WAV header class onto the file data

      pInfo->samplesPerSec  = pHdr->samplesPerSec;
      pInfo->bytesPerSample = pHdr->bitsPerSample / 8;
      pInfo->numSamples     = pHdr->chunk2size / pHdr->blockAlign;
      pInfo->holdtime       = 1E6 / pInfo->samplesPerSec;

      rc = pHdr->isValid(pInfo->filesize);   // examine header, report unsupported features

    } else {
      Serial.print(". READ error, expected 44 bytes but only got ");
      Serial.println(bytesread);
      rc = 0;
    }

  } else {
    rc = 0;   // indicate failure
  }

  // close
  myFile.close();
  return rc;
}

// ----- read WAV header and WAV meta data
bool AudioQSPI::getWaveData(WaveInfo *pInfo, int16_t pBuffer[MAXBUFFERSIZE], const char *audioFileName) {
  // input:  filename
  // output: meta data filled in about the WAV file
  //         buffer filled in with data
  //         all files and file handles are closed
  //         returns true=success, false=failure
  bool rc = getInfo(pInfo, audioFileName);   // read WAV header, check for unsupported features

  // open wave file from 2MB flash memory
  File myFile = this->open(audioFileName);
  if (myFile) {
    // skip over canonical WAV header (error checking is already done in 'getInfo' above)
    char header[44];
    int bytesread = myFile.read(header, sizeof(header));

    // ----- read the remainder of the WAV file, ie, all the sampled audio data
    bytesread = myFile.read(pBuffer, pInfo->numSamples * pInfo->bytesPerSample);
    if (bytesread > 0) {
      // success reading WAV data
      Serial.print(". Successfully read ");
      Serial.print(bytesread);
      Serial.println(" bytes PCM");
    } else {
      Serial.println(". WAV data error, did not read any bytes from file");
      rc = false;
    }

  } else {
    Serial.println(". OPEN error opening file");
    rc = false;
  }
  myFile.close();
  return rc;
}

bool AudioQSPI::play(const char *myfile) {
  // blocking: does not return until playback is finished
  bool ok = true;
  WaveInfo waveMeta;
  int16_t waveData[MAXBUFFERSIZE];

  if (getWaveData(&waveMeta, waveData, myfile)) {
    playAudio16Bit(waveData, waveMeta.numSamples, waveMeta.holdtime);
  } else {
    ok = false;
  }
  return ok;
}
