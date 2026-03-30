#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <qrcode.h>

#define RFID_SS_PIN  5
#define RFID_RST_PIN 4
#define SERVO_PIN    13
#define LED_GREEN    25
#define LED_YELLOW   26
#define LED_RED      27
#define BUZZER_PIN   14

const char *WIFI_SSID      = "Wokwi-GUEST";
const char *WIFI_PASSWORD  = "";
const char *MQTT_BROKER    = "100.64.0.2";
const int   MQTT_PORT      = 1883;
const char *MQTT_CLIENT_ID = "ZOOP_ESP32_001";
const char *TOPIC_STATUS   = "zoop/status/entry";

#define TOLL_AMOUNT       65.0f
#define LOW_BAL_THRESHOLD 100.0f
#define PENALTY_FACTOR    2
#define BARRIER_OPEN_MS   3000
#define OLED_WIDTH        128
#define OLED_HEIGHT       64
#define OLED_I2C_ADDR     0x3C

struct Vehicle {
  const char *uid;
  const char *vehicleNo;
  const char *phone;
  float       balance;
};

Vehicle db[] = {
  { "01020304", "TN01AB1234", "9900000001", 500.0f },
  { "11223344", "KA03CD5678", "9900000002",  80.0f },
  { "55667788", "MH12EF9999", "9900000003",   0.0f }
};
const int DB_SIZE = sizeof(db) / sizeof(db[0]);

MFRC522          rfid(RFID_SS_PIN, RFID_RST_PIN);
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
Servo            barrier;
WiFiClient       wifiClient;
PubSubClient     mqtt(wifiClient);
WebServer        server(80);

String pendingUID       = "";
String pendingVehicleNo = "";
float  pendingAmount    = 0;
bool   paymentPending   = false;
bool   isProcessing     = false;

void connectWiFi();
void connectMQTT();
void handleRFID();
void processSufficient(Vehicle &v);
void processLowBalanceAlert(Vehicle &v);
void processZeroBalance(Vehicle &v);
void openBarrier();
void closeBarrier();
void clearLEDs();
void buzzerBeep(int times, int durationMs);
void oledIdle();
void oledStatus(const char *line1, const char *line2,
                const char *line3, const char *line4);
void oledQR(const char *url);
void publishMQTT(const char *topic, const char *payload);
String   uidToString(MFRC522::Uid &uid);
Vehicle *findVehicle(const String &uid);

