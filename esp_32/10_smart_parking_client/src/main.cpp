#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Adafruit_FT6206.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char *WIFI_SSID = "Wokwi-GUEST";
const char *WIFI_PASS = "";
const char *BROKER = "100.64.0.2";
const int BROKER_PORT = 1883;

const char *TOPIC_STATUS = "smartparking/status";
const char *TOPIC_CMD = "smartparking/cmd";
const char *TOPIC_ACK = "smartparking/ack";

#define TFT_CS 5
#define TFT_DC 15

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_FT6206 ctp = Adafruit_FT6206();

#define C_BG ILI9341_BLACK
#define C_HEADER 0x1082
#define C_FREE 0x07E0
#define C_OCCUPIED 0xF800
#define C_RESERVED 0xFFE0
#define C_SELECT 0x001F
#define C_DK ILI9341_BLACK
#define C_LT ILI9341_WHITE
#define C_FOOTER 0x2124

#define SCREEN_W 240
#define SCREEN_H 320
#define HEADER_H 40
#define FOOTER_H 44
#define FOOTER_Y (SCREEN_H - FOOTER_H)
#define SLOT_COLS 2
#define SLOT_ROWS 3
#define SLOT_W (SCREEN_W / SLOT_COLS)
#define SLOT_H ((SCREEN_H - HEADER_H - FOOTER_H) / SLOT_ROWS)
#define PAD 4
#define NUM_SLOTS 6

struct SlotInfo
{
  bool occupied, reserved, available;
  int dist;
};

SlotInfo slots[NUM_SLOTS];
SlotInfo prevSlots[NUM_SLOTS]; // shadow copy for dirty detection

int selectedSlot = -1;
int prevSelected = -2; // -2 forces initial draw
int myReservedSlot = -1;
int prevReserved = -2;
bool wifiWasConn = false;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// ── Geometry helper ──────────────────────────────────────────────────────

void slotRect(int idx, int &x, int &y, int &w, int &h)
{
  x = (idx % SLOT_COLS) * SLOT_W + PAD;
  y = HEADER_H + (idx / SLOT_COLS) * SLOT_H + PAD;
  w = SLOT_W - PAD * 2;
  h = SLOT_H - PAD * 2;
}

// ── Draw one slot ────────────────────────────────────────────────────────

void drawSlot(int idx)
{
  int x, y, w, h;
  slotRect(idx, x, y, w, h);

  uint16_t bg;
  const char *label;
  if (slots[idx].occupied)
  {
    bg = C_OCCUPIED;
    label = "OCCUPIED";
  }
  else if (slots[idx].reserved)
  {
    bg = C_RESERVED;
    label = "RESERVED";
  }
  else
  {
    bg = C_FREE;
    label = "FREE";
  }

  // Clear the selection-border zone before drawing
  tft.fillRoundRect(x - 3, y - 3, w + 6, h + 6, 6, C_BG);

  if (idx == selectedSlot)
    tft.fillRoundRect(x - 3, y - 3, w + 6, h + 6, 6, C_SELECT);

  tft.fillRoundRect(x, y, w, h, 4, bg);

  tft.setTextColor(C_DK);
  tft.setTextSize(3);
  tft.setCursor(x + w / 2 - 9, y + 8);
  tft.print(idx + 1);

  tft.setTextSize(1);
  int lw = strlen(label) * 6;
  tft.setCursor(x + w / 2 - lw / 2, y + h - 16);
  tft.print(label);

  if (!slots[idx].occupied && slots[idx].dist < 500)
  {
    char buf[8];
    snprintf(buf, sizeof(buf), "%dcm", slots[idx].dist);
    int dw = strlen(buf) * 6;
    tft.setCursor(x + w / 2 - dw / 2, y + h - 27);
    tft.print(buf);
  }
}

// ── Draw footer ──────────────────────────────────────────────────────────

