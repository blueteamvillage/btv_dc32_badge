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
const char* wl_status_to_string(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return "WL_NO_SHIELD";
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
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
typedef struct {
  int16_t fctl;
  int16_t duration;
  uint8_t da;
  uint8_t sa;
  uint8_t bssid;
  int16_t seqctl;
  unsigned char payload[];
} WifiMgmtHdr;
typedef struct {
  unsigned frame_ctrl : 16;
  unsigned duration_id : 16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl : 16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;
typedef struct {
  uint8_t payload[0];
  wifi_ieee80211_mac_hdr_t hdr;
} wifi_ieee80211_packet_t_2;
typedef struct {
  uint8_t payload[0];
  WifiMgmtHdr hdr;
} wifi_ieee80211_packet_t;
bool deauth_sniff_active = false;

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
// Store a random number
int MyRandom = 0;

// //////////////////////////////////////////////////
//
// SETUP - RUN ONCE
//
// //////////////////////////////////////////////////
void setup(){
  // Add a delay to allow opening serial monitor
  delay(800);

  // setup the serial output baud rate
  Serial.begin(115200);

  if (DebugSerial >= 1) {
    Serial.println("Starting Setup");
  }

  // Turn Off WiFi/BT
  if (DebugSerial >= 2) {
    Serial.println("Turn Off WiFi / BlueTooth");
  }
  setModemSleep();

  // Configure LED PWM functionalitites per channel
  if (DebugSerial >= 2) {
    Serial.println("Configure PWM Channels");
  }
  ledcSetup(LED_D3_pwm, freq, resolution);
  ledcSetup(LED_D4_pwm, freq, resolution);
  ledcSetup(LED_D5_pwm, freq, resolution);
  ledcSetup(LED_D6_pwm, freq, resolution);
  ledcSetup(LED_D7_pwm, freq, resolution);

  // Attach the channel to the GPIO to be controlled
  if (DebugSerial >= 2) {
    Serial.println("Attach PWM Channels to LED Pins");
  }
  ledcAttachPin(LED_D3, LED_D3_pwm);
  ledcAttachPin(LED_D4, LED_D4_pwm);
  ledcAttachPin(LED_D5, LED_D5_pwm);
  ledcAttachPin(LED_D6, LED_D6_pwm);
  ledcAttachPin(LED_D7, LED_D7_pwm);

  //Normal LED output
  if (DebugSerial >= 2) {
    Serial.println("Set Output for non-PWM LED Pins");
  }
  pinMode(LED_D2, OUTPUT);

  // Initialize the NeoPixels
  if (DebugSerial >= 2) {
    Serial.println("Initialize NeoPixels");
  }
  NEO01.begin();
  NEO02.begin(); 
  // Set Neopixel Brightness (0-255 scale)
  NEO01.setBrightness(170);
  NEO02.setBrightness(170);

  // Initialize the Display
  if (DebugSerial >= 2) {
    Serial.println("Initialize Display");
  }
  tft.begin();

  // Start all LEDs in OFF mode
  if (DebugSerial >= 2) {
    Serial.println("Turn OFF all LEDs");
  }
  ledAllOff();

  //
  // Random Seed based on Touch03_Value
  randomSeed(touchRead(TCH03_PIN));

  if (DebugSerial >= 1) {
    Serial.println(F("Setup Done!"));
  }

  // END OF SETUP
}

// //////////////////////////////////////////////////
//
// LOOP - MAIN 
//
// //////////////////////////////////////////////////
void loop(){
  if (DebugSerial >= 1) {
    Serial.println("******************** TOP OF MAIN LOOP ********************");
  }
  // Capacitive Touch Dynamic Threshold Adjustment
  // Adjust thresholds UP to account for assembly conditions and battery vs usb
  Touch01_Value = touchRead(TCH01_PIN);
  if ( (Touch01_Value / Touch01_Threshold) > 2 ) { Touch01_Threshold = int(Touch01_Threshold * 1.8); }
  Touch03_Value = touchRead(TCH03_PIN);
  if ( (Touch03_Value / Touch03_Threshold) > 2 ) { Touch03_Threshold = int(Touch03_Threshold * 1.8); }
  Touch05_Value = touchRead(TCH05_PIN);
  if ( (Touch05_Value / Touch05_Threshold) > 2 ) { Touch05_Threshold = int(Touch05_Threshold * 1.8); }

  // Iterate 0 to 254
  for(int i=0; i<255; i++){
    // Set position value to iteration value
    int pos = i;

    //
    // First of three position groups i 0-84
    if (pos < 85) {
      //
      // RUN A RANDOM SCREEN UPDATE ONCE PER GROUP
      if (pos == 0) {
        MyRandom = random(1, 5);
        if (MyRandom == 1){ testFilledCircles(10, GC9A01A_BLUE); }
        if (MyRandom == 2){ testFilledRects(GC9A01A_YELLOW, GC9A01A_MAGENTA); }
        if (MyRandom == 3){ testTriangles(); }
        if (MyRandom >= 4){ testTCHvalueText(); }
      }
      //
      // LED FUNCTIONS
      title_neo_colorshift(pos, 1);
      ledPwmSet(pos, 1);
    // Second of three position groups i 85-169 (pos-85 = 0-84)
    } else if (pos < 170) {
      pos = pos - 85;
      //
      // RUN A RANDOM SCREEN UPDATE ONCE PER GROUP
      if (pos == 0) {
        MyRandom = random(1, 5);
        if (MyRandom == 1){ testFastLines(GC9A01A_RED, GC9A01A_BLUE); }
        if (MyRandom == 2){ testRects(GC9A01A_GREEN); }
        if (MyRandom == 3){ testTCHvalueText(); }
        if (MyRandom >= 4){ testFilledTriangles(); }
      }
      //
      // LED FUNCTIONS
      title_neo_colorshift(pos, 2);
      ledPwmSet(pos, 2);
    // Third of three position groups i 170-254 (pos-170 = 0-84)
    } else {
      pos = pos -170;
      //
      // RUN A RANDOM SCREEN UPDATE ONCE PER GROUP
      if (pos == 0) {
        MyRandom = random(1, 5);
        if (MyRandom == 1){ testRoundRects(); }
        if (MyRandom == 2){ testTCHvalueText(); }
        if (MyRandom == 3){ testCircles(10, GC9A01A_RED); }
        if (MyRandom >= 4){ testFilledRoundRects(); }
      }
      //
      // LED FUNCTIONS
      title_neo_colorshift(pos, 3);
      // Split third group 3/4 (pos 0-42) for even number of transitions
      if (pos <43) {
        //
        ledPwmSet(pos, 3);
      // Split third group 4/4 (pos 43-84) for even number of transitions
      } else {
        //
        ledPwmSet(pos, 4);
      }
    }

    // DEBUG - Print current Iteration value to serial console for troubleshooting
    if (DebugSerial >= 2) {
      Serial.print(" Iteration="); Serial.print(i);
      Serial.print(" Pos="); Serial.print(pos);
      Serial.print(" MyRandom="); Serial.print(MyRandom);
    }

    //
    // TOUCH
    //
    // Read Touch Values
    Touch01_Value = touchRead(TCH01_PIN);
    Touch03_Value = touchRead(TCH03_PIN);
    Touch05_Value = touchRead(TCH05_PIN);
    //
    // Do Stuff If We Detect a Touch on TCH01_PIN
    if (Touch01_Value < Touch01_Threshold) {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2) {
        Serial.print(" T01_TOUCH="); Serial.print(Touch01_Value);
        Serial.print("/"); Serial.print(Touch01_Threshold);
        Serial.print("-"); Serial.print(Touch01_IntCount);
        Serial.print("/"); Serial.print(Touch01_LoopCount);
      }
      // STUFF - TCH01_PIN TOUCHED
      if (Touch01_IntCount == 0){
        // Put stuff to happen once per iteration loop here
      }
      // Put stuff to happen every iteration here
      Touch01_IntCount = 1;
    //
    // Do Stuff If We DONT Detect a Touch on TCH01_PIN
    } else {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2) {
        Serial.print(" T01="); Serial.print(Touch01_Value);
        Serial.print("/"); Serial.print(Touch01_Threshold);
        Serial.print("-"); Serial.print(Touch01_IntCount);
        Serial.print("/"); Serial.print(Touch01_LoopCount);
      }
      // STUFF - TCH01_PIN NOT TOUCHED
    }
    //
    // Do Stuff If We Detect a Touch on TCH03_PIN
    if (Touch03_Value < Touch03_Threshold) {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2) {
        Serial.print(" T03_TOUCH="); Serial.print(Touch03_Value);
        Serial.print("/"); Serial.print(Touch03_Threshold);
        Serial.print("-"); Serial.print(Touch03_IntCount);
        Serial.print("/"); Serial.print(Touch03_LoopCount);
      }
      // STUFF - TCH03_PIN TOUCHED
      if (Touch03_IntCount == 0){
        // Put stuff to happen once per iteration loop here
        status_neo_colorset(status_neo_mode);
      }
      // Put stuff to happen every iteration here
      Touch03_IntCount = 1;
    //
    // Do Stuff If We DONT Detect a Touch on TCH03_PIN
    } else {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2) {
        Serial.print(" T03="); Serial.print(Touch03_Value);
        Serial.print("/"); Serial.print(Touch03_Threshold);
        Serial.print("-"); Serial.print(Touch03_IntCount);
        Serial.print("/"); Serial.print(Touch03_LoopCount);
      }
      // STUFF - TCH03_PIN NOT TOUCHED
    }
    //
    // Do Stuff If We Detect a Touch on TCH05_PIN
    if (Touch05_Value < Touch05_Threshold) {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2) {
        Serial.print(" T05_TOUCH="); Serial.print(Touch05_Value);
        Serial.print("/"); Serial.print(Touch05_Threshold);
        Serial.print("-"); Serial.print(Touch05_IntCount);
        Serial.print("/"); Serial.print(Touch05_LoopCount);
      }
      // STUFF - TCH05_PIN TOUCHED
      if (Touch05_IntCount == 0){
        // Put stuff to happen once per iteration loop here
      }
      // Put stuff to happen every iteration here
      Touch05_IntCount = 1;
      digitalWrite(LED_D2, HIGH);
    //
    // Do Stuff If We DONT Detect a Touch on TCH05_PIN
    } else {
      // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
      if (DebugSerial >= 2) {
        Serial.print(" T05="); Serial.print(Touch05_Value);
        Serial.print("/"); Serial.print(Touch05_Threshold);
        Serial.print("-"); Serial.print(Touch05_IntCount);
        Serial.print("/"); Serial.print(Touch05_LoopCount);
      }
      // STUFF - TCH05_PIN NOT TOUCHED
      digitalWrite(LED_D2, LOW);
    }

    // DEBUG - Print status neopixel mode
    if (DebugSerial >= 2) {
      Serial.print(" Smode="); Serial.print(status_neo_mode);
    }

    // DEBUG - Print Carriage Return for iteration level debug output
    if (DebugSerial >= 2) {
      Serial.println();
    }

    // Pause the loop to display everything
    delay(LEDDelayTime);

    // Turn OFF White LED at end of Iteration loop
    digitalWrite(LED_D2, LOW);

    // END OF FOR ITERATION LOOP
  }

  // Touch Loop Counters - USE TBD
  if (Touch01_IntCount == 1) { Touch01_LoopCount++; Touch01_IntCount = 0; } else { Touch01_LoopCount = 0; }
  if (Touch03_IntCount == 1) { Touch03_LoopCount++; Touch03_IntCount = 0; } else { Touch03_LoopCount = 0; }
  if (Touch05_IntCount == 1) { Touch05_LoopCount++; Touch05_IntCount = 0; } else { Touch05_LoopCount = 0; }

  // Turn off all LEDs at end of loop (Optional)
  // ledAllOff();

  // //////////////////////////////////////////////////
  //
  // Launch DEAUTH SNIFF Alternate Mainline Code When
  // Touch01_LoopCount exceeds Touch01_Loop_Threshold
  //
  // //////////////////////////////////////////////////
  if (Touch01_LoopCount > Touch01_Loop_Threshold) {
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
    while (deauth_sniff_active) {
      //
      if (millis() - previousChannelHopTime >= CHANNEL_HOP_INTERVAL) {
        if (deauthCount >= deauthThreshold)
        {
          Serial.println("Too many Deauths! (>=20)");
          tft.fillScreen(0xF800); // go red
          delay(3500);
          tft.fillScreen(0x07E0); // reset to green
        }
        channelHop();
        previousChannelHopTime = millis();
        //
        // Touch for exit mode settings
        //
        Touch01_Value = touchRead(TCH01_PIN);
        // Do Stuff If We Detect a Touch on TCH01_PIN
        if (Touch01_Value < Touch01_Threshold) {
          // DEBUG - Print current Touch value/threshold to serial console for troubleshooting
          if (DebugSerial >= 2) {
            Serial.print("TCH01_TOUCHED="); Serial.print(Touch01_Value);
            Serial.print("/"); Serial.println(Touch01_Threshold);
          }
          // STUFF - TCH01_PIN TOUCHED
          Touch01_LoopCount++;
        //
        // Do Stuff If We DONT Detect a Touch on TCH01_PIN
        } else {
          // STUFF - TCH01_PIN NOT TOUCHED
          Touch01_LoopCount = 0;
        }
        if (Touch01_LoopCount > 3) {
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
    if (DebugSerial >= 2) {
      Serial.println("Turn Off WiFi / BlueTooth");
    }
    setModemSleep();
    Touch01_LoopCount = 0;
    // Pause before turning on proceeding
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
void disableWiFi(){
    WiFi.disconnect(true);  // Disconnect from the network
    WiFi.mode(WIFI_OFF);    // Switch WiFi off
    Serial.println("WiFi disabled!");
}
//
void disableBluetooth(){
    btStop();
    Serial.println("Bluetooth stopped!");
}
//
void setModemSleep() {
    disableWiFi();
    disableBluetooth();
    setCpuFrequencyMhz(80);
}
//
void enableWiFi(){
    delay(200);
    // Switch Wifi ON in mode AP/STA/AP_STA
    WiFi.mode(WIFI_AP); // Defaulting to AP mode
    delay(200);
    Serial.println("WiFi Started!");
}
//
void wakeModemSleep() {
    setCpuFrequencyMhz(240);
    enableWiFi();
}
// //////////////////////////////////////////////////
//
// DEAUTH SNIFF Functions
// //////////////////////////////////////////////////
void deauth_sniff_setup(){
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
  Serial.print("Channel: " + String(currentChannel));
  Serial.println(" Deauth: " + String(deauthCount));
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
// //////////////////////////////////////////////////
//
// LED Functions
// //////////////////////////////////////////////////
void ledAllOff() {
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
void ledPwmSet(uint8_t pos, uint8_t pass) {
  // Pass 1 pos 0-84
  // D3/6 ON->OFF
  // D4/5/7 OFF->ON
  if (pass == 1){
    ledcWrite(LED_D3_pwm, int(255 - (pos*3)));
    ledcWrite(LED_D6_pwm, int(255 - (pos*3)));
    ledcWrite(LED_D4_pwm, int(pos*3));
    ledcWrite(LED_D5_pwm, int(pos*3));
    ledcWrite(LED_D7_pwm, int(pos*3));
  }
  // Pass 2 pos 0-84
  // D3/6 OFF->ON
  // D4/5/7 ON->OFF
  if (pass == 2){
    ledcWrite(LED_D3_pwm, int(pos*3));
    ledcWrite(LED_D6_pwm, int(pos*3));
    ledcWrite(LED_D4_pwm, int(255 - (pos*3)));
    ledcWrite(LED_D5_pwm, int(255 - (pos*3)));
    ledcWrite(LED_D7_pwm, int(255 - (pos*3)));
  }
  // Pass 3 pos 0-42
  // D3/6 ON->OFF
  // D4/5/7 OFF->ON
  if (pass == 3){
    ledcWrite(LED_D3_pwm, int(255 - ((pos-43)*6)));
    ledcWrite(LED_D6_pwm, int(255 - ((pos-43)*6)));
    ledcWrite(LED_D4_pwm, int((pos-43)*6));
    ledcWrite(LED_D5_pwm, int((pos-43)*6));
    ledcWrite(LED_D7_pwm, int((pos-43)*6));
  }
  // Pass 4 pos 43-84
  // D3/6 OFF->ON
  // D4/5/7 ON->OFF
  if (pass == 4){
    ledcWrite(LED_D3_pwm, int((pos-43)*6));
    ledcWrite(LED_D6_pwm, int((pos-43)*6));
    ledcWrite(LED_D4_pwm, int(255 - ((pos-43)*6)));
    ledcWrite(LED_D5_pwm, int(255 - ((pos-43)*6)));
    ledcWrite(LED_D7_pwm, int(255 - ((pos-43)*6)));
  }
}
//
void title_neo_colorshift(uint8_t pos, uint8_t pass) {
  // Pass 1 pos 0-84
  if (pass == 1){
    // Red 255-0 Green 0-255
    NEO01.setPixelColor(0, int(255 - (pos*3)), int(pos*3), 0);
    // Blue 255-0 Red 0-255
    NEO01.setPixelColor(1, int(pos*3), 0, int(255 - pos*3));
  }
  // Pass 2 pos 0-84
  if (pass == 2){
    // Green 255-0 Blue 0-255
    NEO01.setPixelColor(0, 0, int(255 - (pos*3)), int(pos*3));
    // Red 255-0 Green 0-255
    NEO01.setPixelColor(1, int(255 - (pos*3)), int(pos*3), 0);
  }
  // Pass 3 pos 0-84
  if (pass == 3){
    // Blue 255-0 Red 0-255
    NEO01.setPixelColor(0, int(pos*3), 0, int(255 - pos*3));
    // Green 255-0 Blue 0-255
    NEO01.setPixelColor(1, 0, int(255 - (pos*3)), int(pos*3));
  }
  // Show color setting on Neoixel
  NEO01.show();
}
//
void title_neo_red() {
  NEO01.setPixelColor(0, 255, 0, 0);
  NEO01.setPixelColor(1, 255, 0, 0);
  NEO01.show();
}
//
void title_neo_green() {
  NEO01.setPixelColor(0, 0, 255, 0);
  NEO01.setPixelColor(1, 0, 255, 0);
  NEO01.show();
}
//
void title_neo_blue() {
  NEO01.setPixelColor(0, 0, 0, 255);
  NEO01.setPixelColor(1, 0, 0, 255);
  NEO01.show();
}
//
void status_neo_colorset(uint8_t mode) {
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
  if (mode == 0){ NEO02.setPixelColor(0, 255, 0, 0); status_neo_mode = 1; }
  if (mode == 1){ NEO02.setPixelColor(0, 255, 255, 0); status_neo_mode = 3; }
  if (mode == 2){ NEO02.setPixelColor(0, 0, 255, 255); status_neo_mode = 6; }
  if (mode == 3){ NEO02.setPixelColor(0, 0, 255, 0); status_neo_mode = 2; }
  if (mode == 4){ NEO02.setPixelColor(0, 255, 0, 255); status_neo_mode = 5; }
  if (mode == 5){ NEO02.setPixelColor(0, 255, 255, 255); status_neo_mode = 7; }
  if (mode == 6){ NEO02.setPixelColor(0, 0, 0, 255); status_neo_mode = 4; }
  if (mode == 7){ NEO02.setPixelColor(0, 0, 0, 0); status_neo_mode = 0; }
  // Show color setting on Neoixel
  NEO02.show();
}
// //////////////////////////////////////////////////
//
// DISPLAY Functions
// //////////////////////////////////////////////////
unsigned long testTCHvalueText() {
  tft.fillScreen(GC9A01A_BLACK);
  unsigned long start = micros();
  tft.setCursor(35, 60);
  tft.setTextColor(GC9A01A_WHITE);  tft.setTextSize(3);
  tft.println("TCH Values");
  tft.setTextColor(GC9A01A_BLUE); tft.setTextSize(2);
  tft.print("  TCH01 "); tft.setTextColor(GC9A01A_WHITE); tft.print(Touch01_Value);
  tft.setTextColor(GC9A01A_BLUE); tft.print("/"); tft.println(Touch01_Threshold);
  tft.print("  TCH03 "); tft.setTextColor(GC9A01A_WHITE); tft.print(Touch03_Value);
  tft.setTextColor(GC9A01A_BLUE); tft.print("/"); tft.println(Touch03_Threshold);
  tft.print("  TCH05 "); tft.setTextColor(GC9A01A_WHITE); tft.print(Touch05_Value);
  tft.setTextColor(GC9A01A_BLUE); tft.print("/"); tft.println(Touch05_Threshold);
  return micros() - start;
}
//
unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(GC9A01A_BLACK);
  yield();
  tft.fillScreen(GC9A01A_RED);
  yield();
  tft.fillScreen(GC9A01A_GREEN);
  yield();
  tft.fillScreen(GC9A01A_BLUE);
  yield();
  tft.fillScreen(GC9A01A_BLACK);
  yield();
  return micros() - start;
}
//
unsigned long testText() {
  tft.fillScreen(GC9A01A_BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(GC9A01A_WHITE);  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(GC9A01A_YELLOW); tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextColor(GC9A01A_RED);    tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.println();
  tft.setTextColor(GC9A01A_GREEN);
  tft.setTextSize(5);
  tft.println("Groop");
  tft.setTextSize(2);
  tft.println("I implore thee,");
  tft.setTextSize(1);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  tft.println("Or I will rend thee");
  tft.println("in the gobberwarts");
  tft.println("with my blurglecruncheon,");
  tft.println("see if I don't!");
  return micros() - start;
}
//
unsigned long testLines(uint16_t color) {
  unsigned long start, t;
  int           x1, y1, x2, y2,
                w = tft.width(),
                h = tft.height();
  tft.fillScreen(GC9A01A_BLACK);
  yield();
  x1 = y1 = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t     = micros() - start; // fillScreen doesn't count against timing
  yield();
  tft.fillScreen(GC9A01A_BLACK);
  yield();
  x1    = w - 1;
  y1    = 0;
  y2    = h - 1;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;
  yield();
  tft.fillScreen(GC9A01A_BLACK);
  yield();
  x1    = 0;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = w - 1;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  t    += micros() - start;
  yield();
  tft.fillScreen(GC9A01A_BLACK);
  yield();
  x1    = w - 1;
  y1    = h - 1;
  y2    = 0;
  start = micros();
  for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
  x2    = 0;
  for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
  yield();
  return micros() - start;
}
//
unsigned long testFastLines(uint16_t color1, uint16_t color2) {
  unsigned long start;
  int           x, y, w = tft.width(), h = tft.height();
  tft.fillScreen(GC9A01A_BLACK);
  start = micros();
  for(y=0; y<h; y+=5) tft.drawFastHLine(0, y, w, color1);
  for(x=0; x<w; x+=5) tft.drawFastVLine(x, 0, h, color2);
  return micros() - start;
}
//
unsigned long testRects(uint16_t color) {
  unsigned long start;
  int           n, i, i2,
                cx = tft.width()  / 2,
                cy = tft.height() / 2;
  tft.fillScreen(GC9A01A_BLACK);
  n     = min(tft.width(), tft.height());
  start = micros();
  for(i=2; i<n; i+=6) {
    i2 = i / 2;
    tft.drawRect(cx-i2, cy-i2, i, i, color);
  }
  return micros() - start;
}
//
unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int           n, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;
  tft.fillScreen(GC9A01A_BLACK);
  n = min(tft.width(), tft.height());
  for(i=n; i>0; i-=6) {
    i2    = i / 2;
    start = micros();
    tft.fillRect(cx-i2, cy-i2, i, i, color1);
    t    += micros() - start;
    // Outlines are not included in timing results
    tft.drawRect(cx-i2, cy-i2, i, i, color2);
    yield();
  }
  return t;
}
//
unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = tft.width(), h = tft.height(), r2 = radius * 2;
  tft.fillScreen(GC9A01A_BLACK);
  start = micros();
  for(x=radius; x<w; x+=r2) {
    for(y=radius; y<h; y+=r2) {
      tft.fillCircle(x, y, radius, color);
    }
  }
  return micros() - start;
}
//
unsigned long testCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int           x, y, r2 = radius * 2,
                w = tft.width()  + radius,
                h = tft.height() + radius;
  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for(x=0; x<w; x+=r2) {
    for(y=0; y<h; y+=r2) {
      tft.drawCircle(x, y, radius, color);
    }
  }
  return micros() - start;
}
//
unsigned long testTriangles() {
  unsigned long start;
  int           n, i, cx = tft.width()  / 2 - 1,
                      cy = tft.height() / 2 - 1;
  tft.fillScreen(GC9A01A_BLACK);
  n     = min(cx, cy);
  start = micros();
  for(i=0; i<n; i+=5) {
    tft.drawTriangle(
      cx    , cy - i, // peak
      cx - i, cy + i, // bottom left
      cx + i, cy + i, // bottom right
      tft.color565(i, i, i));
  }
  return micros() - start;
}
//
unsigned long testFilledTriangles() {
  unsigned long start, t = 0;
  int           i, cx = tft.width()  / 2 - 1,
                   cy = tft.height() / 2 - 1;
  tft.fillScreen(GC9A01A_BLACK);
  start = micros();
  for(i=min(cx,cy); i>10; i-=5) {
    start = micros();
    tft.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      tft.color565(0, i*10, i*10));
    t += micros() - start;
    tft.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      tft.color565(i*10, i*10, 0));
    yield();
  }
  return t;
}
//
unsigned long testRoundRects() {
  unsigned long start;
  int           w, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;
  tft.fillScreen(GC9A01A_BLACK);
  w     = min(tft.width(), tft.height());
  start = micros();
  for(i=0; i<w; i+=6) {
    i2 = i / 2;
    tft.drawRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(i, 0, 0));
  }
  return micros() - start;
}
//
unsigned long testFilledRoundRects() {
  unsigned long start;
  int           i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;
  tft.fillScreen(GC9A01A_BLACK);
  start = micros();
  for(i=min(tft.width(), tft.height()); i>20; i-=6) {
    i2 = i / 2;
    tft.fillRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(0, i, 0));
    yield();
  }
  return micros() - start;
}
