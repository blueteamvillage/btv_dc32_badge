// camera_badge_rev01

// Include Libraries
#include <Arduino.h>
#include <WiFi.h>
#include "Adafruit_NeoPixel.h"
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_GC9A01A.h"
#include <esp_wifi_types.h>
#include <esp_wifi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Pin Definitions
//
// Display Pins
#define TFT_DC 16
#define TFT_CS 5
#define TFT_RST 4
// Note: SPI DATA & CLOCK defined by pins_arduino.h in ESP32 Board Library
// SPI data pin = MOSI 23
// SPI clock pin = SCK 18
//
// NeoPixel Data Pins
#define NEO01_DATA 17
#define NEO02_DATA 19
//
// One color LED Pins
#define LED_D2 32
#define LED_D3 27
#define LED_D4 26
#define LED_D5 33
#define LED_D6 25
#define LED_D7 14
//
// Built-in LED
#define LED_BI 22
//
// Capacitive Touch Pins
#define TCH01_PIN 13
#define TCH03_PIN 15
#define TCH05_PIN 12

// Display Properties
//
// Display constructor for primary hardware SPI connection
Adafruit_GC9A01A tft(TFT_CS, TFT_DC);

// NeoPixel Properties
//
// Define NeoPixel Strips - (Num pixels, pin to send signals, pixel type, signal rate)
Adafruit_NeoPixel NEO01 = Adafruit_NeoPixel(2, NEO01_DATA, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel NEO02 = Adafruit_NeoPixel(1, NEO02_DATA, NEO_RGB + NEO_KHZ800);
// Status NeoPixel LED color mode bitwise off=0 red=1 green=2 blue=4
int status_neo_mode = 0;

// PWM Properties
//
// Signal Frequency in Hz
const int freq = 1000;
// Duty Cycle Resolution in bits (1-16)
const int resolution = 8;
// PWM channel Assignment (0-15)
const int LED_D3_pwm = 3;
const int LED_D4_pwm = 4;
const int LED_D5_pwm = 5;
const int LED_D6_pwm = 6;
const int LED_D7_pwm = 7;

// Wireless Properties
//
// WIFI status codes
const char *wl_status_to_string(wl_status_t status)
{
  switch (status)
  {
  case WL_NO_SHIELD:
    return "WL_NO_SHIELD";
  case WL_IDLE_STATUS:
    return "WL_IDLE_STATUS";
  case WL_NO_SSID_AVAIL:
    return "WL_NO_SSID_AVAIL";
  case WL_SCAN_COMPLETED:
    return "WL_SCAN_COMPLETED";
  case WL_CONNECTED:
    return "WL_CONNECTED";
  case WL_CONNECT_FAILED:
    return "WL_CONNECT_FAILED";
  case WL_CONNECTION_LOST:
    return "WL_CONNECTION_LOST";
  case WL_DISCONNECTED:
    return "WL_DISCONNECTED";
  }
}

//
// DEAUTH SNIFF Properties
#define MAX_CHANNELS 13
const unsigned long CHANNEL_HOP_INTERVAL = 2500;
unsigned long previousChannelHopTime = 0;
const wifi_promiscuous_filter_t filt = {.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};
int currentChannel = 1;
unsigned long deauthThreshold = 20;
unsigned long deauthCount = 0;
typedef struct
{
  int16_t fctl;
  int16_t duration;
  uint8_t da;
  uint8_t sa;
  uint8_t bssid;
  int16_t seqctl;
  unsigned char payload[];
} WifiMgmtHdr;
typedef struct
{
  unsigned frame_ctrl : 16;
  unsigned duration_id : 16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl : 16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;
typedef struct
{
  uint8_t payload[0];
  wifi_ieee80211_mac_hdr_t hdr;
} wifi_ieee80211_packet_t_2;
typedef struct
{
  uint8_t payload[0];
  WifiMgmtHdr hdr;
} wifi_ieee80211_packet_t;
bool deauth_sniff_active = false;

//
// BLE Properties
const int SCAN_TIME = 5; // scan duration in seconds
const String FLIPPER_MAC_PREFIX_1 = "80:e1:26";
const String FLIPPER_MAC_PREFIX_2 = "80:e1:27";
BLEScan *pBLEScan;
int flipperCount = 0;
bool flipper_scan_active = false;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    String macAddress = String(advertisedDevice.getAddress().toString().c_str());
    if (macAddress.startsWith(FLIPPER_MAC_PREFIX_1) || macAddress.startsWith(FLIPPER_MAC_PREFIX_2))
    {
      flipperCount++;
    }
  }
};
//

// Capacitive Touch Properties
//
// Touch Thresholds
int Touch01_Threshold = 19;
int Touch03_Threshold = 19;
int Touch05_Threshold = 19;
// Touch Initial Values
int Touch01_Value = 21;
int Touch03_Value = 21;
int Touch05_Value = 21;
//
// Touch Counters
//
// Touch Iteration Counter
int Touch01_IntCount = 0;
int Touch03_IntCount = 0;
int Touch05_IntCount = 0;
// Touch Iteration Flag
int Touch01_IntFlag = 0;
int Touch03_IntFlag = 0;
int Touch05_IntFlag = 0;
// Touch Loop Counter
int Touch01_LoopCount = 0;
int Touch03_LoopCount = 0;
int Touch05_LoopCount = 0;
// Touch Loop Threshold (Touch Held for X Loops of Main)
int Touch01_Loop_Threshold = 3;
int Touch03_Loop_Threshold = 3;
int Touch05_Loop_Threshold = 3;