void handlePayPage()
{
  if (!paymentPending) {
    server.send(404, "text/plain", "No pending payment.");
    return;
  }

  String html = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ZOOP Payment</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: Arial, sans-serif;
      background: #0f0f0f;
      color: #f0f0f0;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      padding: 20px;
    }
    .card {
      background: #1c1c1e;
      border-radius: 16px;
      padding: 30px;
      max-width: 380px;
      width: 100%;
      box-shadow: 0 8px 30px rgba(0,0,0,0.5);
    }
    .logo {
      font-size: 28px;
      font-weight: bold;
      color: #f5a623;
      text-align: center;
      margin-bottom: 6px;
    }
    .subtitle {
      text-align: center;
      color: #888;
      font-size: 13px;
      margin-bottom: 24px;
    }
    .badge {
      background: #ff3b30;
      color: white;
      border-radius: 8px;
      padding: 10px 14px;
      font-size: 13px;
      margin-bottom: 20px;
      text-align: center;
    }
    .row {
      display: flex;
      justify-content: space-between;
      padding: 10px 0;
      border-bottom: 1px solid #2c2c2e;
      font-size: 14px;
    }
    .row .label { color: #888; }
    .row .value { font-weight: bold; color: #f0f0f0; }
    .total-row {
      display: flex;
      justify-content: space-between;
      padding: 16px 0 10px;
      font-size: 17px;
      font-weight: bold;
    }
    .total-row .value { color: #f5a623; }
    .pay-btn {
      width: 100%;
      padding: 16px;
      background: #f5a623;
      color: #000;
      border: none;
      border-radius: 12px;
      font-size: 17px;
      font-weight: bold;
      cursor: pointer;
      margin-top: 22px;
      transition: background 0.2s;
    }
    .pay-btn:hover { background: #e6941a; }
    .note {
      font-size: 11px;
      color: #555;
      text-align: center;
      margin-top: 14px;
    }
  </style>
</head>
<body>
<div class="card">
  <div class="logo">&#128663; ZOOP</div>
  <div class="subtitle">Toll Payment Portal</div>
  <div class="badge">&#9888;&#65039; Zero Balance &mdash; Entry Blocked</div>
  <div class="row">
    <span class="label">Vehicle</span>
    <span class="value">)rawhtml";
  html += pendingVehicleNo;
  html += R"rawhtml(</span>
  </div>
  <div class="row">
    <span class="label">Toll Charge</span>
    <span class="value">&#8377;65</span>
  </div>
  <div class="row">
    <span class="label">Zero Balance Penalty</span>
    <span class="value">&#8377;65</span>
  </div>
  <div class="total-row">
    <span class="label">Total Payable</span>
    <span class="value">&#8377;)rawhtml";
  html += String((int)pendingAmount);
  html += R"rawhtml(</span>
  </div>
  <form method="POST" action="/confirm">
    <input type="hidden" name="uid" value=")rawhtml";
  html += pendingUID;
  html += R"rawhtml(">
    <button class="pay-btn" type="submit">&#128179; PAY &amp; ENTER</button>
  </form>
  <div class="note">Simulation: clicking confirms payment and opens barrier.</div>
</div>
</body>
</html>
)rawhtml";

  server.send(200, "text/html", html);
}

void handleConfirm()
{
  if (!paymentPending) {
    server.send(400, "text/plain", "No pending payment.");
    return;
  }

  String uid = server.arg("uid");
  if (uid != pendingUID) {
    server.send(400, "text/plain", "UID mismatch.");
    return;
  }

  paymentPending = false;
  clearLEDs();
  digitalWrite(LED_GREEN, HIGH);
  buzzerBeep(1, 200);
  oledStatus("  ZOOP PAYMENT", "  CONFIRMED!",
             "ENTRY GRANTED", pendingVehicleNo.c_str());
  openBarrier();

  char payload[200];
  snprintf(payload, sizeof(payload),
    "{\"vehicle\":\"%s\",\"status\":\"ENTRY_GRANTED_POST_PAYMENT\",\"amount\":%.0f}",
    pendingVehicleNo.c_str(), pendingAmount);
  publishMQTT(TOPIC_STATUS, payload);

  char topic[64];
  snprintf(topic, sizeof(topic), "zoop/%s", pendingVehicleNo.c_str());
  char userPayload[256];
  snprintf(userPayload, sizeof(userPayload),
    "{\"status\":\"PAYMENT_CONFIRMED\",\"vehicle\":\"%s\","
    "\"amount\":%.0f,\"message\":\"Payment successful. Entry granted with penalty waived.\"}",
    pendingVehicleNo.c_str(), pendingAmount);
  publishMQTT(topic, userPayload);

  delay(BARRIER_OPEN_MS);
  closeBarrier();
  clearLEDs();
  oledIdle();
  isProcessing = false;

  server.sendHeader("Location", "/thankyou", true);
  server.send(302, "text/plain", "");
}

void handleThankYou()
{
  String html = R"rawhtml(
<!DOCTYPE html><html><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ZOOP</title>
<style>
body{background:#0f0f0f;color:#f0f0f0;font-family:Arial,sans-serif;
  display:flex;justify-content:center;align-items:center;min-height:100vh;}
.box{text-align:center;padding:40px;background:#1c1c1e;border-radius:16px;}
.icon{font-size:60px;margin-bottom:16px;}
h2{color:#34c759;margin-bottom:8px;}
p{color:#888;font-size:14px;}
</style></head><body>
<div class="box">
  <div class="icon">&#9989;</div>
  <h2>Entry Granted!</h2>
  <p>Barrier is now open. Safe travels.</p>
  <p style="margin-top:20px;color:#f5a623;font-size:20px;font-weight:bold;">&#128663; ZOOP</p>
</div></body></html>
)rawhtml";
  server.send(200, "text/html", html);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\n=== ZOOP Initializing ===");

  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  clearLEDs();
  digitalWrite(BUZZER_PIN, LOW);

  barrier.attach(SERVO_PIN);
  closeBarrier();

  Wire.begin(21, 22);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println("OLED init failed!");
    while (true);
  }
  oled.clearDisplay();
  oled.setTextColor(WHITE);

  SPI.begin();
  rfid.PCD_Init();
  Serial.println("MFRC522 ready.");

  connectWiFi();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  connectMQTT();

  server.on("/pay",      HTTP_GET,  handlePayPage);
  server.on("/confirm",  HTTP_POST, handleConfirm);
  server.on("/thankyou", HTTP_GET,  handleThankYou);
  server.begin();
  Serial.println("Web server started.");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  oledIdle();
  Serial.println("=== ZOOP Ready ===\n");
}

void loop()
{
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();
  server.handleClient();
  handleRFID();
}

void handleRFID()
{
  if (isProcessing) return;
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial())   return;

  isProcessing = true;

  String uid = uidToString(rfid.uid);
  Serial.println("Tag UID: " + uid);

  Vehicle *v = findVehicle(uid);

  if (v == nullptr) {
    clearLEDs();
    digitalWrite(LED_RED, HIGH);
    buzzerBeep(3, 100);
    oledStatus("  UNREGISTERED", "  VEHICLE", uid.c_str(), "Contact ZOOP");
    delay(2500);
    clearLEDs();
    oledIdle();
    isProcessing = false;
  }
  else if (v->balance >= TOLL_AMOUNT) {
    processSufficient(*v);
    isProcessing = false;
  }
  else {
    processZeroBalance(*v);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void processSufficient(Vehicle &v)
{
  v.balance -= TOLL_AMOUNT;

  clearLEDs();
  digitalWrite(LED_GREEN, HIGH);
  buzzerBeep(1, 150);

  char line2[32], line3[32], line4[32];
  snprintf(line2, sizeof(line2), "Deducted:  Rs.%.0f", TOLL_AMOUNT);
  snprintf(line3, sizeof(line3), "Balance:   Rs.%.0f", v.balance);
  snprintf(line4, sizeof(line4), "%s", v.vehicleNo);
  oledStatus("  ENTRY GRANTED", line2, line3, line4);

  openBarrier();

  char payload[200];
  snprintf(payload, sizeof(payload),
    "{\"vehicle\":\"%s\",\"status\":\"ENTRY_GRANTED\",\"deducted\":%.0f,\"balance\":%.0f}",
    v.vehicleNo, TOLL_AMOUNT, v.balance);
  publishMQTT(TOPIC_STATUS, payload);

  char topic[64];
  snprintf(topic, sizeof(topic), "zoop/%s", v.vehicleNo);
  char userPayload[256];
  snprintf(userPayload, sizeof(userPayload),
    "{\"status\":\"TOLL_DEDUCTED\",\"vehicle\":\"%s\","
    "\"deducted\":%.0f,\"balance\":%.0f,\"message\":\"Toll paid successfully. Entry granted.\"}",
    v.vehicleNo, TOLL_AMOUNT, v.balance);
  publishMQTT(topic, userPayload);

  if (v.balance <= LOW_BAL_THRESHOLD && v.balance > 0) {
    delay(1500);
    processLowBalanceAlert(v);
  }

  delay(BARRIER_OPEN_MS);
  closeBarrier();
  clearLEDs();
  oledIdle();
}

void processLowBalanceAlert(Vehicle &v)
{
  clearLEDs();
  digitalWrite(LED_YELLOW, HIGH);
  buzzerBeep(3, 200);

  char line2[32];
  snprintf(line2, sizeof(line2), "Balance: Rs.%.0f", v.balance);
  oledStatus("  LOW BALANCE", line2, "MQTT Alert Sent", v.vehicleNo);

  char topic[64];
  snprintf(topic, sizeof(topic), "zoop/%s", v.vehicleNo);

  char payload[256];
  snprintf(payload, sizeof(payload),
    "{\"status\":\"LOW_BALANCE\",\"vehicle\":\"%s\","
    "\"balance\":%.0f,\"message\":\"Recharge your ZOOP wallet soon!\"}",
    v.vehicleNo, v.balance);

  publishMQTT(topic, payload);
  Serial.printf("MQTT Alert sent to topic: %s\n", topic);

  delay(2000);
  clearLEDs();
}

void processZeroBalance(Vehicle &v)
{
  clearLEDs();
  digitalWrite(LED_RED, HIGH);
  buzzerBeep(2, 500);

  pendingUID       = String(v.uid);
  pendingVehicleNo = String(v.vehicleNo);
  pendingAmount    = TOLL_AMOUNT * PENALTY_FACTOR;
  paymentPending   = true;

  char url[128];
  snprintf(url, sizeof(url), "http://100.64.0.2:8888/pay?uid=%s", v.uid);
  Serial.println("Payment URL: " + String(url));

  oledQR(url);

  char topic[64];
  snprintf(topic, sizeof(topic), "zoop/%s", v.vehicleNo);

  char payload[300];
  snprintf(payload, sizeof(payload),
    "{\"status\":\"ZERO_BALANCE\",\"vehicle\":\"%s\","
    "\"payable\":%.0f,\"message\":\"Scan QR at toll to pay and enter.\"}",
    v.vehicleNo, pendingAmount);

  publishMQTT(topic, payload);
}

void openBarrier()
{
  barrier.write(90);
  Serial.println("Barrier OPEN");
}

void closeBarrier()
{
  barrier.write(0);
  Serial.println("Barrier CLOSED");
}

void clearLEDs()
{
  digitalWrite(LED_GREEN,  LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED,    LOW);
}

void buzzerBeep(int times, int durationMs)
{
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(durationMs);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < times - 1) delay(100);
  }
}

void oledIdle()
{
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(20, 10);
  oled.println("  ZOOP");
  oled.setTextSize(1);
  oled.setCursor(15, 38);
  oled.println("Place Your RFID Tag");
  oled.setCursor(28, 52);
  oled.println("to Enter Toll");
  oled.display();
}

void oledStatus(const char *line1, const char *line2,
                const char *line3, const char *line4)
{
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0,  0); oled.println(line1);
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 16); oled.println(line2);
  oled.setCursor(0, 30); oled.println(line3);
  oled.setCursor(0, 44); oled.println(line4);
  oled.display();
}