void drawFooter()
{
  tft.fillRect(0, FOOTER_Y, SCREEN_W, FOOTER_H, C_FOOTER);

  if (selectedSlot == -1)
  {
    tft.setTextColor(0x8410);
    tft.setTextSize(1);
    tft.setCursor(28, FOOTER_Y + 16);
    tft.print("Tap a slot to select");
    return;
  }
  if (slots[selectedSlot].occupied)
  {
    tft.setTextColor(C_OCCUPIED);
    tft.setTextSize(1);
    tft.setCursor(20, FOOTER_Y + 16);
    tft.print("Slot ");
    tft.print(selectedSlot + 1);
    tft.print(" is occupied");
    return;
  }
  if (selectedSlot == myReservedSlot)
  {
    tft.fillRoundRect(16, FOOTER_Y + 8, 208, 28, 5, C_OCCUPIED);
    tft.setTextColor(C_LT);
    tft.setTextSize(1);
    tft.setCursor(44, FOOTER_Y + 18);
    tft.print("CANCEL RESERVATION");
  }
  else if (slots[selectedSlot].reserved)
  {
    tft.setTextColor(C_RESERVED);
    tft.setTextSize(1);
    tft.setCursor(20, FOOTER_Y + 16);
    tft.print("Slot ");
    tft.print(selectedSlot + 1);
    tft.print(" already reserved");
  }
  else
  {
    tft.fillRoundRect(16, FOOTER_Y + 8, 208, 28, 5, C_FREE);
    tft.setTextColor(C_DK);
    tft.setTextSize(1);
    tft.setCursor(48, FOOTER_Y + 18);
    tft.print("RESERVE SLOT ");
    tft.print(selectedSlot + 1);
  }
}

// ── Draw header ──────────────────────────────────────────────────────────

void drawHeader()
{
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, C_HEADER);
  tft.setTextColor(C_LT);
  tft.setTextSize(2);
  tft.setCursor(8, 12);
  tft.print("SMART PARKING");
  tft.fillCircle(SCREEN_W - 14, 20, 6,
                 (WiFi.status() == WL_CONNECTED) ? C_FREE : C_OCCUPIED);
}

// ── WiFi dot only ────────────────────────────────────────────────────────

void updateWifiDot()
{
  bool conn = (WiFi.status() == WL_CONNECTED);
  if (conn != wifiWasConn)
  {
    tft.fillCircle(SCREEN_W - 14, 20, 6, conn ? C_FREE : C_OCCUPIED);
    wifiWasConn = conn;
  }
}

// ── Full initial paint ───────────────────────────────────────────────────

void drawAll()
{
  tft.fillScreen(C_BG);
  drawHeader();
  for (int i = 0; i < NUM_SLOTS; i++)
    drawSlot(i);
  drawFooter();
  memcpy(prevSlots, slots, sizeof(slots));
  prevSelected = selectedSlot;
  prevReserved = myReservedSlot;
  wifiWasConn = (WiFi.status() == WL_CONNECTED);
}

// ── Partial update — only dirty regions ─────────────────────────────────

void updateDisplay()
{
  bool footerDirty = (selectedSlot != prevSelected) ||
                     (myReservedSlot != prevReserved);

  for (int i = 0; i < NUM_SLOTS; i++)
  {
    bool dataDiff = (slots[i].occupied != prevSlots[i].occupied ||
                     slots[i].reserved != prevSlots[i].reserved ||
                     slots[i].dist != prevSlots[i].dist);
    bool selDiff = (i == selectedSlot) != (i == prevSelected);

    if (dataDiff || selDiff)
    {
      drawSlot(i);
      if (i == selectedSlot || i == prevSelected)
        footerDirty = true;
    }
    if (dataDiff && i == selectedSlot)
      footerDirty = true;
  }

  // Redraw the previously selected slot to remove its highlight ring
  if (selectedSlot != prevSelected && prevSelected >= 0)
  {
    drawSlot(prevSelected);
  }

  if (footerDirty)
    drawFooter();
  updateWifiDot();

  memcpy(prevSlots, slots, sizeof(slots));
  prevSelected = selectedSlot;
  prevReserved = myReservedSlot;
}

// ── MQTT ─────────────────────────────────────────────────────────────────

