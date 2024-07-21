# Code

This is where all the project related Code / Firmware is located

# File Structure

This is the file structure of this repository

* [/](/README.md) - Home
* [/code/](/code/) - YOU ARE HERE

# Arduino IDE/CLI

The code presented here is mostly Arduino code.

In order to use this code and push new code to the ESP32 on the badge we recommend using the Arduino IDE/CLI software.
This can be downloaded for various platforms at [arduino.cc](https://www.arduino.cc/en/software).

The Arduino IDE UI will require some manual configuration to work with the badge ESP32 and our code.

The Arduino CLI can be used instead and allows for scripting much of the initial setup and use of whichever IDE you like (in my case I use VSCode).

# Arduino Versions

These are the Arduino IDE and CLI versions we have tested the code with:

* Arduino IDE v 2.3.2
  * [Windows MSI Link](https://downloads.arduino.cc/arduino-ide/arduino-ide_2.3.2_Windows_64bit.msi)
  * [Linux 64bit AppImage Link](https://downloads.arduino.cc/arduino-ide/arduino-ide_2.3.2_Linux_64bit.AppImage)
  * [MacOS ARM64bit DMG Link](https://downloads.arduino.cc/arduino-ide/arduino-ide_2.3.2_macOS_arm64.dmg)
  * [Installation Instructions](https://docs.arduino.cc/software/ide-v2/tutorials/getting-started/ide-v2-downloading-and-installing)
* Arduino CLI v 0.35.3
  * [Windows MSI Link](https://github.com/arduino/arduino-cli/releases/download/v0.35.3/arduino-cli_0.35.3_Windows_64bit.msi)
  * [Linux 64bit Tarball Link](https://github.com/arduino/arduino-cli/releases/download/v0.35.3/arduino-cli_0.35.3_Linux_64bit.tar.gz)
  * [MacOS ARM64bit Tarball Link](https://github.com/arduino/arduino-cli/releases/download/v0.35.3/arduino-cli_0.35.3_macOS_ARM64.tar.gz)
  * [Installation Instructions](https://arduino.github.io/arduino-cli/0.35/installation/)

WARNING - DO NOT use Arduino CLI v 1.x if you want the scripts here to work, it is very buggy concerning local config files and relative paths

# Quickstart

If you want to jump right in and start compiling and uploading code using the Arduino CLI and the scripts provided then checkout the [QUICKSTART page](QUICKSTART.md).

# Arduino IDE setup and use

Some additional setup is required to use the Arduino IDE with the ESP32 on the badge:

* Add the ESP32 boards manager URL in preferences
  * Under File->Preferences find the "Additional boards manager URLs" section and add the following
  * https://espressif.github.io/arduino-esp32/package_esp32_index.json
  * Then click OK and close the IDE.  When it is reopened it wil download the ESP32 board definitions.
* Select the board "ESP32 Dev Module"
  * You need to select a board to associate with the code even if that board is not connected
  * In the "Select Board" dropdown, choose "ESP32 Dev Module" from the list
  * The first time you do this it may download some addiotional software.
* Connect the ESP32 via USB cable and select the COM port if it is not autodetected
  * In most cases it should autodetect the port
  * Sometimes on first connect you need to help it by going to Tools->Port and choosing the port
* This code exceeds the normal partition size so make sure you select the Minimal SPIFFS Partition type
  * This setting can be found under Tools -> Partition Scheme
* This code uses the AdaFruit NeoPixel library which needs to be installed
  * The library can be installed via the Arduino IDE library manager or manually
  * Installation instructions can be found at [the adafruit neopixel github page](https://github.com/adafruit/Adafruit_NeoPixel)

Once the Arduino IDE environment has been set up you can simply open one of the sketches here, make whatever changes you like, then compile and upload the code to the badge.

# Arduino CLI

In an effort to bring portability to the code here (since this is lacking in the Arduino 2.x IDE) we will be providing a local configuration file for the Arduino CLI and scripts to make use of it.

While we cannot ensure this will work on all systems, the files provided should at a minimum help you configure your local environment.

## Using the CLI Local Config

The arduino cli will use a global default config if a config file is not specified.

A local config file is provided named ```arduino-cli.yml``` which uses relative path references for the data and staging and build and user area that Arduino CLI should use.

These directories are local to the repository code directory and the CLI will install all its needed files here instead of in the various places it usually defualts to on differnt systems.

While this might duplicate many files on your system if they are already installed in default locations, it provides a consistent cross platform way of knowing exactly what libraries, headers, etc., that Arduino will use in the compilation of your code.

NOTE that the .gitignore for this repository has been set up to ignore the local directories used to store the core and library files to prevent these files from accidentally getting uploaded to github.

To use the Arduino CLI with this local config file simply add the ```--config-file arduino-cli.yml``` argument to each CLI command.

For example, the command below uses the local config file and will dump the current config to the screen
```
arduino-cli --config-file arduino-cli.yml config dump
```

## Setup for CLI

There are a few things that need to be done to setup the Arduino CLI for use with the badge ESP32 and our code.

The ESP32 board core files must be loaded as well as any libraries the code will use.

To simplify this setup we have created setup bash script named ```cli-setup.sh``` that can be run on Linux and Windows (via Git Bash for Windows).

To perform the CLI setup via the script execute it in the code directory
```
bash ./cli-setup.sh
```

This script will save any std_out and std_err output to a file named ```arduino_cli_setup.log``` which you can review if you are having issues.

To perform the CLI setup manually without this script you can use the commands outlined below.

### Installing the ESP32 board core for CLI

Since the config is specifying a third party core for the ESP32 we need to pull an initial index from it.  This search command will pull that index.
```
arduino-cli --config-file arduino-cli.yml core search esp32
```

Next we need to install the ESP32 board core files using this command.
```
arduino-cli --config-file arduino-cli.yml core install esp32:esp32
```

If the ESP32 board core files were already installed the above will simply say already installed.

To update the core file indexs to check for newer versions of previously installed cores, use the following command
```
arduino-cli --config-file arduino-cli.yml core update-index
```

To upgrade any previously installed cores use the following command
```
arduino-cli --config-file arduino-cli.yml core upgrade
```

### Installing the Neopixel Library

This code uses the AdaFruit NeoPixel library which needs to be installed

If this is the first time we are installing a library we will need the indexes so this search command will get them
```
arduino-cli --config-file arduino-cli.yml lib search 'Adafruit NeoPixel'
```

Next we need to install the Adafruit NeoPixel library files with this command
```
arduino-cli --config-file arduino-cli.yml lib install 'Adafruit NeoPixel'
```

If the Adafruit NeoPixel library files were already installed the above will simply say already installed.

To upgrade a previously installed version of this library use the following command
```
arduino-cli --config-file arduino-cli.yml lib upgrade 'Adafruit NeoPixel'
```

### Installing Other Libraries

In addition to the Neopixel Library above the CTF portion of our code requires a few more libraries.

These should be installed using the same steps as above.

This is the list of additional libraries needed

* "AsyncTCP"
* "ESPAsyncWebSrv"
* "DNSServer"

## Connect badge ESP32 board via CLI

The following command will dump a list of all available board cores installed
```
arduino-cli --config-file arduino-cli.yml board listall
```

Make sure the ESP32 we are using on the badge is in that list
```
  ESP32 Dev Module                 esp32:esp32:esp32
```

If it is not then retry the CLI setup to install the core files.

Next we will connect the badge ESP32 to our machine using a USB C cable
and check if the CLI can see it.

The following command will list all the connected boards the CLI sees
```
arduino-cli --config-file arduino-cli.yml board list
```

This will show the detected COM port and board type (which may be Unknown)
```
  Port Protocol Type                Board Name FQBN Core
  COM4 serial   Serial Port (USB)   Unknown
```

Don't worry if the board FQBN shows up as Unknown since we can set that

Take note of the COM port since we will need that to communicate with it later

## Compile code via CLI

At this point we should be ready to compile and upload code to the badge ESP32 board.

In the following examples we will be referencing the sketch ```camera_badge_rev01``` code so you will need to change this for whichever sketch you wish to use.

The following command will compile the named sketch for use with our board fqbn
```
arduino-cli --config-file arduino-cli.yml compile --fqbn esp32:esp32:esp32 --build-path _build --export-binaries camera_badge_rev01 --build-property build.partitions=min_spiffs --build-property upload.maximum_size=1966080
```

The arg ```--build-path _build``` is optional to have it put all build artifacts here instead of temp for ease of troubleshooting.

The arg ```--export-binaries``` will save the final binary files into ```<sketch_name>/build/<fqbn>/``` for use by upload.

The build property args ```--build-property build.partitions=min_spiffs --build-property upload.maximum_size=1966080``` make sure that the partition size and upload size are set properly.
Our code is THIC and wont fit in the default partition size.

When the CLI compile is run it should show you compile stats and/or errors similar to this
```
  Sketch uses 1827197 bytes (92%) of program storage space. Maximum is 1966080 bytes.
  Global variables use 48884 bytes (14%) of dynamic memory, leaving 278796 bytes for local variables.
  Maximum is 327680 bytes.
  Used library        Version Path
  Adafruit NeoPixel   1.11.0  .\code\libraries\Adafruit_NeoPixel
  WiFi                2.0.0   .\code\_data\packages\esp32\hardware\esp32\2.0.11\libraries\WiFi
  AsyncTCP            1.1.4   .\code\libraries\AsyncTCP
  DNSServer           2.0.0   .\code\_data\packages\esp32\hardware\esp32\2.0.11\libraries\DNSServer
  ESPAsyncWebSrv      1.2.6   .\code\libraries\ESPAsyncWebSrv
  FS                  2.0.0   .\code\_data\packages\esp32\hardware\esp32\2.0.11\libraries\FS
  Used platform Version Path
  esp32:esp32   2.0.11  .\code\_data\packages\esp32\hardware\esp32\2.0.11
```

### Script to Compile code via CLI

To simplify the above we have created a bash script named ```cli-compile.sh``` that takes a single command line argument of the sketch name.

To perform the compile via the script execute it in the code directory
```
bash ./cli-compile.sh camera_badge_rev01
```

## Upload code via CLI

Now we are ready to upload the binary we compiled to the badge ESP32 board.

Note that the binary file needed is saved to ```<sketch_name>/build/<fqbn>/``` and will be named ```<sketch_name>.ino.bin```
The code we have written here may include binaries which can be used without compiling and if you make changes and compile then these files should be overwritten.

The following command will upload the named sketch binary to the badge
```
arduino-cli --config-file arduino-cli.yml upload -p COM4 --fqbn esp32:esp32:esp32 --input-file camera_badge_rev01/build/esp32.esp32.esp32/camera_badge_rev01.ino.bin
```

NOTE: Saving the compiled binary in a defined place and uploading from an input file allows for quickly changing between different code without the need to compile each again.

It should show you upload stats and/or errors
```
  esptool.py v4.5.1
  Serial port COM4
  Connecting....
  Chip is ESP32-D0WD-V3 (revision v3.0)
  Features: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None
  Crystal is 40MHz
  MAC: a8:42:e3:47:f0:50
  Uploading stub...
  Running stub...
  Stub running...
  Changing baud rate to 921600
  Changed.
  Configuring flash size...
  Flash will be erased from 0x00001000 to 0x00005fff...
  ...
  Compressed 18960 bytes to 13073...
  Writing at 0x00001000... (100 %)
  Wrote 18960 bytes (13073 compressed) at 0x00001000 in 0.3 seconds (effective 538.9 kbit/s)...
  Hash of data verified.
  ...
  Compressed 732656 bytes to 475614...
  Writing at 0x00010000... (3 %)
  Writing at 0x0001c1bd... (6 %)
  ...
  Writing at 0x000bcec0... (96 %)
  Writing at 0x000c2b72... (100 %)
  Wrote 732656 bytes (475614 compressed) at 0x00010000 in 6.3 seconds (effective 925.8 kbit/s)...
  Hash of data verified.
  Leaving...
  Hard resetting via RTS pin...
```

### Script to Upload code via CLI

To simplify the above we have created a bash script named ```cli-upload.sh``` that takes two command line arguments of the sketch name and serial port.

To perform the compile via the script execute it in the code directory
```
bash ./cli-upload.sh camera_badge_rev01 COM4
```

## Monitor Serial Output

To monitor the serial output you can use the arduino-cli or your favorite serial app.

The following command will list all the connected boards so you can get the serial port
```
arduino-cli --config-file arduino-cli.yml board list
```

The arduino-cli monitor command with serial port (from above) and baud config can be used like this:
```
arduino-cli monitor --port COM4 --config baudrate=115200
```
