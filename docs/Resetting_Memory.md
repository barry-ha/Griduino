<h1>Griduino Memory Reset Instructions</h1>

<h2 id="intro">1. Introduction</h2>

<img src="../img/griduino-logo-120.png" align="right" alt="Griduino logo" title="Griduino logo"/>In rare instances, the Griduino flash memory may become corrupted. Here's how to recover by reformatting its internal file system and then re-installing Griduino software.

<h2 id="save">2. Backup Your Current Griduino Software</h2>
These steps will copy the software from Griduino to your hard drive. This is easier than downloading new software. Later, you'll restore this backup onto Griduino which guarantees you'll be running the exact same version as before.

1. **Plug in Griduino**<br/>
   - Use a standard USB data cable to connect your Griduino hardware to your computer.
   - Depending on your computer, you should see a message about "setting up device" and possibly "device Feather M4 is ready".
   - Remove Griduino's top cover to expose the "reset" button adjacent to the USB connector.

1. **Open Feather as a Drive**<br/>
   - Press the Feather's "reset" button **twice** rapidly, about a tenth of a second apart.
   - The Griduino screen may go blank out and turn white. Many computers will sound an audible "drive ready" chime.
   - In Windows, open the **File Explorer**.
   - Find the new drive, e.g. "**FEATHERBOOT (F:)**"
   - If the USB device name is CIRCUITPY, then double-click the Reset button again until you get FEATHERBOOT.

1. **Copy "CURRENT.UF2" from FEATHERBOOT to your computer**
   - Note: The file "current.uf2" is always the current software program loaded into the processor memory system.
   - Drag and drop "current.uf2" to your computer's local storage. 
   - Store the .UF2 file in a temporary place that you can find again later when you restore this software.

<h2 id="program">2. Reformat File System</h2>
Normally, you'll never need to reformat Arduino's file system. In the rare case you need to start over with a clean file system, here's how. 

1. **Download "flash_erase_express.uf2" file from GitHub**<br/>
   - Visit https://github.com/barry-ha/Griduino/tree/master/downloads
   - Download a copy of **flash_erase_express.uf2**
   - Note: We pre-compiled this program for you. It is a direct and unmodified program from the `examples` folder of https://github.com/adafruit/Adafruit_SPIFlash

1. **Install flash_erase_express**<br/>
   - Press the Feather's "reset" button **twice** rapidly, about a tenth of a second apart.
   - Drag the .UF2 file that you downloaded and drop it on the new FEATHERBOOT drive. Or you can copy/paste it to the new drive.
   - After the copying completes, the on-board Neopixel will be blue.
   - A few seconds later, the Neopixel should starting flashing green once per second. This indicates the SPI flash has been erased and all is well.
   - If the Nexopixel starts flashing red two or three times a second, an error has occurred and hardware repair is needed.
 
<h2 id="circuitpy">3. Install CircuitPython</h2>
We temporarily install CircuitPython, which prepares the flash memory file system to become compatible with Windows file systems.

1. **Download "CircuitPython" binary distribution file**<br/>
   - Get the latest CircuitPython UF2 file for your board (Feather M4 Express) from https://circuitpython.org/downloads.
   - As of April 2025, the latest stable release is 9.2.7 and the UF2 file is named "[adafruit-circuitpython-feather_m4_express-en_US-8.2.9.uf2](https://downloads.circuitpython.org/bin/feather_m4_express/en_US/adafruit-circuitpython-feather_m4_express-en_US-9.2.7.uf2)" and will look something like:<br/>![](img/circuit_python_feather_m4.jpg)

1. **Start the bootloader on the Feather board** by double-clicking its Reset button.<br/>
   - After a moment, you should see a "FEATHERBOOT" drive appear on your desktop computer.<br/>![](img/featherboot-img0702.jpg)

1. **Drag the circuitpython UF2 file** from Windows to FEATHERBOOT.
   - ![](img/drag-circuitpy-onto-featherboot-img0703.jpg)
   - The UF2 file will be copied and the Feather will reboot.
   - Then you should see a CIRCUITPY drive appear as an external USB drive. It will already have a few files on it.<br/>![](img/circuitpy-img0704.jpg)

<h2 id="audio">4. Optional: Install Audio Files</h2>

Optional: Griduino can use speech to announce grid lines. To do so, you can install audio recordings of the letters and numbers. These are .WAV files provided on GitHub in the `audio` folder.

You only need to install audio files one time.

The idea here is to use the CircuitPython installation (above) which allows the Flash memory to appear as an external USB drive. Then we can copy the "audio" folder to Flash. If, instead, the Griduino software is running then we cannot access its file system from the computer. The first time CircuitPython is installed, it automatically formats Flash memory in a way that remains compatible thereafter.