void publishCmd(const char *action, int slotId)
{
  char buf[64];
  snprintf(buf, sizeof(buf), "{\"action\":\"%s\",\"slot\":%d}", action, slotId);
  mqtt.publish(TOPIC_CMD, buf);
}

void onMessage(char *topic, byte *payload, unsigned int len)
{
  if (strcmp(topic, TOPIC_STATUS) != 0)
    return;

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, payload, len) != DeserializationError::Ok)
    return;

  JsonArray arr = doc["slots"].as<JsonArray>();
  int i = 0;
  for (JsonObject s : arr)
  {
    if (i >= NUM_SLOTS)
      break;
    slots[i].occupied = s["occupied"].as<bool>();
    slots[i].reserved = s["reserved"].as<bool>();
    slots[i].available = s["available"].as<bool>();
    slots[i].dist = s["dist"].as<int>();
    i++;
  }

  if (myReservedSlot >= 0 && slots[myReservedSlot].occupied)
  {
    myReservedSlot = -1;
    selectedSlot = -1;
  }

  updateDisplay();
}

// ── Touch ────────────────────────────────────────────────────────────────

void handleTouch()
{
  if (!ctp.touched())
    return;
  delay(10);
  if (!ctp.touched())
    return;

  TS_Point p = ctp.getPoint();
  p.x = map(p.x, 0, 240, 240, 0);
  p.y = map(p.y, 0, 320, 320, 0);

  if (p.y >= FOOTER_Y && selectedSlot != -1)
  {
    if (!slots[selectedSlot].occupied)
    {
      if (selectedSlot == myReservedSlot)
      {
        publishCmd("release", selectedSlot + 1);
        myReservedSlot = -1;
        selectedSlot = -1;
      }
      else if (!slots[selectedSlot].reserved)
      {
        if (myReservedSlot >= 0)
          publishCmd("release", myReservedSlot + 1);
        publishCmd("reserve", selectedSlot + 1);
        myReservedSlot = selectedSlot;
      }
    }
    updateDisplay();
    return;
  }

  if (p.y >= HEADER_H && p.y < FOOTER_Y)
  {
    int idx = (p.y - HEADER_H) / SLOT_H * SLOT_COLS + p.x / SLOT_W;
    if (idx >= 0 && idx < NUM_SLOTS)
    {
      selectedSlot = (selectedSlot == idx) ? -1 : idx;
      updateDisplay();
    }
  }
}

// ── Connections ───────────────────────────────────────────────────────────

void connectWiFi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  tft.setTextColor(0x8410);
  tft.setTextSize(1);
  tft.setCursor(10, 140);
  tft.print("Connecting WiFi...");
  while (WiFi.status() != WL_CONNECTED)
    delay(500);
}

void connectMQTT()
{
  String cid = "ParkClient-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  while (!mqtt.connected())
  {
    if (mqtt.connect(cid.c_str()))
    {
      mqtt.subscribe(TOPIC_STATUS);
      mqtt.subscribe(TOPIC_ACK);
    }
    else
    {
      delay(2000);
    }
  }
}

// ── Setup / Loop ──────────────────────────────────────────────────────────

void setup()
{
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(C_BG);
  tft.setTextColor(C_LT);
  tft.setTextSize(2);
  tft.setCursor(20, 110);
  tft.print("Smart Parking");

  if (!ctp.begin(40))
  {
    tft.fillScreen(C_OCCUPIED);
    tft.setCursor(10, 140);
    tft.setTextSize(2);
    tft.print("Touch FAIL");
    while (1)
      ;
  }

  connectWiFi();
  mqtt.setServer(BROKER, BROKER_PORT);
  mqtt.setBufferSize(1024);
  mqtt.setCallback(onMessage);
  connectMQTT();

  tft.fillScreen(C_BG);
  tft.setTextColor(C_FREE);
  tft.setTextSize(2);
  tft.setCursor(30, 150);
  tft.print("Connected!");
  delay(800);

  memset(slots, 0, sizeof(slots));
  memset(prevSlots, 0, sizeof(prevSlots));
  drawAll();
}

void loop()
{
  if (!mqtt.connected())
    connectMQTT();
  mqtt.loop();
  handleTouch();
  updateWifiDot();
}