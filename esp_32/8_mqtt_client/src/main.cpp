#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define TFT_CS  15
#define TFT_DC   2

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

const char* MQTT_HOST    = "100.64.0.2";
const int   MQTT_PORT    = 1883;
const char* TOPIC_INPUT  = "calc/input";
const char* TOPIC_RESULT = "calc/result";

#define EXPR_Y    0
#define EXPR_H   60
#define RESULT_Y 60
#define RESULT_H 50
#define KEY_Y   114
#define KEY_ROWS  4
#define KEY_COLS  5
#define KEY_W   (240 / KEY_COLS)             // 48 px
#define KEY_H   ((320 - KEY_Y) / KEY_ROWS)   // 51 px

#define C_BG       0x0841
#define C_PANEL    0x10A2
#define C_BORDER   0x2945
#define C_EXPR_TXT 0xFFE0   // yellow
#define C_RES_TXT  0x07EF   // cyan
#define C_DIM      0x7BCF   // muted grey
#define C_NUM      0x2945   // dark slate
#define C_OP       0x0219   // teal
#define C_PAR      0x2104   // plum
#define C_SEND     0x0480   // green
#define C_DEL      0x7800   // red
#define C_CLR      0x3186   // indigo
#define C_FLASH    0xFFFF   // white press-flash
#define C_WHITE    0xFFFF
#define C_BLACK    0x0000

struct Key { const char* label; uint8_t col, row; uint16_t bg, fg; };

const Key KEYS[] = {
  // Row 0
  { "7",    0, 0, C_NUM,  C_WHITE    },
  { "8",    1, 0, C_NUM,  C_WHITE    },
  { "9",    2, 0, C_NUM,  C_WHITE    },
  { "/",    3, 0, C_OP,   C_EXPR_TXT },
  { "(",    4, 0, C_PAR,  C_RES_TXT  },
  // Row 1
  { "4",    0, 1, C_NUM,  C_WHITE    },
  { "5",    1, 1, C_NUM,  C_WHITE    },
  { "6",    2, 1, C_NUM,  C_WHITE    },
  { "*",    3, 1, C_OP,   C_EXPR_TXT },
  { ")",    4, 1, C_PAR,  C_RES_TXT  },
  // Row 2
  { "1",    0, 2, C_NUM,  C_WHITE    },
  { "2",    1, 2, C_NUM,  C_WHITE    },
  { "3",    2, 2, C_NUM,  C_WHITE    },
  { "-",    3, 2, C_OP,   C_EXPR_TXT },
  { "DEL",  4, 2, C_DEL,  C_WHITE    },
  // Row 3
  { "0",    0, 3, C_NUM,  C_WHITE    },
  { ".",    1, 3, C_NUM,  C_WHITE    },
  { "+",    2, 3, C_OP,   C_EXPR_TXT },
  { "CLR",  3, 3, C_CLR,  C_WHITE    },
  { "SEND", 4, 3, C_SEND, C_WHITE    },
};
constexpr int NUM_KEYS = sizeof(KEYS) / sizeof(KEYS[0]);

// ── Globals ───────────────────────────────────────────────────────────────────
Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
Adafruit_FT6206  ctp;                // I²C – no CS needed
WiFiClient       wifiClient;
PubSubClient     mqtt(wifiClient);

String expression  = "";
String resultText  = "";
bool   exprDirty   = false;
bool   resultDirty = false;

uint32_t lastTouchMs  = 0;
const uint32_t DEBOUNCE = 220;   // ms

// ── Forward Declarations ──────────────────────────────────────────────────────
void drawUI();
void drawExprPanel();
void drawResultPanel();
void drawAllKeys();
void drawKey(const Key& k, bool pressed = false);
void flashKey(const Key& k);
void handleKey(const char* label);
void statusLine(const char* msg, uint16_t col);
void connectWiFi();
void connectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int len);