// Loop Control Properties
//
// Main Loop LED Iteration Delay Time [in ms]
int LEDDelayTime = 20;
//
// Debug Serial - If set greater than 0 it writes to serial for debugging
// 0 = no debug text
// 1 = basic debug info once per main loop
// 2 = extra debug info
int DebugSerial = 2;

// //////////////////////////////////////////////////
//
// SETUP - RUN ONCE
//
// //////////////////////////////////////////////////
void setup()
{
  // Add a delay to allow opening serial monitor
  delay(800);

  // setup the serial output baud rate
  Serial.begin(115200);

  if (DebugSerial >= 1)
  {
    Serial.println("Starting Setup");
  }

  // Turn Off WiFi/BT
  if (DebugSerial >= 2)
  {
    Serial.println("Turn Off WiFi / BlueTooth");
  }
  setModemSleep();

  // Configure LED PWM functionalitites per channel
  if (DebugSerial >= 2)
  {
    Serial.println("Configure PWM Channels");
  }
  ledcSetup(LED_D3_pwm, freq, resolution);
  ledcSetup(LED_D4_pwm, freq, resolution);
  ledcSetup(LED_D5_pwm, freq, resolution);
  ledcSetup(LED_D6_pwm, freq, resolution);
  ledcSetup(LED_D7_pwm, freq, resolution);

  // Attach the channel to the GPIO to be controlled
  if (DebugSerial >= 2)
  {
    Serial.println("Attach PWM Channels to LED Pins");
  }
  ledcAttachPin(LED_D3, LED_D3_pwm);
  ledcAttachPin(LED_D4, LED_D4_pwm);
  ledcAttachPin(LED_D5, LED_D5_pwm);
  ledcAttachPin(LED_D6, LED_D6_pwm);
  ledcAttachPin(LED_D7, LED_D7_pwm);

  // Normal LED output
  if (DebugSerial >= 2)
  {
    Serial.println("Set Output for non-PWM LED Pins");
  }
  pinMode(LED_D2, OUTPUT);
  pinMode(LED_BI, OUTPUT);

  // Initialize the NeoPixels
  if (DebugSerial >= 2)
  {
    Serial.println("Initialize NeoPixels");
  }
  NEO01.begin();
  NEO02.begin();
  // Set Neopixel Brightness (0-255 scale)
  NEO01.setBrightness(170);
  NEO02.setBrightness(170);

  // Initialize the Display
  if (DebugSerial >= 2)
  {
    Serial.println("Initialize Display");
  }
  tft.begin();

  // Start all LEDs in OFF mode
  if (DebugSerial >= 2)
  {
    Serial.println("Turn OFF all LEDs");
  }
  ledAllOff();

  //
  // Random Seed based on Touch03_Value
  randomSeed(touchRead(TCH03_PIN));

  if (DebugSerial >= 1)
  {
    Serial.println(F("Setup Done!"));
  }

  // END OF SETUP
}

