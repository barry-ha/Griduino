# AutoGridCL

This is a command-line program to update the grid square in WSJT-X from Griduino in real time.

## Install Software

Install Python

* Download and Install Python https://www.python.org/downloads/
* Add Python to PATH https://datatofish.com/add-python-to-windows-path/

Get AutoGridCL files into a local folder

* Visit GitHub at https://github.com/barry-ha/Griduino
* Fetch the files in the 'examples/AutoGridCL' folder, currently:
  * AutoGridCL.py
  * pywsjtx/__init__.py
  * pywsjtx/latlong_to_grid_square.py
  * pywsjtx/simple_server.py
  * pywsjtx/wsjtx_packets.py
 
## Set up WSJT-X for multicast

Run WSJT-X and configure File > Settings > Reporting:
* <b>UDP Server: 224.0.0.1</b> <br/>*224.0.0.1 is the most compatible with JTAlert, GridTracker, HRD, etc and these packets are not routed outboard onto your LAN*<br/>*127.0.0.1 can be used if you use WSJT only with AutoGridCL and no other programs*
* <b>UDP Server port number: 2237</b> <br/>*2237 is the default*
* <b>Outgoing interfaces: loopback_0</b> <br/>*a loopback interface is required*
* <b>Multicast TTL: 1</b> <br/>*this is a packet's Time-To-Live, a small number of seconds*
 
## Set up AutoGridCL

You need to modify the file 'AutoGridCL.py' so it can read NMEA sentences from Griduino and send UDP packets to and from WSJT-X.

Using a text editor (e.g. Notepad or Visual Studio Code), modify 'AutoGridCL.py': 

* line 7: <b>GPS_PORT = "COM53"</b> <br/>*must match that of your Griduino or GPS*
* line 8: <b>GPS_RATE = 115200</b> <br/>*Griduino always runs at 115200 baud. Other GPS devices often use 9600 baud.*
* line 11: <b>MULTICAST_ADDR = "224.0.0.1"</b> <br/>*must match the Multicast IP address in WSJT-X*
* line 12: <b>MULTICAST_PORT = 2237</b> <br/>*must match the Multicast UDP port in WSJT-X*

## Running AutoGridCL

WSJT-X must be started before AutoGridCL.py.

Start WSJT-X and set your grid using File>Settings>General to a grid different than the grid you are in and check the box to enable Autogrid.

Run AutoGridCL.py by double-clicking it in your File Explorer.

You should see a command line window open with its console messages.

Then you should start to see UDP packets read from WSJT-X and, after Griduino syncs with satellites, some NMEA sentences from the GPS.

If everything is set up properly, you should see your TX6 message in WSJT-X change to the gridsquare you are actually in.

When you end AutoGridCL, its last grid will linger until you change it, or restart WSJT-X, or toggle autogrid off/on. When you restart WSJT-X, it will always remember and revert to the grid square you originally configured.

## Troubleshooting

Error reporting is very crude in this prototype program. You will need to have some resourcefulness to troubleshoot problems at this early stage.

In theory, this program should work with several different USB-attached GPS pucks. As long as the GPS sends standard NMEA data with lat/long position in $GPGGA or $GPGLL sentences, this will automatically decode them.

Here are some suggestions that might help.
* If a command-line window does not appear after starting AutoGridCL.py, then it might have an exception and have ended right away. To see messages, open a command window, change to the directory where the file is located, and type 'python AutoGridCL.py'
* If you don't see NMEA messages from Griduino, reboot it and watch the screen to see the version number. Griduino software version must be 1.10 or later.
* If this program can't connect to Griduino, close any programs using it (such as NMEA Time2 or the Arduino workbench) before starting AutoGridCL. Note that the USB port cannot be shared, so only one program at a time can listen to Griduino. For extra fun, note that NMEA Time2 is a background service and cannot be closed; you must configure its GPS setting to 'Off' to disconnect it from the serial port.
* Tell Barry about problems you run into so he can add it to this list: barry@k7bwh.com.

## Credit
Thanks goes to:
* Francis KV5W, https://kv5w.com/2021/10/17/wsjt-x-autogrid/
* Brian Moran N9ADG, https://github.com/bmo/py-wsjtx

 