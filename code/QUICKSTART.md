# Code Quickstart

Notes:

* You will need Arduino CLI Installed and a Bash shell (on Windows Git Bash is provided by Git for Windows)
* The CTF code is a precompiled library so you cant change that for now, source to be released after defcon
* The current code that is loaded on the badge is the sketch named ```camera_badge_rev01```

## Setting up Arduino CLI environment

Follow these steps:

* Change directory to the code directory if you arent already there
* Run the following command preceding it with ```bash``` if needed by your OS
  * ```./cli-setup.sh```
* If it completes without errors then setup is complete
  * If you have errors then refer to the main README doc or script file to try the steps manually

## Compiling Code

Once you have made whatever changes you wanted to the code here is how we compile it.

* Change directory to the code directory if you arent already there
* Run the following command preceding it with ```bash``` if needed by your OS changing the sketch name if needed
  * ```./cli-compile.sh camera_badge_rev01```
* If it completes without errors then the compile is complete
  * If you have errors then read what they are and make the appropriate changes to your code

## Upload Code

Once the code has been compiled we can upload it to the badge.

Follow these steps:

* Change directory to the code directory if you arent already there
* Run the following command
  * ```arduino-cli --config-file arduino-cli.yml board list```
* Take note of the serial port name of the connected board 
* Run the following command preceding it with ```bash``` if needed and changing the serial port to above
  * ```./cli-upload.sh camera_badge_rev01 COM6```

## Serial Monitor

Once the code has been uploaded you can use the serial monitor to look for any text the code is sending.

* Run the following command changing the serial port to what board list showed above
  * ```arduino-cli monitor --config baudrate=115200 --port COM6```
* On Linux
  * ```arduino-cli --config-file arduino-cli.yml monitor --config baudrate=115200 --port /dev/ttyUSB0```
  
