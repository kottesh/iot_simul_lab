#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// WiFi Configuration
const char *WIFI_SSID = "Wokwi-GUEST";
const char *WIFI_PASSWORD = "";

// MQTT Configuration
const char *MQTT_BROKER = "100.64.0.1";
const int MQTT_PORT = 1883;
const char *MQTT_CLIENT = "ESP32-GasDetector";

const char *TOPIC_LEVEL = "home/gas/level";
const char *TOPIC_STATUS = "home/gas/status";
const char *TOPIC_ALERT = "home/gas/alert";
const char *TOPIC_VALVE = "home/gas/valve";
const char *TOPIC_FAN = "home/gas/fan";
const char *TOPIC_HEALTH = "home/gas/system/health";
const char *TOPIC_CONTROL = "home/gas/control";

// Pin Definitions
#define MQ2_ANALOG_PIN 34
#define MQ2_DIGITAL_PIN 35
#define BUZZER_PIN 26
#define LED_RED_PIN 27
#define LED_GREEN_PIN 25
#define RELAY_PIN 32
#define SERVO_PIN 33

// OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Thresholds
int THRESHOLD_WARNING = 300;
int THRESHOLD_DANGER = 600;

// System State
enum GasState
{
  SAFE,
  WARNING_LEVEL,
  DANGER_LEVEL
};
GasState currentState = SAFE;
GasState previousState = SAFE;

bool valveOpen = true;
bool buzzerSilenced = false;
bool autoRecovery = false;
int servoAngle = 0;

bool testMode = false;
unsigned long testModeStart = 0;
unsigned long lastSensorRead = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastBlink = 0;
bool blinkState = false;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
Servo ventServo;

// Function Prototypes
void connectWiFi();
void connectMQTT();
void mqttCallback(char *, byte *, unsigned int);
void handleGasLevel(int ppm);
void handleCommand(const String &cmd, const JsonDocument &doc);
void setValve(bool open, const char *reason);
void setFan(int angle, const char *reason, int ppm);
void systemReset();
void systemStatus(int ppm);
void updateOLED(int ppm);

void publishLevel(int ppm);
void publishStatus(int ppm);
void publishAlert(int ppm);
void publishValve(const char *reason);
void publishFan(const char *reason, int ppm);
void publishHealth();

void setup()
{
  Serial.begin(115200);
  Serial.println("\n=== Gas Detector Boot ===");

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(MQ2_DIGITAL_PIN, INPUT);

  // Initialize to safe state
  digitalWrite(RELAY_PIN, LOW);
  noTone(BUZZER_PIN);
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, HIGH);

  ventServo.attach(SERVO_PIN);
  ventServo.write(0);

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("OLED init failed!");
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Smart Gas Detector");
  display.println("Booting...");
  display.display();
  delay(1000);

  connectWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  connectMQTT();

  Serial.println("System ready.");
}

void loop()
{
  if (!mqtt.connected())
    connectMQTT();
  mqtt.loop();

  unsigned long now = millis();

  // Test mode auto-expire after 10 seconds
  if (testMode && (now - testModeStart >= 10000))
  {
    testMode = false;
    Serial.println("Test mode expired — resetting.");
    systemReset();
  }

  // Read sensor every 2 seconds
  if (now - lastSensorRead >= 2000)
  {
    lastSensorRead = now;

    // Digital pin: LOW = gas detected, HIGH = no gas
    int digitalValue = digitalRead(MQ2_DIGITAL_PIN);
    int ppm = (digitalValue == LOW) ? 1000 : 0;

    if (!testMode)
      handleGasLevel(ppm);

    publishLevel(ppm);
    updateOLED(ppm);
  }

  // Heartbeat every 30 seconds
  if (now - lastHeartbeat >= 30000)
  {
    lastHeartbeat = now;
    publishHealth();
  }

  // Blink LEDs and buzzer in WARNING state
  if (currentState == WARNING_LEVEL && (now - lastBlink >= 500))
  {
    lastBlink = now;
    blinkState = !blinkState;
    digitalWrite(LED_RED_PIN, blinkState);
    digitalWrite(LED_GREEN_PIN, !blinkState);
    if (!buzzerSilenced)
    {
      if (blinkState)
        tone(BUZZER_PIN, 1000);
      else
        noTone(BUZZER_PIN);
    }
  }
}

void connectWiFi()
{
  Serial.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK: " + WiFi.localIP().toString());
}

void connectMQTT()
{
  while (!mqtt.connected())
  {
    Serial.print("Connecting MQTT @ ");
    Serial.print(MQTT_BROKER);
    Serial.print("...");
    if (mqtt.connect(MQTT_CLIENT))
    {
      Serial.println("connected.");
      mqtt.subscribe(TOPIC_CONTROL);
      publishHealth();
    }
    else
    {
      Serial.print("failed (rc=");
      Serial.print(mqtt.state());
      Serial.println("). Retry in 3 s...");
      delay(3000);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  String msg;
  for (unsigned int i = 0; i < length; i++)
    msg += (char)payload[i];
  Serial.println("[MQTT IN] " + String(topic) + " → " + msg);

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, msg))
  {
    Serial.println("JSON parse error — ignoring.");
    return;
  }
  handleCommand(doc["command"].as<String>(), doc);
}