1. **Download "griduino_audio.zip" file from GitHub**<br/>
   - Visit https://github.com/barry-ha/Griduino/tree/master/downloads
   - Get a copy of **audio_female.zip** and **audio_male.zip**
   - Choose the voice you want; Griduino can only work with one voice at a time. To switch to another voice, repeat this process with the other files.
   - If you want to make your own recordings that are compatible with Griduino, see the README document at https://github.com/barry-ha/Audio_QSPI.

1. **Unzip e.g. "audio_male.zip"**
   - Extract this file to a temporary folder. 
   - The result will have a folder named "audio" that contains 26 letters and 10 numbers as short recordings in standard Microsoft WAV format.<br/>![Extracted audio folder](img/audio-folder-unzipped-img0701.jpg)

1. **Start the bootloader on the Feather board** by double-clicking its Reset button.<br/>
   - After a moment, you should see a "FEATHERBOOT" drive appear on your desktop computer.<br/>![](img/featherboot-img0702.jpg)

1. **Drag the circuitpython UF2 file** from Windows to FEATHERBOOT.
   - ![](img/drag-circuitpy-onto-featherboot-img0703.jpg)
   - The UF2 file will be copied and the Feather will reboot.
   - Then you should see a CIRCUITPY drive appear as an external USB drive. It will already have a few files on it.<br/>![](img/circuitpy-img0704.jpg)

1. **Drag the audio folder** from Windows to CIRCUITPY.<br/>
   - Be sure to store all of the WAV files (there are at least 36 of them) in a top-level folder named "audio".
   - Note that file names and folders are case-sensitive.<br/>![](img/drag-audio-to-circuitpy-img0705.jpg)
   - Now we're all done with CircuitPy mode, although you can use re-install CircuitPy later for direct access to the file system.

<h2 id="griduino">4. Re-install Griduino Program</h2>

You can re-install the Griduino software you saved earlier, or you may download the latest software from GitHub.

1. **Open Feather as a Drive**<br/>
   - Press the Feather's "reset" button **twice** rapidly, about a tenth of a second apart.
   - Find the new drive, e.g. "**FEATHERBOOT (F:)**"
   - If the USB device name is CIRCUITPY, then double-click the Reset button again until you get FEATHERBOOT.

1. **Install Griduino Software on Feather M4 Express**<br/>
   - Drag the .UF2 file that you saved earlier and drop it on the new FEATHERBOOT drive. Or you can copy/paste it to the new drive.
   - After the copying completes, Griduino will automatically restart with the new software.
   - First it shows an animation, then it shows a credits screen with the program name and version number, then a hints screen with a visual reminder of the default touch-sensitive areas.

1. **Test Griduino's audio playback**<br/>
   - If you installed audio files: Start the Griduino program
   - Press the "gear" icon until you see the **Audio Type** screen.<br/>Select **Spoken Word**.<br/>It should immediately announce your grid square.<br/>If it beeps, then Griduino did not find the audio files.
   - Press the "gear" icon until you see the **Speaker Volume** screen.<br/>Press the up/down buttons to adjust a comfortable volume.<br/>It should play an audio sample on each press.
   - To troubleshoot errors, open the **Arduino IDE** and click **Tools** > **Serial Monitor** to read error messages.

<h2 id="uf2">5. Understanding File System's Configuration Files</h2>

Griduino automatically creates and updates its internal configuration files. These are in a binary format you'll never need to work with them. However, it may be interesting to know where they are and how they work.

The files are stored in Flash memory in a section of onboard memory. To see the directory of files:

* **Terminal window:** Open a terminal window and send the command "list files", or
* **CircuitPy:** Reload Griduino with the CircuitPy program and use Windows File Explorer

The files you can expect to see are:

* Directory of audio
   * `19004 0.wav`
   * `24712 1.wav`
   * `19602 2.wav`
   * etc
   * `25040 z.wav`
   * `38 files, 799330 bytes`
* Directory of Griduino
   * ` 97 announce.cfg` - Morse code, Spoken word, No audio
   * `272 gpshistory.csv` - GPS breadcrumb trail
   * `216 gpsmodel.cfg` - latitude, longitude, altitude, metric/english, timezone
   * ` 97 nmea.cfg` - send NMEA sentences to USB cable yes/no
   * `100 screen.cfg` - screen orientation
   * `100 volume.cfg` - audio volume
   * `9 files, 882 bytes`

<h2 id="more">Appendix. Griduino's File System</h2>

Here is additional information about Griduino's file system:

1. (Understanding Configuration Files)[https://github.com/barry-ha/Griduino/blob/master/docs/PROGRAMMING.md#5-understanding-file-systems-configuration-files]
1. (How to Use a Serial Console)[https://github.com/barry-ha/Griduino/blob/master/docs/PROGRAMMING.md#8-how-to-use-a-serial-console]
1. (Griduino Programming Instructions)[https://github.com/barry-ha/Griduino/blob/master/docs/PROGRAMMING.md]
