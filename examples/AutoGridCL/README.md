# AutoGridCL

This is a command-line program to update WSJT-X from Griduino or a GPS.

## Install Software

Install Python

* Download and Install Python https://www.python.org/downloads/
* Add Python to PATH https://datatofish.com/add-python-to-windows-path/
* Verify Python works by ... 

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
* UDP Server: 224.0.0.1 <br/>*224.0.0.1 is the most compatible with JTAlert, GridTracker, etc and is not routed on your LAN*<br/>*127.0.0.1 can be specified if you use WSJT only with AutoGridCL and no other programs*
* UDP Server port number: 2237 <br/>*2237 is the default*
* Outgoing interfaces: loopback_0 <br/>*a loopback interface is required*
* Multicast TTL: 1 <br/>*this is Time-To-Live, a small number*
 
## Set up AutoGridCL

You need to modify the file 'AutoGridCL.py' so it can read NMEA sentences from Griduino and UDP packets from WSJT-X.

Using a text editor (e.g. Notepad or Visual Studio Code), make the following edits to 'AutoGridCL.py' 

* line 7: <b>GPS_PORT = "COM53"</b> <br/>*must match that of your Griduino or GPS*
* line 8: <b>GPS_RATE = 115200</b> <br/>*Griduino always runs at 115200 baud. Other GPS devices often use 9600 baud.*
* line 11: <b>MULTICAST_ADDR = "224.0.0.1"</b> <br/>*must match the Multicast IP address in WSJT-X*
* line 12: <b>MULTICAST_PORT = 2237</b> <br/>*must match the Multicast UDP port in WSJT-X*

## Running AutoGridCL

Start WSJT-X and set your grid using File>Settings>General to a grid different than the grid you are in and check the box to enable Autogrid. Save settings.

WSJT-X must be started before AutoGridCL.py.

Run AutoGridCL.py by double-clicking it in your File Explorer.

You should see a command line window open with its console messages.

Then you should start to see UDP packets read from WSJT-X and, after Griduino syncs with satellites, some NMEA sentences from the GPS.

If everything is set up properly, you should see your TX6 message in WSJT-X change to the gridsquare you are actually in.

When you end AutoGridCL, its last grid will linger until you change it, or restart WSJT-X, or toggle autogrid off/on. When you restart WSJT-X, it will always remember and revert to the grid square you originally configured.

## Troubleshooting

Error reporting is very crude in this prototype program. Some resourcefulness is required to troubleshoot at this early stage. 

Here are some suggestions that might help.
* If a command-line window does not appear after starting AutoGridCL.py, then it might have an exception and have ended right away. To see messages, open a command window, change to the directory where the file is located, and type 'python AutoGridCL.py'
* If you don't see NMEA messages from Griduino, reboot it and watch the screen to see the version number. Griduino software version must be 1.10 or later.
* Ask Barry questions: barry@k7bwh.com.

## Credit
Thanks goes to:
* Francis KV5W, https://kv5w.com/2021/10/17/wsjt-x-autogrid/
* Brian Moran N9ADG, https://github.com/bmo/py-wsjtx

 