void handleCommand(const String &cmd, const JsonDocument &doc)
{
  int ppm = analogRead(MQ2_ANALOG_PIN);
  Serial.println("[CMD] " + cmd);

  if (cmd == "VALVE_OPEN")
    setValve(true, "MANUAL");
  else if (cmd == "VALVE_CLOSE")
    setValve(false, "MANUAL");
  else if (cmd == "FAN_OFF")
    setFan(0, "MANUAL", ppm);
  else if (cmd == "FAN_PARTIAL")
    setFan(90, "MANUAL", ppm);
  else if (cmd == "FAN_ON")
    setFan(180, "MANUAL", ppm);
  else if (cmd == "BUZZER_SILENCE")
  {
    buzzerSilenced = true;
    noTone(BUZZER_PIN);
    Serial.println("Buzzer silenced.");
  }
  else if (cmd == "BUZZER_ENABLE")
  {
    buzzerSilenced = false;
    Serial.println("Buzzer re-enabled.");
  }
  else if (cmd == "BUZZER_TEST")
  {
    if (!buzzerSilenced)
    {
      tone(BUZZER_PIN, 2000);
      delay(2000);
      noTone(BUZZER_PIN);
    }
  }
  else if (cmd == "LED_TEST")
  {
    for (int i = 0; i < 3; i++)
    {
      digitalWrite(LED_RED_PIN, HIGH);
      digitalWrite(LED_GREEN_PIN, HIGH);
      delay(400);
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(LED_GREEN_PIN, LOW);
      delay(400);
    }
    digitalWrite(LED_GREEN_PIN, currentState == SAFE ? HIGH : LOW);
    digitalWrite(LED_RED_PIN, currentState == DANGER_LEVEL ? HIGH : LOW);
  }
  else if (cmd == "SYSTEM_RESET")
    systemReset();
  else if (cmd == "SYSTEM_STATUS")
    systemStatus(ppm);
  else if (cmd == "SYSTEM_REBOOT")
  {
    Serial.println("Rebooting...");
    delay(500);
    ESP.restart();
  }
  else if (cmd == "TEST_ALARM")
  {
    testMode = true;
    testModeStart = millis();
    currentState = DANGER_LEVEL;
    setValve(false, "TEST");
    setFan(180, "TEST", 999);
    if (!buzzerSilenced)
      tone(BUZZER_PIN, 2000);
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(LED_GREEN_PIN, LOW);
    publishAlert(999);
    Serial.println("TEST_ALARM → 10 s simulation.");
  }
  else if (cmd == "TEST_WARNING")
  {
    testMode = true;
    testModeStart = millis();
    currentState = WARNING_LEVEL;
    setFan(90, "TEST", 450);
    Serial.println("TEST_WARNING → 10 s simulation.");
  }
  else if (cmd == "SENSOR_CALIBRATE")
  {
    long sum = 0;
    for (int i = 0; i < 20; i++)
    {
      sum += analogRead(MQ2_ANALOG_PIN);
      delay(50);
    }
    Serial.println("Calibration baseline ADC: " + String(sum / 20));
  }
  else if (cmd == "HEALTH_PING")
    publishHealth();
  else if (cmd == "SET_THRESHOLD")
  {
    if (doc.containsKey("warning"))
      THRESHOLD_WARNING = doc["warning"].as<int>();
    if (doc.containsKey("danger"))
      THRESHOLD_DANGER = doc["danger"].as<int>();
    Serial.println("Thresholds → WARNING:" + String(THRESHOLD_WARNING) +
                   " DANGER:" + String(THRESHOLD_DANGER));
  }
  else if (cmd == "AUTO_RECOVERY_ON")
  {
    autoRecovery = true;
    Serial.println("Auto-recovery ON.");
  }
  else if (cmd == "AUTO_RECOVERY_OFF")
  {
    autoRecovery = false;
    Serial.println("Auto-recovery OFF.");
  }
  else
    Serial.println("Unknown command: " + cmd);
}

void handleGasLevel(int ppm)
{
  previousState = currentState;

  if (ppm >= THRESHOLD_DANGER)
    currentState = DANGER_LEVEL;
  else if (ppm >= THRESHOLD_WARNING)
    currentState = WARNING_LEVEL;
  else
    currentState = SAFE;

  switch (currentState)
  {
  case DANGER_LEVEL:
    digitalWrite(LED_RED_PIN, HIGH);
    digitalWrite(LED_GREEN_PIN, LOW);
    if (!buzzerSilenced)
      tone(BUZZER_PIN, 2000);
    setValve(false, "DANGER_AUTO");
    setFan(180, "DANGER_AUTO", ppm);
    if (previousState != DANGER_LEVEL)
      publishAlert(ppm);
    break;

  case WARNING_LEVEL:
    setFan(90, "WARNING_AUTO", ppm);
    break;

  case SAFE:
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, HIGH);
    if (!buzzerSilenced)
      noTone(BUZZER_PIN);
    if (autoRecovery)
      setValve(true, "AUTO_RECOVERY");
    setFan(0, "SAFE", ppm);
    break;
  }

  if (previousState != currentState)
    publishStatus(ppm);
}