void oledQR(const char *url)
{
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("SCAN TO PAY & ENTER");
  oled.drawLine(0, 10, 127, 10, WHITE);

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, url);

  int qrSize   = qrcode.size;
  int scale    = 52 / qrSize;
  if (scale < 1) scale = 1;

  int offsetX  = (128 - qrSize * scale) / 2;
  int offsetY  = 12 + (52 - qrSize * scale) / 2;

  for (int y = 0; y < qrSize; y++) {
    for (int x = 0; x < qrSize; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        for (int dy = 0; dy < scale; dy++)
          for (int dx = 0; dx < scale; dx++)
            oled.drawPixel(offsetX + x*scale + dx,
                           offsetY + y*scale + dy, WHITE);
      }
    }
  }

  oled.display();
  Serial.println("QR Code displayed on OLED.");
}

void connectWiFi()
{
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
  else
    Serial.println("\nWiFi failed.");
}

void connectMQTT()
{
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    Serial.print("Connecting to MQTT broker...");
    if (mqtt.connect(MQTT_CLIENT_ID)) {
      Serial.println(" connected.");
      return;
    } else {
      Serial.printf(" failed (rc=%d). Retry in 2s.\n", mqtt.state());
      delay(2000);
      attempts++;
    }
  }
  if (!mqtt.connected()) {
    Serial.println("MQTT connection failed - continuing without MQTT");
  }
}

void publishMQTT(const char *topic, const char *payload)
{
  if (!mqtt.connected()) {
    Serial.println("MQTT not connected - skipping publish");
    return;
  }
  if (mqtt.publish(topic, payload))
    Serial.printf("MQTT Published -> [%s] %s\n", topic, payload);
  else
    Serial.println("MQTT publish failed.");
}

String uidToString(MFRC522::Uid &uid)
{
  String s = "";
  for (byte i = 0; i < uid.size; i++) {
    if (uid.uidByte[i] < 0x10) s += "0";
    s += String(uid.uidByte[i], HEX);
  }
  s.toUpperCase();
  return s;
}

Vehicle *findVehicle(const String &uid)
{
  for (int i = 0; i < DB_SIZE; i++)
    if (String(db[i].uid) == uid) return &db[i];
  return nullptr;
}
