#!/bin/bash
now=$(date)
echo "$now" > arduino_cli_setup.log

echo "Install / Upgrade ESP32 core" |tee -a arduino_cli_setup.log
echo "##########" >>arduino_cli_setup.log

echo "arduino-cli --config-file arduino-cli.yml core search esp32" |tee -a arduino_cli_setup.log
arduino-cli --config-file arduino-cli.yml core search esp32 >>arduino_cli_setup.log 2>&1
echo "##########" >>arduino_cli_setup.log

echo "arduino-cli --config-file arduino-cli.yml core install esp32:esp32" |tee -a arduino_cli_setup.log
arduino-cli --config-file arduino-cli.yml core install esp32:esp32 >>arduino_cli_setup.log 2>&1
echo "##########" >>arduino_cli_setup.log

echo "arduino-cli --config-file arduino-cli.yml core update-index" |tee -a arduino_cli_setup.log
arduino-cli --config-file arduino-cli.yml core update-index >>arduino_cli_setup.log 2>&1
echo "##########" >>arduino_cli_setup.log

echo "arduino-cli --config-file arduino-cli.yml core upgrade" |tee -a arduino_cli_setup.log
arduino-cli --config-file arduino-cli.yml core upgrade >>arduino_cli_setup.log 2>&1
echo "##########" >>arduino_cli_setup.log

# ########## loadLibs ##########

loadLibs=("Adafruit NeoPixel" "AsyncTCP" "ESPAsyncWebSrv" "DNSServer")

for l in "${loadLibs[@]}"; do
  echo "Install / Upgrade $l Library" |tee -a arduino_cli_setup.log
  echo "##########" >>arduino_cli_setup.log

  echo "arduino-cli --config-file arduino-cli.yml lib search '$l'" |tee -a arduino_cli_setup.log
  arduino-cli --config-file arduino-cli.yml lib search "$l" >>arduino_cli_setup.log 2>&1
  echo "##########" >>arduino_cli_setup.log

  echo "arduino-cli --config-file arduino-cli.yml lib install '$l'" |tee -a arduino_cli_setup.log
  arduino-cli --config-file arduino-cli.yml lib install "$l" >>arduino_cli_setup.log 2>&1
  echo "##########" >>arduino_cli_setup.log

  echo "arduino-cli --config-file arduino-cli.yml lib upgrade '$l'" |tee -a arduino_cli_setup.log
  arduino-cli --config-file arduino-cli.yml lib upgrade "$l" >>arduino_cli_setup.log 2>&1
  echo "##########" >>arduino_cli_setup.log
done