void setValve(bool open, const char *reason)
{
  valveOpen = open;
  digitalWrite(RELAY_PIN, open ? LOW : HIGH);
  publishValve(reason);
  Serial.print("Valve → ");
  Serial.println(open ? "OPEN" : "CLOSED");
}

void setFan(int angle, const char *reason, int ppm)
{
  servoAngle = angle;
  ventServo.write(angle);
  publishFan(reason, ppm);
  Serial.print("Fan/Servo → ");
  Serial.print(angle);
  Serial.println("°");
}

void systemReset()
{
  currentState = SAFE;
  buzzerSilenced = false;
  testMode = false;
  blinkState = false;
  noTone(BUZZER_PIN);
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, HIGH);
  setValve(true, "SYSTEM_RESET");
  setFan(0, "SYSTEM_RESET", 0);
  Serial.println("System reset complete.");
}

void systemStatus(int ppm)
{
  publishLevel(ppm);
  publishStatus(ppm);
  publishValve("STATUS_REQUEST");
  publishFan("STATUS_REQUEST", ppm);
  publishHealth();
}

void updateOLED(int ppm)
{
  const char *stateStr =
      currentState == DANGER_LEVEL ? "!! DANGER !!" : currentState == WARNING_LEVEL ? "  WARNING   "
                                                                                    : "    SAFE    ";

  display.clearDisplay();

  display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(10, 1);
  display.print("GAS DETECTOR");

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.print(stateStr);

  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print("Valve:");
  display.print(valveOpen ? "OPEN " : "SHUT ");
  display.print("Fan:");
  display.print(servoAngle == 0 ? "OFF" : servoAngle == 90 ? "MID"
                                                           : "MAX");

  display.setCursor(0, 56);
  display.print("AutoRec:");
  display.print(autoRecovery ? "ON " : "OFF");
  display.print(" Mute:");
  display.print(buzzerSilenced ? "Y" : "N");

  display.display();
}

void publishLevel(int ppm)
{
  StaticJsonDocument<128> doc;
  doc["ppm"] = ppm;
  doc["unit"] = "ppm";
  doc["uptime_s"] = millis() / 1000;
  char buf[128];
  serializeJson(doc, buf);
  mqtt.publish(TOPIC_LEVEL, buf);
}

void publishStatus(int ppm)
{
  const char *s = currentState == DANGER_LEVEL ? "DANGER" : currentState == WARNING_LEVEL ? "WARNING"
                                                                                          : "SAFE";
  StaticJsonDocument<128> doc;
  doc["status"] = s;
  doc["ppm"] = ppm;
  char buf[128];
  serializeJson(doc, buf);
  mqtt.publish(TOPIC_STATUS, buf);
}

void publishAlert(int ppm)
{
  StaticJsonDocument<256> doc;
  doc["alert"] = true;
  doc["level"] = "DANGER";
  doc["ppm"] = ppm;
  doc["message"] = "Gas leakage detected! Evacuate immediately.";
  char buf[256];
  serializeJson(doc, buf);
  mqtt.publish(TOPIC_ALERT, buf, true);
}

void publishValve(const char *reason)
{
  StaticJsonDocument<128> doc;
  doc["state"] = valveOpen ? "OPEN" : "CLOSED";
  doc["reason"] = reason;
  char buf[128];
  serializeJson(doc, buf);
  mqtt.publish(TOPIC_VALVE, buf);
}

void publishFan(const char *reason, int ppm)
{
  const char *s = servoAngle == 0 ? "OFF" : servoAngle == 90 ? "PARTIAL"
                                                             : "ON";
  StaticJsonDocument<128> doc;
  doc["state"] = s;
  doc["angle"] = servoAngle;
  doc["reason"] = reason;
  doc["ppm"] = ppm;
  char buf[128];
  serializeJson(doc, buf);
  mqtt.publish(TOPIC_FAN, buf);
}

void publishHealth()
{
  StaticJsonDocument<256> doc;
  doc["device"] = MQTT_CLIENT;
  doc["status"] = "online";
  doc["uptime_s"] = millis() / 1000;
  doc["ip"] = WiFi.localIP().toString();
  doc["valve"] = valveOpen ? "OPEN" : "CLOSED";
  doc["fan_angle"] = servoAngle;
  doc["auto_recovery"] = autoRecovery;
  doc["buzzer_silenced"] = buzzerSilenced;
  doc["threshold_warning"] = THRESHOLD_WARNING;
  doc["threshold_danger"] = THRESHOLD_DANGER;
  char buf[256];
  serializeJson(doc, buf);
  mqtt.publish(TOPIC_HEALTH, buf);
}
