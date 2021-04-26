# Changelog
To install a firmware update, see the [Programming](https://github.com/barry-ha/Griduino/blob/master/docs/PROGRAMMING.md#2-how-to-install-the-griduino-program) instructions.

All notable changes to this project are documented in this file.

Version numbers correspond to [Downloads](https://github.com/barry-ha/Griduino/tree/master/downloads) available as pre-compiled binary Griduino programs.

The changelog format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and the Griduino project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).


**v1.01** &nbsp; 2020-0426

New: Replaced the status screen with new "Grid Size and Scale" information.

This gives the user a sense of scale looking at our main screen map. It helps understand the bread crumb trail and how far to go within a 4-digit and 6-digit grid squares. The previous status screen was one of the first screens built and became redundant with other screens.

**v1.0** &nbsp; 2020-04-17

Griduino has all major features completed, so the version number jumps to **v1.0**. We like the [semantic versioning](https://semver.org/) scheme so we'll manage our version numbers accordingly. However, since Griduino doesn't have an API, our scheme is  modified to only use two numbers. 

Given a version number `MAJOR.MINOR`, we will increment the:

   * `MAJOR` version when adding major features or hardware changes, and
   * `MINOR` version for fixing bugs or for small incremental changes.

**v0.37** &nbsp; 2020-04-12

Added speech audio output as an alternative to Morse code. This is a major upgrade.

Griduino can now speak grid square names. This feature requires:

   1. Install audio recordings of the letters and numbers. These WAV files are copied onto the Feather separately using a different process than the binary program file. See https://github.com/barry-ha/Griduino/blob/master/docs/PROGRAMMING.md#3-install-audio-files
   1. Install the latest Griduino program, v0.37 or later
   1. Press the "gear" icon until you see the **Audio Type** screen.<br/>Select **Spoken Word**.<br/>It should immediately announce your grid square.<br/>If it beeps, then Griduino did not find the audio files.

**v0.36** &nbsp; Skipped

This was an internal testing version. The pre-compiled binary was not released.

**v0.35** &nbsp; 2020-02-25

Fixed several bugs in the Altimeter view. Added a small 'sync' button on right-hand side which calibrates the sea level pressure so that the two reported altitudes match each other. The GPS is not always right but it's pretty close and the 'sync' button will make large adjustments easy. 

Also fixed the three-day graph of barometric pressure. It had been graphing only two days and now it will show up to three full days.

Get this download and try it out! Recommended for all users.

**v0.34** &nbsp; 2020-02-11

Improved the Altimeter view usability. This is a work in progress.

**v0.33** &nbsp; 2021-02-09

Added Altimeter view which compares altitude from the barometer to altitude from the GPS. This is an interesting way to cross-check and decide for yourself the accuracy of reports.

The problem is that both of them can be off:

* An accurate reading from the barometer depends on knowing your current pressure at sea level at your location. The altimeter screen therefore offers + and - buttons to calibrate it. You can get the correct pressure by searching the web for a local weather report, or by adjusting it to a known altitude. 
* An accurate reading from the GPS depends on the number and position of satellites overhead. You would need a good satellite right overhead for best results. Further, signal reflections from nearby objects can throw it off. Unfortunately the consumer GPS service was designed to be more accurate positionally than for altitude. For example, at my home, the GPS reports can vary day-by-day by 300 feet or so.

**v0.32** &nbsp; 2021-02-03

Added 3-day graph of barometric pressure, a "Baroduino" if you'll excuse the amalgam. Please report any bugs or usability glitches. 

This is a major update that merges code from [examples/Baroduino](https://github.com/barry-ha/Griduino/tree/master/examples/Baroduino) into the main Griduino program which becomes an additional view in the screens as you cycle through views. The standalone example program is no longer needed. We also fixed a few bugs and changed titles of the configuration screens to be more descriptive, such as "1. Speaker" instead of "Settings 1".

**v0.31** &nbsp; 2021-01-30

Updated the pressure sensor code to use the latest BMP3XX library. Please update to the v2 library from Adafruit or you’ll get a compile error:
1	Run the Arduino workbench
1	Tools > Manage Libraries … > Type: Updateable
1	Find “Adafruit BMP3XX Library” in the list and update to the latest version
1	It’s okay to get all the latest library dependencies, too. The list has been updated and tested for v0.31.

When we started the Griduino project in early 2020, Adafruit offered only one barometric sensor: BMP388. In October 2020, Bosch introduced a more sensitive device, BMP390, and Adafruit followed suit to sell it on a pin-compatible breakout board. However, it’s not quite software-compatible. I bought and tested the new BMP390 to make sure it works successfully and today I checked in the code changes. Griduino software will now work with either barometric sensor and is a little more future-proof.

**v0.30** &nbsp; 2020-12-19

Fixed the background color of the activity indicator on the bottom line. The code change was to extend the base class in view.h so that every screen is allowed to have its own background color independent of other screens.

**v0.29** &nbsp; 2020-11-26

Created new example program to study TFT Resistive Touchscreen behavior and calibration. See [examples/TFT Touch Calibrator](https://github.com/barry-ha/Griduino/tree/master/examples/TFT_Touch_Calibrator). The idea is to display the touchscreen configuration values along with values actually measured when you actually touch it. The screen feedback shows how where each touch is mapped into screen coordinates. There is a certain amount of manufacturing variability and it's possible that my values don't suite your own device. If your particular touchscreen looks too far off, it reports the measured values which you can use to edit your source code and compile/run again.

**v0.28** &nbsp; 2020-11-12

In the graphing barometer, added "number of satellites" and today's date. Also added debug output for the serial-attached console to help track down if the RTC hangs up.

**v0.27** &nbsp; 2020-11-03

Added new feature to select how often grid-crossing announcements are made. You can choose either 4-digit grid lines (about 70 miles N-S and 100 miles E-W) or 6-digit grid lines (about 3 miles N-S and 4 miles E-W). The setting is implemented as a new 'view' in cfg_setting4.h and is retained in non-volatile RAM.

**v0.26** &nbsp; 2020-10-29

Added data logger for barometric pressure into the main Griduino program. Now it collects the weather history in the background while doing everything else.  Then, to display the pressure graph, download baroduino_v026.uf2.

**v0.25** &nbsp; 2020-10-03

Improved timing to make the GMT clock more closely match WWV.

Before this change, it typically displayed 'xx:59' seconds as the tone is heard.  But sometimes the 'xx:58' is displayed, and sometimes it skips the 'xx:59'  display and goes directly to the :00 seconds exactly on the 1 second 'tick'.

**v0.24** &nbsp; 2020-10-02

Added new view for a frivolous "Groundhog Day" counter display. We feel like we're stuck in a time loop, just like Bill Murray in his 1993 movie. Now we know how long we've been in the pandemic and self-imposed social distancing.

Also vastly updated the stadalone "Baroduino" example program.

**v0.23** &nbsp; 2020-09-02

Refactored views into base class "View" and derived classes. No visible change to usage and operation.

**v0.22** &nbsp; 2020-08-23

Added setting to show distance in miles/kilometers

**v0.21** &nbsp; 2020-08-20

Fixed audio volume set-save-restore bug

**v0.20** &nbsp; 2020-08-14
 
Added icons for gear, arrow

**v0.18** &nbsp; 2020-07-06

Added runtime selection of GPS receiver vs simulated trail

**v0.17** &nbsp; 2020-06-23

Added Settings control panel, and clear breadcrumb trail

**v0.16** &nbsp; 2020-06-03

Added GMT Clock view

**v0.15** &nbsp; 2020-05-30

Added simulated GPS track (class MockModel)

**v0.13** &nbsp; 2020-05-12

Implemented our own TouchScreen functions

**v0.12** &nbsp; 2020-05-09

Refactored screen writing to class TextField

**v0.11]** &nbsp; 2020-05-21

Added GPS save/restore to visually power up in the same place as previous spot

**v0.10** &nbsp; 2020-04-19

Added altimeter example program

**v09.8** &nbsp; 2020-02-18  

Added saving settings in 2MB RAM


**v09.4** &nbsp; 2020-02-18

Added a new view for controlling audio volume

**v09.3** &nbsp; 2020-01-01  

Made the Morse Code actually work, and replaces view-stat-screen

**v09.2** &nbsp; 2019-12-30  

Added Morse Code announcements via generated audio waveform on DAC.


**v09.0** &nbsp; 2019-12-20  

Generates sound by synthesized sine wave intended for decent fidelity from a small speaker. The hardware goal is to support spoken-word output.

