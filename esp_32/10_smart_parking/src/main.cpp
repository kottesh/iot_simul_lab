#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char *WIFI_SSID = "Wokwi-GUEST"; // change for real hardware
const char *WIFI_PASS = "";
const char *BROKER = "100.64.0.2";
const int BROKER_PORT = 1883;

const char *TOPIC_STATUS = "smartparking/status";
const char *TOPIC_CMD = "smartparking/cmd";
const char *TOPIC_ACK = "smartparking/ack";

#define NUM_SLOTS 6
#define THRESHOLD_CM 20
#define RESERVE_TIMEOUT_MS (30 * 1000)//(5UL * 60000UL)
#define STATUS_INTERVAL_MS 1000

// Slot 1→6 TRIG / ECHO pin pairs
const int TRIG_PINS[NUM_SLOTS] = {12, 13, 25, 26, 27, 33};
const int ECHO_PINS[NUM_SLOTS] = {14, 35, 34, 32, 4, 17};

struct Slot
{
  bool occupied, reserved;
  float dist;
  unsigned long resAt;
};
Slot slots[NUM_SLOTS];

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
unsigned long lastStatus = 0;

// --- Sensor ---

float measureDist(int trig, int echo)
{
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 30000);
  return (dur == 0) ? 999.0f : dur * 0.0343f / 2.0f;
}

void updateSlots()
{
  for (int i = 0; i < NUM_SLOTS; i++)
  {
    float d = measureDist(TRIG_PINS[i], ECHO_PINS[i]);
    slots[i].dist = d;
    if (d < THRESHOLD_CM)
    {
      slots[i].occupied = true;
      slots[i].reserved = false; // car arrived, consume reservation
    }
    else
    {
      slots[i].occupied = false;
      if (slots[i].reserved &&
          millis() - slots[i].resAt > RESERVE_TIMEOUT_MS)
      {
        slots[i].reserved = false; // timed-out reservation
      }
    }
  }
}

// --- MQTT publish ---

void publishStatus()
{
  StaticJsonDocument<512> doc;
  doc["total"] = NUM_SLOTS;
  int freeCount = 0;
  JsonArray arr = doc.createNestedArray("slots");
  for (int i = 0; i < NUM_SLOTS; i++)
  {
    bool avail = !slots[i].occupied && !slots[i].reserved;
    if (avail)
      freeCount++;
    JsonObject s = arr.createNestedObject();
    s["id"] = i + 1;
    s["occupied"] = slots[i].occupied;
    s["reserved"] = slots[i].reserved;
    s["available"] = avail;
    s["dist"] = (int)slots[i].dist;
  }
  doc["free"] = freeCount;
  char buf[512];
  serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(TOPIC_STATUS, buf, true); // retained
}

// --- MQTT callback ---

void onMessage(char *topic, byte *payload, unsigned int len)
{
  StaticJsonDocument<128> req;
  if (deserializeJson(req, payload, len) != DeserializationError::Ok)
    return;

  const char *action = req["action"] | "";
  int sid = req["slot"] | 0;
  if (sid < 1 || sid > NUM_SLOTS)
    return;
  int idx = sid - 1;

  char ack[96];

  if (strcmp(action, "reserve") == 0)
  {
    if (slots[idx].occupied || slots[idx].reserved)
    {
      snprintf(ack, sizeof(ack),
               "{\"slot\":%d,\"success\":false,\"msg\":\"unavailable\"}", sid);
    }
    else
    {
      slots[idx].reserved = true;
      slots[idx].resAt = millis();
      snprintf(ack, sizeof(ack),
               "{\"slot\":%d,\"success\":true,\"msg\":\"reserved\"}", sid);
      publishStatus();
    }
  }
  else if (strcmp(action, "release") == 0)
  {
    if (!slots[idx].reserved)
    {
      snprintf(ack, sizeof(ack),
               "{\"slot\":%d,\"success\":false,\"msg\":\"not reserved\"}", sid);
    }
    else
    {
      slots[idx].reserved = false;
      snprintf(ack, sizeof(ack),
               "{\"slot\":%d,\"success\":true,\"msg\":\"released\"}", sid);
      publishStatus();
    }
  }
  else
  {
    return;
  }

  mqtt.publish(TOPIC_ACK, ack);
}

// --- Connection helpers ---

void connectWiFi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" OK");
}

void connectMQTT()
{
  String cid = "ParkSrv-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  while (!mqtt.connected())
  {
    Serial.print("MQTT...");
    if (mqtt.connect(cid.c_str()))
    {
      mqtt.subscribe(TOPIC_CMD);
      Serial.println("OK");
    }
    else
    {
      delay(2000);
    }
  }
}

// --- Setup / Loop ---

void setup()
{
  Serial.begin(115200);
  for (int i = 0; i < NUM_SLOTS; i++)
  {
    pinMode(TRIG_PINS[i], OUTPUT);
    pinMode(ECHO_PINS[i], INPUT);
    slots[i] = {false, false, 999.0f, 0};
  }
  connectWiFi();
  mqtt.setServer(BROKER, BROKER_PORT);
  mqtt.setBufferSize(1024);
  mqtt.setCallback(onMessage);
  connectMQTT();
  publishStatus();
}

void loop()
{
  if (!mqtt.connected())
    connectMQTT();
  mqtt.loop();
  updateSlots();
  if (millis() - lastStatus >= STATUS_INTERVAL_MS)
  {
    lastStatus = millis();
    publishStatus();
  }
}