// //////////////////////////////////////////////////
//
// LOOP - MAIN
//
// //////////////////////////////////////////////////
void loop()
{
  if (DebugSerial >= 1)
  {
    Serial.println("******************** TOP OF MAIN LOOP ********************");
  }
  // Capacitive Touch Dynamic Threshold Adjustment
  // Adjust thresholds UP to account for assembly conditions and battery vs usb
  Touch01_Value = touchRead(TCH01_PIN);
  if ((Touch01_Value / Touch01_Threshold) > 2)
  {
    Touch01_Threshold = int(Touch01_Threshold * 1.8);
  }
  Touch03_Value = touchRead(TCH03_PIN);
  if ((Touch03_Value / Touch03_Threshold) > 2)
  {
    Touch03_Threshold = int(Touch03_Threshold * 1.8);
  }
  Touch05_Value = touchRead(TCH05_PIN);
  if ((Touch05_Value / Touch05_Threshold) > 2)
  {
    Touch05_Threshold = int(Touch05_Threshold * 1.8);
  }

  // //////////////////////////////////
  //     START OF ITERATION LOOP
  // //////////////////////////////////
  //
  // Iterate 0 to 254
  for (int i = 0; i < 255; i++)
  {
    // Set position value to iteration value
    int pos = i;

    // DEBUG - Print current Iteration value to serial console for troubleshooting
    if (DebugSerial >= 2)
    {
      Serial.print(" Iteration=");
      Serial.print(i);
      Serial.print(" Pos=");
      Serial.print(pos);
    }

    // **********
    // TOUCH
    // **********
    //
    // Read Touch Values
    Touch01_Value = touchRead(TCH01_PIN);
    Touch03_Value = touchRead(TCH03_PIN);
    Touch05_Value = touchRead(TCH05_PIN);
    //
    // **********
    // TCH01
    // **********
    //
    // Do Stuff If We Detect a Touch on TCH01_PIN
    if (Touch01_Value < Touch01_Threshold)
    {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2)
      {
        Serial.print(" T1_TCH=");
        Serial.print(Touch01_Value);
        Serial.print("/");
        Serial.print(Touch01_Threshold);
        Serial.print("-");
        Serial.print(Touch01_IntCount);
        Serial.print("/");
        Serial.print(Touch01_LoopCount);
      }
      // STUFF - TCH01_PIN TOUCHED
      if (Touch01_IntFlag == 0)
      {
        // Put stuff to happen once per iteration loop here
        Touch01_IntFlag = 1;
        //
        // *** Add more stuff here if needed ***
        //
      }
      // Put stuff to happen every iteration here
      Touch01_IntCount++;
      //
      // *** Add more stuff here if needed ***
      title_neo_blue();
      //
      // Do Stuff If We DONT Detect a Touch on TCH01_PIN
    }
    else
    {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2)
      {
        Serial.print(" T1=");
        Serial.print(Touch01_Value);
        Serial.print("/");
        Serial.print(Touch01_Threshold);
        Serial.print("-");
        Serial.print(Touch01_IntCount);
        Serial.print("/");
        Serial.print(Touch01_LoopCount);
      }
      // STUFF - TCH01_PIN NOT TOUCHED
      if (Touch01_IntCount > 1)
      {
        Touch01_IntCount--;
      }
      else
      {
        Touch01_IntCount = 0;
      }
      //
      // *** Add more stuff here if needed ***
      //
    }
    //
    // **********
    // TCH03
    // **********
    //
    // Do Stuff If We Detect a Touch on TCH03_PIN
    if (Touch03_Value < Touch03_Threshold)
    {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2)
      {
        Serial.print(" T3_TCH=");
        Serial.print(Touch03_Value);
        Serial.print("/");
        Serial.print(Touch03_Threshold);
        Serial.print("-");
        Serial.print(Touch03_IntCount);
        Serial.print("/");
        Serial.print(Touch03_LoopCount);
      }
      // STUFF - TCH03_PIN TOUCHED
      if (Touch03_IntFlag == 0)
      {
        // Put stuff to happen once per iteration loop here
        Touch03_IntFlag = 1;
        //
        // *** Add more stuff here if needed ***
        //
        //
      }
      // Put stuff to happen every iteration here
      Touch03_IntCount++;
      //
      // *** Add more stuff here if needed ***
      title_neo_blue();
      // Do Stuff If We DONT Detect a Touch on TCH03_PIN
    }
    else
    {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2)
      {
        Serial.print(" T3=");
        Serial.print(Touch03_Value);
        Serial.print("/");
        Serial.print(Touch03_Threshold);
        Serial.print("-");
        Serial.print(Touch03_IntCount);
        Serial.print("/");
        Serial.print(Touch03_LoopCount);
      }
      // STUFF - TCH03_PIN NOT TOUCHED
      if (Touch03_IntCount > 1)
      {
        Touch03_IntCount--;
      }
      else
      {
        Touch03_IntCount = 0;
      }
      //
      // *** Add more stuff here if needed ***
      //
    }
    //
    // **********
    // TCH05
    // **********
    //
    // Do Stuff If We Detect a Touch on TCH05_PIN
    if (Touch05_Value < Touch05_Threshold)
    {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2)
      {
        Serial.print(" T5_TCH=");
        Serial.print(Touch05_Value);
        Serial.print("/");
        Serial.print(Touch05_Threshold);
        Serial.print("-");
        Serial.print(Touch05_IntCount);
        Serial.print("/");
        Serial.print(Touch05_LoopCount);
      }
      // STUFF - TCH05_PIN TOUCHED
      if (Touch05_IntFlag == 0)
      {
        // Put stuff to happen once per iteration loop here
        Touch05_IntFlag = 1;
        //
        // *** Add more stuff here if needed ***
        //
      }
      // Put stuff to happen every iteration here
      Touch05_IntCount++;
      //
      // *** Add more stuff here if needed ***
      digitalWrite(LED_D2, HIGH);
      //
      // Do Stuff If We DONT Detect a Touch on TCH05_PIN
    }
    else
    {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2)
      {
        Serial.print(" T5=");
        Serial.print(Touch05_Value);
        Serial.print("/");
        Serial.print(Touch05_Threshold);
        Serial.print("-");
        Serial.print(Touch05_IntCount);
        Serial.print("/");
        Serial.print(Touch05_LoopCount);
      }
      // STUFF - TCH05_PIN NOT TOUCHED
      if (Touch05_IntCount > 1)
      {
        Touch05_IntCount--;
      }
      else
      {
        Touch05_IntCount = 0;
      }
      //
      // *** Add more stuff here if needed ***
      digitalWrite(LED_D2, LOW);
      //
    }

    // ********************
    // ITERATION POS GROUPS
    // ********************
    //
    // First of three position groups i 0-84
    if (pos < 85)
    {
      //
      if (pos == 0)
      {
        cameraShutter1();
        // cameraShutter2();
      }
      //
      // LED FUNCTIONS
      title_neo_colorshift(pos, 1);
      ledPwmSet(pos, 1);
      BI_blink_one(pos);
      // Second of three position groups i 85-169 (pos-85 = 0-84)
    }
    else if (pos < 170)
    {
      pos = pos - 85;
      //
      if (pos == 0)
      {
        // cameraShutter1();
        cameraShutter2();
      }
      //
      // LED FUNCTIONS
      title_neo_colorshift(pos, 2);
      ledPwmSet(pos, 2);
      BI_blink_one(pos);
      // Third of three position groups i 170-254 (pos-170 = 0-84)
    }
    else
    {
      pos = pos - 170;
      //
      // LED FUNCTIONS
      title_neo_colorshift(pos, 3);
      BI_blink_one(pos);
      // Split third group 3/4 (pos 0-42) for even number of transitions
      if (pos < 43)
      {
        //
        ledPwmSet(pos, 3);
        // Split third group 4/4 (pos 43-84) for even number of transitions
      }
      else
      {
        //
        ledPwmSet(pos, 4);
      }
    }

    // DEBUG - Print status neopixel mode
    if (DebugSerial >= 2)
    {
      Serial.print(" Smode=");
      Serial.print(status_neo_mode);
    }

    // DEBUG - Print Carriage Return for iteration level debug output
    if (DebugSerial >= 2)
    {
      Serial.println();
    }

    // Pause the loop to display everything
    delay(LEDDelayTime);

    // Turn OFF White LED at end of Iteration loop
    digitalWrite(LED_D2, LOW);

    // END OF FOR ITERATION LOOP
  }
  // //////////////////////////////////
  //     END OF ITERATION LOOP
  // //////////////////////////////////

  // Touch Loop Counters
  if (Touch01_IntCount >= 1)
  {
    Touch01_LoopCount++;
    Touch01_IntCount = 0;
  }
  else
  {
    Touch01_LoopCount = 0;
  }
  if (Touch03_IntCount >= 1)
  {
    Touch03_LoopCount++;
    Touch03_IntCount = 0;
  }
  else
  {
    Touch03_LoopCount = 0;
  }
  if (Touch05_IntCount >= 1)
  {
    Touch05_LoopCount++;
    Touch05_IntCount = 0;
  }
  else
  {
    Touch05_LoopCount = 0;
  }

  // Reset Touch Iteration Flags
  Touch01_IntFlag = 0;
  Touch03_IntFlag = 0;
  Touch05_IntFlag = 0;

  // Turn off all LEDs at end of loop (Optional for troubleshooting)
  // ledAllOff();

  // //////////////////////////////////////////////////
  //
  // Launch DEAUTH SNIFF Alternate Mainline Code When
  // Touch01_LoopCount exceeds Touch01_Loop_Threshold
  //
  // //////////////////////////////////////////////////
  if (Touch01_LoopCount > Touch01_Loop_Threshold)
  {
    Serial.println("LONG TOUCH DETECTED on TCH01 - JUMP TO ALTERNATE CODE");
    ledAllOff();
    title_neo_red();
    // Print Serial Welcome Once
    Serial.println("****************************************");
    Serial.println("****************************************");
    Serial.println("** WELCOME TO WIFI DEAUTH SNIFF MODE ***");
    Serial.println("****************************************");
    Serial.println("****************************************");
    Serial.println("*** ACTIVATED BY LONG TOUCH ON TCH01 ***");
    Serial.println("****************************************");
    Serial.println("****************************************");
    Serial.println("** LONG PRESS AGAIN TO EXIT THIS MODE **");
    Serial.println("****************************************");
    Serial.println("****************************************");
    // Pause before turning on proceeding
    delay(1000);
    //
    // DEAUTH SNIFF SETUP
    deauth_sniff_setup();
    // Pause for WiFi to come up
    delay(2000);
    //
    // DEAUTH SNIFF Alternative Main Loop
    while (deauth_sniff_active)
    {
      if (millis() - previousChannelHopTime >= CHANNEL_HOP_INTERVAL)
      {
        if (deauthCount >= deauthThreshold)
        {
          tft.fillScreen(0xF800); // Red background
          tft.setCursor(35, 60);
          tft.setTextColor(GC9A01A_BLACK);
          tft.setTextSize(2);
          tft.println("Detected!");

          tft.setCursor(35, 100);
          tft.setTextColor(GC9A01A_BLACK);
          tft.setTextSize(2);
          tft.print("Channel: ");
          tft.println(currentChannel);

          tft.setCursor(35, 140);
          tft.setTextColor(GC9A01A_BLACK);
          tft.setTextSize(2);
          tft.print("Deauths: ");
          tft.println(deauthCount > 9999 ? 9999 : deauthCount); // Limit to 4 digits

          delay(3500);
          tft.fillScreen(0x07E0); // Reset to green
        }
        Serial.println("Channel - " + String(currentChannel) + " :: Deauths - " + String(deauthCount)); // This will not be the exact number printed on the screen as RX CB is continous
        channelHop();
        previousChannelHopTime = millis();

        tft.fillScreen(0x07E0); // Green background
        tft.setCursor(35, 60);
        tft.setTextColor(GC9A01A_BLACK);
        tft.setTextSize(3);
        tft.println("Scanning...");

        tft.setCursor(35, 100);
        tft.setTextColor(GC9A01A_BLACK);
        tft.setTextSize(2);
        tft.print("Channel: ");
        tft.println(currentChannel);

        if (deauthCount < deauthThreshold)
        {
          tft.setCursor(35, 140);
          tft.setTextColor(GC9A01A_BLACK);
          tft.setTextSize(2);
          tft.print("Deauths: ");
          tft.println(deauthCount);
        }

        // Touch for exit mode settings
        Touch01_Value = touchRead(TCH01_PIN);
        if (Touch01_Value < Touch01_Threshold)
        {
          if (DebugSerial >= 2)
          {
            Serial.print("TCH01_TOUCHED=");
            Serial.print(Touch01_Value);
            Serial.print("/");
            Serial.println(Touch01_Threshold);
          }
          Touch01_LoopCount++;
        }
        else
        {
          Touch01_LoopCount = 0;
        }
        if (Touch01_LoopCount > 3)
        {
          deauth_sniff_active = false;
        }
      }
    }
    // END ALTERNATE MAIN LOOP
    Serial.println("****************************************");
    Serial.println("**** EXITING WIFI DEAUTH SNIFF MODE ****");
    Serial.println("****************************************");
    ledAllOff();
    // Turn Off WiFi/BT
    if (DebugSerial >= 2)
    {
      Serial.println("Turn Off WiFi / BlueTooth");
    }
    setModemSleep();
    Touch01_LoopCount = 0;
    // Pause before turning on proceeding
    delay(100);
  }
  // //////////////////////////////////////////////////
  //
  // Launch WOF Alternate Mainline Code When
  // Touch03_LoopCount exceeds Touch03_Loop_Threshold
  //
  // //////////////////////////////////////////////////
  if (Touch03_LoopCount > Touch03_Loop_Threshold)
  {
    Serial.println("LONG TOUCH DETECTED on TCH03 - JUMP TO WOF SCAN MODE");
    ledAllOff();
    title_neo_red();

    flipper_scan_setup();

    // Start WOF scanning loop
    while (flipper_scan_active)
    {
      Serial.println("Scanning for Flippers...");
      flipperCount = 0; // Reset count before each scan
      BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME, false);
      pBLEScan->clearResults(); // Flush results before next scan

      tftBlack();
      tft.setCursor(35, 60);
      tft.setTextColor(GC9A01A_WHITE);
      tft.setTextSize(3);
      tft.println("Flippers");

      tft.setCursor(35, 100);
      tft.setTextColor(GC9A01A_WHITE);
      tft.setTextSize(3);
      tft.println("Nearby");

      tft.setCursor(35, 140);
      tft.setTextColor(GC9A01A_WHITE);
      tft.setTextSize(3);
      tft.println(flipperCount > 9999 ? 9999 : flipperCount); // Limit to 4 digits

      // Delay before the next scan
      delay(5000); // 5 seconds delay

      // Check for exit mode touch
      Touch03_Value = touchRead(TCH03_PIN);
      if (Touch03_Value < Touch03_Threshold)
      {
        Touch03_LoopCount++;
      }
      else
      {
        Touch03_LoopCount = 0;
      }

      // Exit WOF scan mode if long touch detected
      if (Touch03_LoopCount > Touch03_Loop_Threshold)
      {
        Serial.println("LONG TOUCH DETECTED on TCH03 - EXITING WOF SCAN MODE");
        flipper_scan_active = false; // Exit the scanning loop

        Serial.println("****************************************");
        Serial.println("**** EXITING WALL OF FLIPPERS SCAN MODE ******");
        Serial.println("****************************************");
        ledAllOff();
        // Turn Off Bluetooth/Other
        if (DebugSerial >= 2)
        {
          Serial.println("Turn Off Bluetooth");
        }
        setModemSleep();
        Touch03_LoopCount = 0;
        // Pause before proceeding
        delay(100);
      }
    }
  }

  // //////////////////////////////////////////////////
  //
  // Launch BATT_CHRG_NOLED Alternate Mainline Code When
  // Touch05_LoopCount exceeds Touch05_Loop_Threshold
  //
  // //////////////////////////////////////////////////
  if (Touch05_LoopCount > Touch05_Loop_Threshold)
  {
    //
    Serial.println("LONG TOUCH DETECTED on TCH05 - JUMP TO ALTERNATE CODE");
    //
    ledAllOff();
    tftBlack();
    //
    Touch05_LoopCount = 0;
    //
    // Alternate code loop
    batt_chrg_noled();
    //
    // END ALTERNATE MAIN LOOP
    Serial.println("****************************************");
    Serial.println("***** EXITING BATT_CHRG_NOLED MODE *****");
    Serial.println("****************************************");
    //
    ledAllOff();
    tftBlack();
    //
    Touch05_LoopCount = 0;
    //
    // Pause before exiting
    delay(100);
  }

  // END OF MAIN LOOP
}