// ═════════════════════════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  // ── Display ──
  tft.begin();
  tft.setRotation(0);           // portrait, USB at bottom
  tft.fillScreen(C_BG);

  // ── FT6206 capacitive touch (I²C) ──
  // Default Wire uses GPIO21=SDA, GPIO22=SCL on ESP32 DevKit C v4.
  // If your board routes I²C elsewhere, call Wire.begin(SDA_PIN, SCL_PIN)
  // here before ctp.begin().
  Wire.begin();                 // SDA=21, SCL=22 (ESP32 DevKit C v4 defaults)
  if (!ctp.begin(40)) {         // 40 = sensitivity coefficient (matches reference)
    Serial.println("[touch] FT6206 not found – check SDA/SCL wiring");
    // Non-fatal in Wokwi – sim may not enforce I²C probe
  }

  // ── Splash ──
  tft.setTextColor(C_RES_TXT);
  tft.setTextSize(2);
  tft.setCursor(18, 80);
  tft.print("MQTT Calc v2.0");

  tft.setTextColor(C_DIM);
  tft.setTextSize(1);
  tft.setCursor(38, 108);
  tft.print("ESP32 + ILI9341-CAP");

  // ── Connect WiFi ──
  statusLine("Connecting WiFi...", C_EXPR_TXT);
  connectWiFi();
  statusLine("WiFi OK!", 0x07E0);
  delay(350);

  // ── Connect MQTT ──
  statusLine("Connecting MQTT...", C_EXPR_TXT);
  connectMQTT();
  statusLine("MQTT Ready!", 0x07E0);
  delay(350);

  // ── Main UI ──
  drawUI();
}

