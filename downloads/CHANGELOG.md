# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and the Griduino project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Version numbers correspond to [Downloads](https://github.com/barry-ha/Griduino/tree/master/downloads) available as pre-compiled binary Griduino programs. To install them, see the [Programming](https://github.com/barry-ha/Griduino/blob/master/docs/PROGRAMMING.md) instructions.

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

