#!/bin/bash

###########
# HELP TXT
###########
Help()
{
   # Display Help
   echo "This script requires TWO arguments."
   echo
   echo "Syntax: esptool-upload.sh <Sketch_Name> <Serial_Port>"
   echo
}

#################
# Check For Args
#################
if [ $# -eq 0 ] || [ -z "$2" ] || ! [ -z "$3" ]
  then
    Help
    exit;
fi

###################
# Check For Sketch
###################
if ! [ -d "$1" ]
  then
    echo "Sketch $1 Not Found"
    exit;
fi

###################
# Check For Serial
###################
if [ -z `arduino-cli --config-file arduino-cli.yml board list | grep "$2" | cut -d " " -f 1` ]
  then
    echo "Serial $2 Not Found"
    echo "Verify the serial port the board is connected to with:"
    echo "arduino-cli --config-file arduino-cli.yml board list"
    exit;
fi

# Set the baud rate of the serial port (This is needed for certain computers)
stty -f $2 460800

# Flash the firmware using esptool.py
esptool.py --chip esp32 --port $2 --baud 460800 write_flash -z 0x1000 $1/build/esp32.esp32.esp32/$1.ino.bin