// //////////////////////////////////////////////////
//
// FUNCTIONS
//
// //////////////////////////////////////////////////
//
// WiFi & BT Functions
// //////////////////////////////////////////////////
void disableWiFi()
{
  WiFi.disconnect(true); // Disconnect from the network
  WiFi.mode(WIFI_OFF);   // Switch WiFi off
  Serial.println("WiFi disabled!");
}
//
void disableBluetooth()
{
  btStop();
  Serial.println("Bluetooth stopped!");
}
//
void setModemSleep()
{
  disableWiFi();
  disableBluetooth();
  setCpuFrequencyMhz(80);
}
//
void enableBluetooth()
{
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  Serial.println("Bluetooth enabled!");
}
//
void enableWiFi()
{
  delay(200);
  // Switch Wifi ON in mode AP/STA/AP_STA
  WiFi.mode(WIFI_AP); // Defaulting to AP mode
  delay(200);
  Serial.println("WiFi Started!");
}
//
void wakeModemSleep()
{
  setCpuFrequencyMhz(240);
  enableWiFi();
}
// //////////////////////////////////////////////////
//
// DEAUTH SNIFF Functions
// //////////////////////////////////////////////////
void deauth_sniff_setup()
{
  WiFi.begin();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(packetSniffer);
  tft.fillScreen(0x07E0);
  previousChannelHopTime = millis();
  deauth_sniff_active = true;
  Touch01_LoopCount = 0;
}
//
void channelHop()
{
  currentChannel = (currentChannel % MAX_CHANNELS) + 1;
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
  deauthCount = 0;
}
//
void packetSniffer(void *buf, wifi_promiscuous_pkt_type_t type)
{
  if (type == WIFI_PKT_MGMT)
  {
    wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t *)buf;
    const wifi_ieee80211_packet_t_2 *ipkt = (wifi_ieee80211_packet_t_2 *)packet->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

    switch (hdr->frame_ctrl & 0xFC) // Mask to get type and subtype only
    {
    case 0xB0: // Authentication frame

      break;
    case 0xC0: // Deauthentication frame
      deauthCount++;
      break;
    case 0x00: // Association request frame

      break;
    case 0x30: // Reassociation response frame

      break;
    case 0x40: // Probe request frame

      break;
    case 0x50: // Probe response frame

      break;
    case 0x80: // Beacon frame

      break;
    default:
      break;
    }
  }
}

