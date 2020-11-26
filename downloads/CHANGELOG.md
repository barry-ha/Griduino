# Changelog
All notable changes to this project are documented in this file.

The changelog format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and the Griduino project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Version numbers correspond to [Downloads](https://github.com/barry-ha/Griduino/tree/master/downloads) available as pre-compiled binary Griduino programs. To install them, see the [Programming](https://github.com/barry-ha/Griduino/blob/master/docs/PROGRAMMING.md) instructions.

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