// ═════════════════════════════════════════════════════════════════════════════
//  LOOP
// ═════════════════════════════════════════════════════════════════════════════
void loop() {
  // Keep MQTT alive
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  // Redraw only dirty panels
  if (exprDirty) { drawExprPanel();  exprDirty   = false; }
  if (resultDirty){ drawResultPanel(); resultDirty = false; }

  // ── Touch Handling ──────────────────────────────────────────────────────
  // FT6206 pattern from reference code: poll ctp.touched(), getPoint(),
  // then mirror both axes exactly as in the reference sketch.
  if (!ctp.touched()) return;

  uint32_t now = millis();
  if (now - lastTouchMs < DEBOUNCE) return;
  lastTouchMs = now;

  TS_Point p = ctp.getPoint();

  // Mirror axes to match screen orientation (portrait, rotation=0)
  // Identical to: p.x = map(p.x, 0, 240, 240, 0)  in the reference code.
  int sx = map(p.x, 0, 240, 239, 0);
  int sy = map(p.y, 0, 320, 319, 0);
  sx = constrain(sx, 0, 239);
  sy = constrain(sy, 0, 319);

  Serial.printf("[touch] raw(%d,%d) → screen(%d,%d)\n", p.x, p.y, sx, sy);

  // Only handle touches inside the keypad area
  if (sy < KEY_Y) return;

  int col = constrain(sx / KEY_W, 0, KEY_COLS - 1);
  int row = constrain((sy - KEY_Y) / KEY_H, 0, KEY_ROWS - 1);

  for (int i = 0; i < NUM_KEYS; i++) {
    if (KEYS[i].col == col && KEYS[i].row == row) {
      flashKey(KEYS[i]);
      handleKey(KEYS[i].label);
      return;
    }
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  KEY HANDLING
// ═════════════════════════════════════════════════════════════════════════════
void handleKey(const char* label) {
  if (strcmp(label, "DEL") == 0) {
    if (expression.length() > 0) {
      expression.remove(expression.length() - 1);
      exprDirty = true;
    }
  } else if (strcmp(label, "CLR") == 0) {
    expression  = "";
    resultText  = "";
    exprDirty   = true;
    resultDirty = true;
  } else if (strcmp(label, "SEND") == 0) {
    if (expression.length() > 0) {
      resultText  = "Waiting...";
      resultDirty = true;
      mqtt.publish(TOPIC_INPUT, expression.c_str());
      Serial.print("[mqtt] published → ");
      Serial.println(expression);
    }
  } else {
    if (expression.length() < 40) {
      expression += String(label);
      exprDirty = true;
    }
  }
}

// ═════════════════════════════════════════════════════════════════════════════
//  MQTT CALLBACK
// ═════════════════════════════════════════════════════════════════════════════
void mqttCallback(char* topic, byte* payload, unsigned int len) {
  resultText = "";
  for (unsigned int i = 0; i < len; i++) resultText += (char)payload[i];
  resultDirty = true;
  Serial.print("[mqtt] result ← ");
  Serial.println(resultText);
}

// ═════════════════════════════════════════════════════════════════════════════
//  DISPLAY
// ═════════════════════════════════════════════════════════════════════════════
void drawUI() {
  tft.fillScreen(C_BG);
  drawExprPanel();
  drawResultPanel();
  tft.fillRect(0, KEY_Y - 4, 240, 4, C_BORDER);   // thick divider
  drawAllKeys();
}

void drawExprPanel() {
  tft.fillRect(0, EXPR_Y, 240, EXPR_H, C_PANEL);
  tft.drawRect(0, EXPR_Y, 240, EXPR_H, C_BORDER);

  tft.setTextSize(1);
  tft.setTextColor(C_DIM);
  tft.setCursor(5, EXPR_Y + 4);
  tft.print("EXPRESSION");

  char cntBuf[6];
  snprintf(cntBuf, sizeof(cntBuf), "%2dc", (int)expression.length());
  tft.setCursor(205, EXPR_Y + 4);
  tft.print(cntBuf);

  // Two lines, 18 chars each at size-2 (12 px/char → 216 px max)
  const int MAX_CHARS = 18;
  String disp = expression;
  if ((int)disp.length() > MAX_CHARS * 2)
    disp = disp.substring(disp.length() - MAX_CHARS * 2);

  String l1 = disp.length() <= MAX_CHARS ? disp          : disp.substring(0, MAX_CHARS);
  String l2 = disp.length() <= MAX_CHARS ? String("")    : disp.substring(MAX_CHARS);

  while ((int)l1.length() < MAX_CHARS) l1 += ' ';
  while ((int)l2.length() < MAX_CHARS) l2 += ' ';

  tft.setTextColor(C_EXPR_TXT);
  tft.setTextSize(2);
  tft.setCursor(5, EXPR_Y + 16);
  tft.print(l1);
  tft.setCursor(5, EXPR_Y + 38);
  tft.print(l2);
}

void drawResultPanel() {
  tft.fillRect(0, RESULT_Y, 240, RESULT_H, C_PANEL);
  tft.drawRect(0, RESULT_Y, 240, RESULT_H, C_BORDER);

  tft.setTextSize(1);
  tft.setTextColor(C_DIM);
  tft.setCursor(5, RESULT_Y + 4);
  tft.print("RESULT");

  String disp = resultText;
  disp.trim();
  if ((int)disp.length() > 19) disp = disp.substring(0, 19);
  while ((int)disp.length() < 19) disp += ' ';

  tft.setTextColor(C_RES_TXT);
  tft.setTextSize(2);
  tft.setCursor(5, RESULT_Y + 22);
  tft.print(disp);
}

void drawAllKeys() {
  for (int i = 0; i < NUM_KEYS; i++) drawKey(KEYS[i]);
}

void drawKey(const Key& k, bool pressed) {
  int x = k.col * KEY_W;
  int y = KEY_Y + k.row * KEY_H;
  int w = KEY_W;
  int h = KEY_H;

  uint16_t bg = pressed ? C_FLASH : k.bg;
  uint16_t fg = pressed ? C_BLACK : k.fg;

  tft.fillRoundRect(x + 2, y + 2, w - 4, h - 4, 5, bg);
  tft.drawRoundRect(x + 2, y + 2, w - 4, h - 4, 5, C_BORDER);

  int len = strlen(k.label);
  if (len == 1) {
    tft.setTextSize(2);
    tft.setTextColor(fg);
    tft.setCursor(x + w / 2 - 6, y + h / 2 - 8);
    tft.print(k.label);
  } else {
    tft.setTextSize(1);
    tft.setTextColor(fg);
    tft.setCursor(x + w / 2 - len * 3, y + h / 2 - 4);
    tft.print(k.label);
  }
}

void flashKey(const Key& k) {
  drawKey(k, true);
  delay(70);
  drawKey(k, false);
}

// ─────────────────────────────────────────────────────────────────────────────
void statusLine(const char* msg, uint16_t col) {
  tft.fillRect(0, 152, 240, 14, C_BG);
  tft.setTextSize(1);
  tft.setTextColor(col);
  int16_t x1, y1; uint16_t w, h;
  tft.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - (int)w) / 2, 154);
  tft.print(msg);
}

// ═════════════════════════════════════════════════════════════════════════════
//  WiFi
// ═════════════════════════════════════════════════════════════════════════════
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(250);
    Serial.print('.');
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("\n[wifi] IP %s\n", WiFi.localIP().toString().c_str());
  else
    Serial.println("\n[wifi] failed – continuing anyway");
}

// ═════════════════════════════════════════════════════════════════════════════
//  MQTT
// ═════════════════════════════════════════════════════════════════════════════
void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  int tries = 0;
  while (!mqtt.connected() && tries < 8) {
    String id = "ESP32Calc-" + String(esp_random() & 0xFFFF, HEX);
    Serial.printf("[mqtt] connecting as %s ... ", id.c_str());
    if (mqtt.connect(id.c_str())) {
      Serial.println("OK");
      mqtt.subscribe(TOPIC_RESULT);
      Serial.printf("[mqtt] subscribed → %s\n", TOPIC_RESULT);
    } else {
      Serial.printf("fail (rc=%d), retry\n", mqtt.state());
      delay(1500);
    }
    tries++;
  }
}