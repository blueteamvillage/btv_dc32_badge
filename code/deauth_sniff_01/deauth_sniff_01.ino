#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <esp_wifi_types.h>
#include <esp_wifi.h>

#define TFT_DC 16
#define TFT_CS 5
#define MAX_CHANNELS 13

Adafruit_GC9A01A tft(TFT_CS, TFT_DC);

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

void setup()
{
  WiFi.begin();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(packetSniffer);
  initializeDisplay();

  Serial.begin(115200);

  tft.begin();

  tft.fillScreen(0x07E0);

  previousChannelHopTime = millis();
}

void initializeDisplay()
{
  // undetermined currently
}

void channelHop()
{
  currentChannel = (currentChannel % MAX_CHANNELS) + 1;
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
  Serial.println("Deauth: " + String(deauthCount));
  deauthCount = 0;
  Serial.println("Channel: " + String(currentChannel));
}

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

void loop()
{
  if (millis() - previousChannelHopTime >= CHANNEL_HOP_INTERVAL)
  {

    if (deauthCount >= deauthThreshold)
    {
      Serial.println("Too many Deauths! (>=20)");
      tft.fillScreen(0xF800); // go red
      delay(3500);
      tft.fillScreen(0x07E0); // reset to green
    }
    channelHop();
    previousChannelHopTime = millis();
  }
}