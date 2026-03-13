#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define TFT_CS 15
#define TFT_DC 2

const char *WIFI_SSID = "Wokwi-GUEST";
const char *WIFI_PASS = "";
const char *MQTT_HOST = "100.64.0.2";
const int   MQTT_PORT = 1883;

#define TOPIC_TEMP   "/home/temp"
#define TOPIC_HUMID  "/home/humid"
#define TOPIC_ALERT  "/home/alerts"
#define TOPIC_STATUS "/home/status"

#define C_BG     0x0821
#define C_CARD   0x1082
#define C_BORDER 0x294A
#define C_WHITE  0xFFFF
#define C_GRAY   0x7BEF
#define C_DKGRAY 0x2965
#define C_GREEN  0x07E0
#define C_RED    0xF800
#define C_ORANGE 0xFC60
#define C_YELLOW 0xFFE0
#define C_CYAN   0x07FF

#define HDR_Y  0
#define HDR_H  30
#define TEMP_Y 30
#define TEMP_H 70
#define HUM_Y  100
#define HUM_H  70
#define ALT_Y  170
#define ALT_H  90
#define LOG_Y  260
#define LOG_H  60

#define ALERT_LINES 3  // number of alert history lines shown

Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
Adafruit_FT6206  ctp;
WiFiClient       wifiClient;
PubSubClient     mqtt(wifiClient);

// ── Live values ───────────────────────────────────────────────────────────────
String valTemp   = "--";
String valHumid  = "--";
String valStatus = "Waiting...";
bool   mqttOK    = false;

// Alert ring buffer — newest at index [ALERT_LINES-1]
String alertHistory[ALERT_LINES];
int    alertCount = 0;   // total alerts received (caps at ALERT_LINES for display)

// Log ring buffer
String logLines[3];

// ── Shadow values — track what is currently drawn ─────────────────────────────
String  drawnTemp;
String  drawnHumid;
String  drawnStatus;
bool    drawnMqttOK     = false;
String  drawnAlerts[ALERT_LINES];
String  drawnLog[3];
bool    drawnAlertFlash = false;

// ── Dirty flags (set when we KNOW a redraw is needed regardless of shadow) ────
bool dirtyHdr   = false;
bool dirtyTemp  = false;
bool dirtyHumid = false;
bool dirtyAlert = false;
bool dirtyLog   = false;
bool fullRedraw = true;

// ── Alert flash ───────────────────────────────────────────────────────────────
bool     alertFlashing    = false;
uint32_t alertFlashStart  = 0;
uint8_t  alertPhase       = 0;

uint32_t lastTouchMs = 0;

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

void drawCardBg(int y, int h, uint16_t bg, uint16_t border) {
  tft.fillRect(0, y, 240, h, C_BG);
  tft.fillRoundRect(2, y + 2, 236, h - 4, 7, bg);
  tft.drawRoundRect(2, y + 2, 236, h - 4, 7, border);
}

void cardLabel(int y, const char *label, uint16_t col) {
  tft.setTextSize(1);
  tft.setTextColor(col);
  tft.setCursor(10, y + 9);
  tft.print(label);
}

void cardValue(int y, int h, const char *val, uint16_t col) {
  int len  = strlen(val);
  int txtW = len * 18;
  int cx   = max(8, (240 - txtW) / 2);
  int cy   = y + (h / 2) - 4;
  tft.setTextSize(3);
  tft.setTextColor(col);
  tft.setCursor(cx, cy);
  tft.print(val);
}

void pushLog(const char *topic, const char *val) {
  logLines[0] = logLines[1];
  logLines[1] = logLines[2];
  char buf[36];
  snprintf(buf, sizeof(buf), "%-6s: %.22s", topic, val);
  logLines[2] = String(buf);
}