// BLE Functions
void flipper_scan_setup()
{
  // Enable Bluetooth scanning setup
  enableBluetooth();

  Serial.println("****************************************");
  Serial.println("****************************************");
  Serial.println("** WELCOME TO WALL OF FLIPPERS SCAN MODE *****");
  Serial.println("****************************************");
  Serial.println("****************************************");
  Serial.println("*** ACTIVATED BY LONG TOUCH ON TCH03 ***");
  Serial.println("****************************************");
  Serial.println("****************************************");
  Serial.println("** LONG PRESS AGAIN TO EXIT THIS MODE **");
  Serial.println("****************************************");
  Serial.println("****************************************");

  tftBlack();
  tft.setCursor(35, 60);
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(3);
  tft.println("Flippers");

  tft.setCursor(35, 100);
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(3);
  tft.println("Nearby");

  // Pause before proceeding
  delay(1000);

  flipper_scan_active = true;
  Touch03_LoopCount = 0;
}
//
// //////////////////////////////////////////////////
//
// LED Functions
// //////////////////////////////////////////////////
void ledAllOff()
{
  digitalWrite(LED_D2, LOW);
  ledcWrite(LED_D3_pwm, 0);
  ledcWrite(LED_D4_pwm, 0);
  ledcWrite(LED_D5_pwm, 0);
  ledcWrite(LED_D6_pwm, 0);
  ledcWrite(LED_D7_pwm, 0);
  NEO01.setPixelColor(0, 0, 0, 0);
  NEO01.setPixelColor(1, 0, 0, 0);
  NEO01.show();
  NEO02.setPixelColor(0, 0, 0, 0);
  NEO02.show();
}
//
void ledPwmSet(uint8_t pos, uint8_t pass)
{
  // Pass 1 pos 0-84
  // D3/6 ON->OFF
  // D4/5/7 OFF->ON
  if (pass == 1)
  {
    ledcWrite(LED_D3_pwm, int(255 - (pos * 3)));
    ledcWrite(LED_D6_pwm, int(255 - (pos * 3)));
    ledcWrite(LED_D4_pwm, int(pos * 3));
    ledcWrite(LED_D5_pwm, int(pos * 3));
    ledcWrite(LED_D7_pwm, int(pos * 3));
  }
  // Pass 2 pos 0-84
  // D3/6 OFF->ON
  // D4/5/7 ON->OFF
  if (pass == 2)
  {
    ledcWrite(LED_D3_pwm, int(pos * 3));
    ledcWrite(LED_D6_pwm, int(pos * 3));
    ledcWrite(LED_D4_pwm, int(255 - (pos * 3)));
    ledcWrite(LED_D5_pwm, int(255 - (pos * 3)));
    ledcWrite(LED_D7_pwm, int(255 - (pos * 3)));
  }
  // Pass 3 pos 0-42
  // D3/6 ON->OFF
  // D4/5/7 OFF->ON
  if (pass == 3)
  {
    ledcWrite(LED_D3_pwm, int(255 - ((pos - 43) * 6)));
    ledcWrite(LED_D6_pwm, int(255 - ((pos - 43) * 6)));
    ledcWrite(LED_D4_pwm, int((pos - 43) * 6));
    ledcWrite(LED_D5_pwm, int((pos - 43) * 6));
    ledcWrite(LED_D7_pwm, int((pos - 43) * 6));
  }
  // Pass 4 pos 43-84
  // D3/6 OFF->ON
  // D4/5/7 ON->OFF
  if (pass == 4)
  {
    ledcWrite(LED_D3_pwm, int((pos - 43) * 6));
    ledcWrite(LED_D6_pwm, int((pos - 43) * 6));
    ledcWrite(LED_D4_pwm, int(255 - ((pos - 43) * 6)));
    ledcWrite(LED_D5_pwm, int(255 - ((pos - 43) * 6)));
    ledcWrite(LED_D7_pwm, int(255 - ((pos - 43) * 6)));
  }
}
//
void title_neo_colorshift(uint8_t pos, uint8_t pass)
{
  // If Touch Area TCH01 or TCH03 is pressed dont do anything here
  if (Touch01_IntCount || Touch03_IntCount > 0)
  {
    pass = 0;
  }
  // Pass 1 pos 0-84
  if (pass == 1)
  {
    // Red 255-0 Green 0-255
    NEO01.setPixelColor(0, int(255 - (pos * 3)), int(pos * 3), 0);
    // Blue 255-0 Red 0-255
    NEO01.setPixelColor(1, int(pos * 3), 0, int(255 - pos * 3));
  }
  // Pass 2 pos 0-84
  if (pass == 2)
  {
    // Green 255-0 Blue 0-255
    NEO01.setPixelColor(0, 0, int(255 - (pos * 3)), int(pos * 3));
    // Red 255-0 Green 0-255
    NEO01.setPixelColor(1, int(255 - (pos * 3)), int(pos * 3), 0);
  }
  // Pass 3 pos 0-84
  if (pass == 3)
  {
    // Blue 255-0 Red 0-255
    NEO01.setPixelColor(0, int(pos * 3), 0, int(255 - pos * 3));
    // Green 255-0 Blue 0-255
    NEO01.setPixelColor(1, 0, int(255 - (pos * 3)), int(pos * 3));
  }
  // Show color setting on Neoixel
  NEO01.show();
}
//
void title_neo_red()
{
  NEO01.setPixelColor(0, 255, 0, 0);
  NEO01.setPixelColor(1, 255, 0, 0);
  NEO01.show();
}
//
void title_neo_green()
{
  NEO01.setPixelColor(0, 0, 255, 0);
  NEO01.setPixelColor(1, 0, 255, 0);
  NEO01.show();
}
//
void title_neo_blue()
{
  NEO01.setPixelColor(0, 0, 0, 255);
  NEO01.setPixelColor(1, 0, 0, 255);
  NEO01.show();
}
//
void status_neo_colorset(uint8_t mode)
{
  // Status NeoPixel LED color mode
  // bitwise off=0 red=1 green=2 blue=4
  //
  // 0 -> 1 Off -> Red
  // 1 -> 3 Red -> Red+Green
  // 3 -> 2 Red+Green -> Green
  // 2 -> 6 Green -> Green+Blue
  // 6 -> 4 Green+Blue -> Blue
  // 4 -> 5 Blue -> Blue+Red
  // 5 -> 7 Blue+Red -> Blue+Red+Green
  // 7 -> 0 Blue+Red+Green -> Off
  //
  if (mode == 0)
  {
    NEO02.setPixelColor(0, 255, 0, 0);
    status_neo_mode = 1;
  }
  if (mode == 1)
  {
    NEO02.setPixelColor(0, 255, 255, 0);
    status_neo_mode = 3;
  }
  if (mode == 2)
  {
    NEO02.setPixelColor(0, 0, 255, 255);
    status_neo_mode = 6;
  }
  if (mode == 3)
  {
    NEO02.setPixelColor(0, 0, 255, 0);
    status_neo_mode = 2;
  }
  if (mode == 4)
  {
    NEO02.setPixelColor(0, 255, 0, 255);
    status_neo_mode = 5;
  }
  if (mode == 5)
  {
    NEO02.setPixelColor(0, 255, 255, 255);
    status_neo_mode = 7;
  }
  if (mode == 6)
  {
    NEO02.setPixelColor(0, 0, 0, 255);
    status_neo_mode = 4;
  }
  if (mode == 7)
  {
    NEO02.setPixelColor(0, 0, 0, 0);
    status_neo_mode = 0;
  }
  // Show color setting on Neoixel
  NEO02.show();
}
//
void BI_on()
{
  digitalWrite(LED_BI, LOW); // LOW = ON?
}
//
void BI_off()
{
  digitalWrite(LED_BI, HIGH); // HIGH = OFF?
}
//
void BI_blink_one(uint8_t pos)
{
  if (pos <= 21)
  {
    digitalWrite(LED_BI, LOW); // LOW = ON?
  }
  else
  {
    digitalWrite(LED_BI, HIGH); // HIGH = OFF?
  }
}
//
void BI_blink_two(uint8_t pos)
{
  if (pos <= 11)
  {
    digitalWrite(LED_BI, LOW); // LOW = ON?
  }
  else if (pos > 11 and pos <= 22)
  {
    digitalWrite(LED_BI, HIGH); // HIGH = OFF?
  }
  else if (pos > 22 and pos <= 33)
  {
    digitalWrite(LED_BI, LOW); // LOW = ON?
  }
  else
  {
    digitalWrite(LED_BI, HIGH); // HIGH = OFF?
  }
}
//
void BI_blink_three(uint8_t pos)
{
  if (pos <= 5)
  {
    digitalWrite(LED_BI, LOW); // LOW = ON?
  }
  else if (pos > 5 and pos <= 10)
  {
    digitalWrite(LED_BI, HIGH); // HIGH = OFF?
  }
  else if (pos > 10 and pos <= 15)
  {
    digitalWrite(LED_BI, LOW); // LOW = ON?
  }
  else if (pos > 15 and pos <= 20)
  {
    digitalWrite(LED_BI, HIGH); // HIGH = OFF?
  }
  else if (pos > 20 and pos <= 25)
  {
    digitalWrite(LED_BI, LOW); // LOW = ON?
  }
  else
  {
    digitalWrite(LED_BI, HIGH); // HIGH = OFF?
  }
}
//
void batt_chrg_noled()
{
  // Set an exit var
  bool batt_chrg_noled_active = true;
  //
  while (batt_chrg_noled_active)
  {
    BI_off();
    // Print Serial Message About Mode
    Serial.println("****************************************");
    Serial.println("****************************************");
    Serial.println("********* BATT_CHRG_NOLED MODE *********");
    Serial.println("****************************************");
    Serial.println("*** ACTIVATED BY LONG TOUCH ON TCH01 ***");
    Serial.println("***      THE MONARCH LOGO BUTTON     ***");
    Serial.println("****************************************");
    Serial.println("** LONG PRESS AGAIN TO EXIT THIS MODE **");
    Serial.println("****************************************");
    Serial.println("****************************************");
    // Pause
    delay(3500);
    // Turn on-board LED on briefly to show badge is still on
    BI_on();
    // Pause
    delay(500);
    //
    // Touch for exit mode settings
    //
    Touch05_Value = touchRead(TCH05_PIN);
    // Do Stuff If We Detect a Touch on TCH05_PIN
    if (Touch05_Value < Touch05_Threshold)
    {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2)
      {
        Serial.print("T5_TCH=");
        Serial.print(Touch05_Value);
        Serial.print("/");
        Serial.print(Touch05_Threshold);
        Serial.print("-");
        Serial.println(Touch05_LoopCount);
      }
      // STUFF - TCH05_PIN TOUCHED
      Touch05_LoopCount++;
      //
      // Do Stuff If We DONT Detect a Touch on TCH05_PIN
    }
    else
    {
      // STUFF - TCH05_PIN NOT TOUCHED
      Touch05_LoopCount = 0;
    }
    if (Touch05_LoopCount > Touch05_Loop_Threshold)
    {
      batt_chrg_noled_active = false;
    }
  }
}
// //////////////////////////////////////////////////
//
// DISPLAY Functions
// //////////////////////////////////////////////////
void tftBlack()
{
  // Set the tft screen to black
  tft.fillScreen(GC9A01A_BLACK);
}
unsigned long cameraShutter1()
{
  unsigned long start, t = 0;
  int cx = tft.width() / 2 - 1;
  int cy = tft.height() / 2 - 1;
  tft.fillScreen(GC9A01A_BLACK);
  start = micros();

  uint16_t startColor = tft.color565(255, 0, 0);
  uint16_t endColor = tft.color565(0, 0, 255);

  int colorSteps = min(cx, cy) / 5;

  for (int i = min(cx, cy); i > 10; i -= 5)
  {
    start = micros();

    uint8_t red = map(i, 10, min(cx, cy), 255, 0);
    uint8_t green = 0;
    uint8_t blue = map(i, 10, min(cx, cy), 0, 255);

    uint16_t color = tft.color565(red, green, blue);

    for (int angle = 0; angle < 360; angle += 45)
    {
      float rad = radians(angle);
      float radOffset = radians(15);

      int x1 = cx + i * cos(rad);
      int y1 = cy + i * sin(rad);
      int x2 = cx + i * cos(rad + radOffset);
      int y2 = cy + i * sin(rad + radOffset);
      int x3 = cx + i * cos(rad + 2 * radOffset);
      int y3 = cy + i * sin(rad + 2 * radOffset);

      tft.fillTriangle(cx, cy, x1, y1, x2, y2, color);
      tft.drawTriangle(cx, cy, x1, y1, x2, y2, tft.color565(0, 255, 255));
    }

    t += micros() - start;
    yield();
  }

  tft.fillTriangle(cx, cy - 10, cx - 10, cy + 10, cx + 10, cy + 10, GC9A01A_BLACK);

  return t;
}

