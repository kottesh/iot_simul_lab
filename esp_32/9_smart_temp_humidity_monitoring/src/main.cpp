#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ─── Config ───────────────────────────────────────────────
#define DHT_PIN          4
#define DHT_TYPE         DHT22
#define PUBLISH_INTERVAL 2000 // ms

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASS ""

#define MQTT_BROKER  "100.64.0.2"
#define MQTT_PORT    1883
#define CLIENT_ID    "ESP32-TempMonitor"

// Topics
#define TOPIC_TEMP    "/home/temp"
#define TOPIC_HUMID   "/home/humid"
#define TOPIC_ALERT   "/home/alerts"
#define TOPIC_STATUS  "/home/status"

// Thresholds
#define TEMP_MAX  35.0
#define HUMID_MAX 80.0

// ─── Globals ──────────────────────────────────────────────
DHT          dht(DHT_PIN, DHT_TYPE);
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

// ─── WiFi ─────────────────────────────────────────────────
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.printf("\n[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
}

// ─── MQTT ─────────────────────────────────────────────────
void onMessage(char* topic, byte* payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
  Serial.printf("[MQTT] Received [%s]: %s\n", topic, msg.c_str());
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("[MQTT] Connecting...");
    if (mqtt.connect(CLIENT_ID)) {
      Serial.println(" connected");
      mqtt.publish(TOPIC_STATUS, "online", true);  // retained
    } else {
      Serial.printf(" failed (rc=%d), retry 5s\n", mqtt.state());
      delay(5000);
    }
  }
}

void ensureConnected() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected())              connectMQTT();
}

// ─── Sensor ───────────────────────────────────────────────
struct Reading {
  float temp;
  float humid;
  bool  valid;
};

Reading readDHT() {
  Reading r;
  r.temp  = dht.readTemperature();
  r.humid = dht.readHumidity();
  r.valid = !isnan(r.temp) && !isnan(r.humid);
  return r;
}

// ─── Publish ──────────────────────────────────────────────
void publishReading(const Reading& r) {
  char tBuf[8], hBuf[8];
  dtostrf(r.temp,  4, 2, tBuf);
  dtostrf(r.humid, 4, 2, hBuf);

  mqtt.publish(TOPIC_TEMP,  tBuf);
  mqtt.publish(TOPIC_HUMID, hBuf);

  Serial.printf("[DATA] Temp: %s°C | Humidity: %s%%\n", tBuf, hBuf);
}

void checkThresholds(const Reading& r) {
  if (r.temp > TEMP_MAX) {
    char msg[40];
    snprintf(msg, sizeof(msg), "HIGH_TEMP:%.2f", r.temp);
    mqtt.publish(TOPIC_ALERT, msg);
    Serial.printf("[ALERT] %s\n", msg);
  }
  if (r.humid > HUMID_MAX) {
    char msg[40];
    snprintf(msg, sizeof(msg), "HIGH_HUMID:%.2f", r.humid);
    mqtt.publish(TOPIC_ALERT, msg);
    Serial.printf("[ALERT] %s\n", msg);
  }
}

// ─── Main ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  dht.begin();
  connectWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMessage);
  connectMQTT();
}

void loop() {
  ensureConnected();
  mqtt.loop();

  static unsigned long lastPublish = 0;
  if (millis() - lastPublish >= PUBLISH_INTERVAL) {
    lastPublish = millis();
    Reading r = readDHT();
    if (r.valid) {
      publishReading(r);
      checkThresholds(r);
    } else {
      Serial.println("[ERR] Sensor read failed");
    }
  }
}