void pushAlert(const String &msg) {
  alertHistory[0] = alertHistory[1];
  alertHistory[1] = alertHistory[2];
  alertHistory[2] = msg;
  alertCount++;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Draw functions
// ─────────────────────────────────────────────────────────────────────────────

void drawHeader() {
  drawCardBg(HDR_Y, HDR_H, C_CARD, C_BORDER);
  tft.setTextColor(C_WHITE); tft.setTextSize(1);
  tft.setCursor(10, HDR_Y + 7);
  tft.print("HOME MONITOR");
  tft.setTextColor(C_GRAY);
  tft.setCursor(10, HDR_Y + 19);
  String s = valStatus;
  if (s.length() > 16) s = s.substring(0, 16);
  tft.print(s);
  tft.fillCircle(225, HDR_Y + 15, 5, mqttOK ? C_GREEN : C_RED);
  tft.drawCircle(225, HDR_Y + 15, 5, C_DKGRAY);

  drawnStatus  = valStatus;
  drawnMqttOK  = mqttOK;
}

void drawTempCard() {
  drawCardBg(TEMP_Y, TEMP_H, C_CARD, C_BORDER);
  cardLabel(TEMP_Y, "TEMPERATURE  (C)", C_GRAY);
  tft.setTextSize(1); tft.setTextColor(C_DKGRAY);
  tft.setCursor(195, TEMP_Y + 9);
  tft.print("deg C");
  String disp = valTemp;
  if (disp.length() > 8) disp = disp.substring(0, 8);
  cardValue(TEMP_Y, TEMP_H, disp.c_str(), C_CYAN);

  drawnTemp = valTemp;
}

void drawHumidCard() {
  drawCardBg(HUM_Y, HUM_H, C_CARD, C_BORDER);
  cardLabel(HUM_Y, "HUMIDITY  (%)", C_GRAY);
  tft.setTextSize(1); tft.setTextColor(C_DKGRAY);
  tft.setCursor(210, HUM_Y + 9);
  tft.print("%");
  String disp = valHumid;
  if (disp.length() > 8) disp = disp.substring(0, 8);
  cardValue(HUM_Y, HUM_H, disp.c_str(), C_GREEN);

  drawnHumid = valHumid;
}

// Alert card shows the last ALERT_LINES messages, newest at bottom
void drawAlertCard(bool flashOn) {
  uint16_t bg     = flashOn ? 0x8000  : C_CARD;
  uint16_t brd    = flashOn ? C_ORANGE : C_BORDER;
  uint16_t hdrCol = flashOn ? C_YELLOW : C_GRAY;
  uint16_t txtCol = flashOn ? C_WHITE  : C_ORANGE;

  drawCardBg(ALT_Y, ALT_H, bg, brd);

  tft.setTextSize(1); tft.setTextColor(hdrCol);
  tft.setCursor(10, ALT_Y + 9);
  tft.print(flashOn ? "!! ALERT !!" : "ALERT");

  // Count how many entries are non-empty
  int first = ALERT_LINES;
  for (int i = 0; i < ALERT_LINES; i++)
    if (alertHistory[i].length() > 0) { first = i; break; }

  if (first == ALERT_LINES) {
    // Nothing received yet
    tft.setTextColor(C_DKGRAY);
    tft.setCursor(10, ALT_Y + 28);
    tft.print("No alerts");
  } else {
    int lineH  = 17;
    int startY = ALT_Y + 24;
    for (int i = first; i < ALERT_LINES; i++) {
      // Newest line is brightest
      uint16_t lc = (i == ALERT_LINES - 1) ? txtCol : C_DKGRAY;
      tft.setTextColor(lc);
      tft.setCursor(10, startY + (i - first) * lineH);
      // Truncate to 28 chars to fit card width
      String line = alertHistory[i];
      if (line.length() > 28) line = line.substring(0, 28);
      tft.print(line);
    }
  }

  if (!flashOn && alertCount > 0) {
    tft.setTextColor(C_DKGRAY);
    tft.setCursor(140, ALT_Y + ALT_H - 14);
    tft.print("tap to clear");
  }

  drawnAlertFlash = flashOn;
  for (int i = 0; i < ALERT_LINES; i++) drawnAlerts[i] = alertHistory[i];
}

void drawLogCard() {
  drawCardBg(LOG_Y, LOG_H, C_BG, C_BORDER);
  tft.setTextSize(1); tft.setTextColor(C_DKGRAY);
  tft.setCursor(10, LOG_Y + 6);
  tft.print("Last received:");
  for (int i = 0; i < 3; i++) {
    tft.setTextColor(i == 2 ? C_GRAY : C_DKGRAY);
    tft.setCursor(10, LOG_Y + 18 + i * 13);
    tft.print(logLines[i]);
  }
  for (int i = 0; i < 3; i++) drawnLog[i] = logLines[i];
}

void drawAll() {
  tft.fillScreen(C_BG);
  drawHeader();
  drawTempCard();
  drawHumidCard();
  drawAlertCard(alertPhase);
  drawLogCard();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Selective update — compare shadow, repaint only what changed
// ─────────────────────────────────────────────────────────────────────────────

void updateDisplay() {
  // Header: MQTT status dot or status string changed
  if (dirtyHdr || valStatus != drawnStatus || mqttOK != drawnMqttOK) {
    drawHeader();
    dirtyHdr = false;
  }

  if (dirtyTemp || valTemp != drawnTemp) {
    drawTempCard();
    dirtyTemp = false;
  }

  if (dirtyHumid || valHumid != drawnHumid) {
    drawHumidCard();
    dirtyHumid = false;
  }

  // Alert: flash phase changed, or history content changed
  bool alertsDiffer = false;
  for (int i = 0; i < ALERT_LINES; i++)
    if (alertHistory[i] != drawnAlerts[i]) { alertsDiffer = true; break; }

  if (dirtyAlert || alertsDiffer || (alertPhase != drawnAlertFlash)) {
    drawAlertCard(alertPhase);
    dirtyAlert = false;
  }

  // Log: any line changed
  bool logDiffer = false;
  for (int i = 0; i < 3; i++)
    if (logLines[i] != drawnLog[i]) { logDiffer = true; break; }

  if (dirtyLog || logDiffer) {
    drawLogCard();
    dirtyLog = false;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  MQTT callback
// ─────────────────────────────────────────────────────────────────────────────

void mqttCallback(char *topic, byte *payload, unsigned int len) {
  char buf[128];
  int n = min((int)len, 127);
  memcpy(buf, payload, n);
  buf[n] = '\0';
  String msg = String(buf);
  msg.trim();

  if (strcmp(topic, TOPIC_TEMP) == 0) {
    valTemp = msg;
    pushLog("TEMP", msg.c_str());
  } else if (strcmp(topic, TOPIC_HUMID) == 0) {
    valHumid = msg;
    pushLog("HUMID", msg.c_str());
  } else if (strcmp(topic, TOPIC_ALERT) == 0) {
    pushAlert(msg);
    alertFlashing   = true;
    alertFlashStart = millis();
    alertPhase      = 1;
    pushLog("ALERT", msg.c_str());
  } else if (strcmp(topic, TOPIC_STATUS) == 0) {
    valStatus = msg;
    pushLog("STATUS", msg.c_str());
  }
  // Shadow comparison in updateDisplay() handles whether a repaint is needed
}

// ─────────────────────────────────────────────────────────────────────────────
//  WiFi / MQTT connection
// ─────────────────────────────────────────────────────────────────────────────

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t++ < 40) delay(250);
}

void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  for (int t = 0; t < 8 && !mqtt.connected(); t++) {
    String id = "ESP32Dash-" + String(esp_random() & 0xFFFF, HEX);
    if (mqtt.connect(id.c_str())) {
      mqtt.subscribe(TOPIC_TEMP);
      mqtt.subscribe(TOPIC_HUMID);
      mqtt.subscribe(TOPIC_ALERT);
      mqtt.subscribe(TOPIC_STATUS);
      mqttOK = true;
    } else {
      delay(1500);
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Setup / Loop
// ─────────────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(C_BG);
  Wire.begin();
  ctp.begin(40);

  tft.setTextColor(C_CYAN); tft.setTextSize(2);
  tft.setCursor(20, 100);
  tft.print("Home Monitor");
  tft.setTextColor(C_GRAY); tft.setTextSize(1);
  tft.setCursor(60, 130);
  tft.print("Connecting...");

  connectWiFi();
  connectMQTT();
  drawAll();
}

void loop() {
  if (!mqtt.connected()) {
    mqttOK = false;
    connectMQTT();
  }
  mqtt.loop();

  // Alert flash — 250 ms toggle for 3 seconds
  if (alertFlashing) {
    uint32_t elapsed = millis() - alertFlashStart;
    if (elapsed < 3000) {
      uint8_t phase = (elapsed / 250) % 2;
      if (phase != alertPhase) alertPhase = phase;
    } else {
      alertFlashing = false;
      alertPhase    = 0;
    }
  }

  updateDisplay();

  // Touch — tap alert card to clear history
  if (ctp.touched()) {
    uint32_t now = millis();
    if (now - lastTouchMs > 300) {
      lastTouchMs = now;
      TS_Point p  = ctp.getPoint();
      int sy = map(p.y, 0, 320, 319, 0);
      if (sy >= ALT_Y && sy < ALT_Y + ALT_H) {
        alertFlashing = false;
        alertPhase    = 0;
        alertCount    = 0;
        for (int i = 0; i < ALERT_LINES; i++) alertHistory[i] = "";
      }
    }
  }

  delay(16);
}