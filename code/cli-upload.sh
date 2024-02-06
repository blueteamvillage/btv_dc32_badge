#!/bin/bash

###########
# HELP TXT
###########
Help()
{
   # Display Help
   echo "This script requires TWO arguments."
   echo
   echo "Syntax: cli-upload.sh <Sketch_Name> <Serial_Port>"
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

echo "arduino-cli --config-file arduino-cli.yml upload -p $2 --fqbn esp32:esp32:esp32 --input-file $1/build/esp32.esp32.esp32/$1.ino.bin"
arduino-cli --config-file arduino-cli.yml upload -p $2 --fqbn esp32:esp32:esp32 --input-file $1/build/esp32.esp32.esp32/$1.ino.bin
