#!/bin/bash

########################################
## VARIABLE DEFAULTS
########################################
BoardSerial="not-set"
CodeSketch="camera_badge_rev01"

########################################
## COLOR TEXT SETUP
########################################
green='\e[32m'
blue='\e[34m'
yellow='\e[33m'
red='\e[31m'
ERED='\e[91m'
clear='\e[0m'
CTxtGreen(){
	echo -ne $green$1$clear
}
CTxtBlue(){
	echo -ne $blue$1$clear
}
CTxtYellow(){
	echo -ne $yellow$1$clear
}
CTxtRed(){
	echo -ne $red$1$clear
}
CTxtERROR(){
	echo -ne $ERED$1$clear
}
# Test Color Text
#echo -ne $(CTxtGreen 'Test Green ')
#echo -ne $(CTxtBlue 'Test Blue ')
#echo $(CTxtYellow 'Test Yellow ')

########################################
## MENU FUNCTIONS
########################################
GetBoardSerial(){
  BoardSerial=$(arduino-cli --config-file arduino-cli.yml board list | grep serial | head -n 1 | awk '{print $1}')
  if [ ! $BoardSerial ]
  then
    BoardSerial="not-set"
  fi
}
GetCodeSketch(){
  ls -d future*/
  read -e -p "$(CTxtGreen '####') $(CTxtBlue 'CHOOSE A CODE SKETCH >>') " -i "$CodeSketch" CodeSketch
}
MinicomSerialMonitor(){
  minicom -D $BoardSerial -b 115200
}
CompileCode(){
  if ! [ -d "$CodeSketch" ]
  then
    echo "$(CTxtERROR 'CODE SKETCH') $(CTxtYellow $CodeSketch) $(CTxtERROR 'NOT FOUND')"
    return;
  elif [ -f "$CodeSketch/build/esp32.esp32.esp32/$CodeSketch.ino.bin" ]
  then
    echo "$(CTxtERROR 'BINARY FOUND FOR CODE SKETCH') $(CTxtYellow $CodeSketch)"
    read -p "$(CTxtERROR 'THIS WILL OVERWRITE EXISTING BINARY, TYPE Y TO PROCEED >>') " CompileOverwrite
    if [ $CompileOverwrite == "Y" ] || [ $CompileOverwrite == "y" ]
    then
      echo "$(CTxtGreen 'PROCEEDING...')"
    else
      return;
    fi
  fi
  echo "$(CTxtYellow 'arduino-cli --config-file arduino-cli.yml compile --fqbn esp32:esp32:esp32 --build-path _build --export-binaries') $(CTxtRed $CodeSketch) $(CTxtYellow '--build-property build.partitions=min_spiffs --build-property upload.maximum_size=1966080') "
  arduino-cli --config-file arduino-cli.yml compile --fqbn esp32:esp32:esp32 --build-path _build --export-binaries $CodeSketch --build-property build.partitions=min_spiffs --build-property upload.maximum_size=1966080
  read -s -p 'Press any key to continue...'
  return;
}
UploadCode(){
  if [ $BoardSerial == "not-set" ]
  then
    echo "$(CTxtERROR 'BADGE SERIAL NOT SET')"
    return;
  elif ! [ -f "$CodeSketch/build/esp32.esp32.esp32/$CodeSketch.ino.bin" ]
  then
    echo "$(CTxtERROR 'BINARY NOT FOUND FOR CODE SKETCH') $(CTxtYellow $CodeSketch)"
    return;
  fi
  echo "$(CTxtYellow 'arduino-cli --config-file arduino-cli.yml upload -p') $(CTxtRed $BoardSerial) $(CTxtYellow '--fqbn esp32:esp32:esp32 --input-file') $(CTxtRed $CodeSketch)$(CTxtYellow '/build/esp32.esp32.esp32/')$(CTxtRed $CodeSketch)$(CTxtYellow '.ino.bin') "
  arduino-cli --config-file arduino-cli.yml upload -p $BoardSerial --fqbn esp32:esp32:esp32 --input-file $CodeSketch/build/esp32.esp32.esp32/$CodeSketch.ino.bin
  read -s -p 'Press any key to continue...'
  return;
}

########################################
## MENU SETUP
########################################
menu(){
if [ -f "$CodeSketch/build/esp32.esp32.esp32/$CodeSketch.ino.bin" ]
then
  CodeBinFlag="$(CTxtYellow 'YES')"
else
  CodeBinFlag="$(CTxtRed 'NO')"
fi
if [ $BoardSerial == "not-set" ]
then
  BoardSerialTxt="$(CTxtRed 'not-set')"
else
  BoardSerialTxt="$(CTxtYellow $BoardSerial)"
fi
echo -ne "
$(CTxtGreen '######################################################################')
$(CTxtGreen '####. . . . . . . . FECALMTR MENU FOR FUTURE BADGE . . . . . . . .####')
$(CTxtGreen '######################################################################')
$(CTxtGreen '#### BADGE SERIAL:') $BoardSerialTxt $(CTxtGreen 'CODE:') $(CTxtYellow $CodeSketch) $(CTxtGreen 'BIN:') $CodeBinFlag
$(CTxtGreen '######################################################################')
$(CTxtGreen '####') $(CTxtBlue '1 - UPDATE BADGE SERIAL . .') $(CTxtGreen '####') $(CTxtBlue '2 - CHANGE CODE SKETCH. . .') $(CTxtGreen '####')
$(CTxtGreen '####') $(CTxtBlue '3 - COMPILE CODE SKETCH . .') $(CTxtGreen '####') $(CTxtBlue '4 - UPLOAD CODE TO BADGE. .') $(CTxtGreen '####')
$(CTxtGreen '####') $(CTxtBlue '5 - MINICOM SERIAL MONITOR.') $(CTxtGreen '####') $(CTxtBlue '0 - EXIT MENU . . . . . . .') $(CTxtGreen '####')
$(CTxtGreen '######################################################################')
"
        read -p "$(CTxtGreen '####') $(CTxtBlue 'CHOOSE AN OPTION >>') " a
        case $a in
	        1) GetBoardSerial ; menu ;;
	        2) GetCodeSketch ; menu ;;
	        3) CompileCode ; menu ;;
	        4) UploadCode ; menu ;;
	        5) MinicomSerialMonitor ; menu ;;
			0) exit 0 ;;
			*) echo "$(CTxtERROR 'WRONG OPTION')"; menu;;
        esac
}

########################################
## MENU LAUNCH
########################################
menu