unsigned long cameraShutter2()
{
  unsigned long start, t = 0;
  int cx = tft.width() / 2 - 1;
  int cy = tft.height() / 2 - 1;
  tft.fillScreen(GC9A01A_BLACK);
  start = micros();

  uint16_t startColor = tft.color565(255, 0, 0);
  uint16_t endColor = tft.color565(0, 0, 255);

  int colorSteps = min(cx, cy) / 5;

  for (int i = min(cx, cy); i > 10; i -= 5)
  {
    start = micros();

    uint8_t red = map(i, 10, min(cx, cy), 255, 0);
    uint8_t green = 0;
    uint8_t blue = map(i, 10, min(cx, cy), 0, 255);

    uint16_t color = tft.color565(red, green, blue);

    for (int angle = 0; angle < 360; angle += 45)
    {
      float rad = radians(angle + i);
      float radOffset = radians(15);

      int x1 = cx + i * cos(rad);
      int y1 = cy + i * sin(rad);
      int x2 = cx + i * cos(rad + radOffset);
      int y2 = cy + i * sin(rad + radOffset);
      int x3 = cx + i * cos(rad + 2 * radOffset);
      int y3 = cy + i * sin(rad + 2 * radOffset);

      tft.fillTriangle(cx, cy, x1, y1, x2, y2, color);
      tft.drawTriangle(cx, cy, x1, y1, x2, y2, tft.color565(0, 255, 255));
    }

    t += micros() - start;
    yield();
  }

  tft.fillTriangle(cx, cy - 10, cx - 10, cy + 10, cx + 10, cy + 10, GC9A01A_BLACK);

  return t;
}
