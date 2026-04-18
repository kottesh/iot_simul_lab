#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <IRremote.h>

#define IR_RECV_PIN 15
#define STEP_PIN 33
#define DIR_PIN 32

#define IR_BUTTON_POWER 0xFFA25D
#define IR_BUTTON_PLUS 0xFF02FD
#define IR_BUTTON_MINUS 0xFF9867
#define IR_BUTTON_1 0xFF30CF
#define IR_BUTTON_2 0xFF18E7
#define IR_BUTTON_3 0xFF7A85

const char *ssid = "Wokwi-GUEST";
const char *password = "";
const char *mqtt_server = "100.64.0.1";
const int mqtt_port = 1883;
const char *mqtt_topic = "/fan/control";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

int currentSpeed = 0;
const int SPEED_STEP = 1;
const int MIN_SPEED = 0;
const int MAX_SPEED = 10;
bool fanOn = false;

unsigned long lastStepTime = 0;
int stepDelay = 2000;

void setupWiFi();
void setupMQTT();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void reconnectMQTT();
void handleIRRemote();
void publishSpeedCommand(bool speedUp);
void publishPowerCommand(bool powerOn);
void updateFanSpeed(int newSpeed);
void printStatus();
void runStepper();

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("   ESP32 Fan Control with IR & MQTT");
    Serial.println("========================================\n");

    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    digitalWrite(STEP_PIN, LOW);
    digitalWrite(DIR_PIN, HIGH);
    Serial.println("[STEPPER] Initialized - STEP: GPIO " + String(STEP_PIN) + ", DIR: GPIO " + String(DIR_PIN));

    IrReceiver.begin(IR_RECV_PIN, ENABLE_LED_FEEDBACK);
    Serial.println("[IR] Receiver initialized on GPIO " + String(IR_RECV_PIN));

    setupWiFi();
    setupMQTT();

    Serial.println("\n[READY] System initialized!");
    Serial.println("Use IR remote to control fan speed");
    Serial.println("  - Press + to increase speed");
    Serial.println("  - Press - to decrease speed");
    Serial.println("  - Press POWER to toggle fan\n");
}

void loop()
{
    if (!mqttClient.connected())
    {
        reconnectMQTT();
    }
    mqttClient.loop();
    handleIRRemote();
    runStepper();
}

void runStepper()
{
    if (fanOn && stepDelay > 0)
    {
        unsigned long currentTime = micros();
        if (currentTime - lastStepTime >= stepDelay)
        {
            digitalWrite(STEP_PIN, HIGH);
            delayMicroseconds(1000);
            digitalWrite(STEP_PIN, LOW);
            lastStepTime = currentTime;
        }
    }
}

void setupWiFi()
{
    Serial.print("[WIFI] Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\n[WIFI] Connected!");
        Serial.print("[WIFI] IP Address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\n[WIFI] Connection failed! Continuing without WiFi...");
    }
}

void setupMQTT()
{
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);
    Serial.println("[MQTT] Configured for " + String(mqtt_server));
}

void reconnectMQTT()
{
    int retries = 0;
    while (!mqttClient.connected() && retries < 3)
    {
        Serial.print("[MQTT] Attempting connection...");

        String clientId = "ESP32FanControl-" + String(random(0xffff), HEX);

        if (mqttClient.connect(clientId.c_str()))
        {
            Serial.println(" connected!");
            mqttClient.subscribe(mqtt_topic);
            Serial.println("[MQTT] Subscribed to: " + String(mqtt_topic));
        }
        else
        {
            Serial.print(" failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" retrying in 2 seconds...");
            delay(2000);
            retries++;
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("\n[MQTT] Message received on topic: ");
    Serial.println(topic);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error)
    {
        Serial.print("[MQTT] JSON parsing failed: ");
        Serial.println(error.c_str());
        return;
    }

    Serial.print("[MQTT] Payload: ");
    serializeJson(doc, Serial);
    Serial.println();

    if (doc.containsKey("speedPlus") && doc["speedPlus"].as<bool>())
    {
        Serial.println("[MQTT] Processing speedPlus command");
        int newSpeed = currentSpeed + SPEED_STEP;
        if (newSpeed > MAX_SPEED)
            newSpeed = MAX_SPEED;
        updateFanSpeed(newSpeed);
        fanOn = (currentSpeed > 0);
    }

    if (doc.containsKey("speedDown") && doc["speedDown"].as<bool>())
    {
        Serial.println("[MQTT] Processing speedDown command");
        int newSpeed = currentSpeed - SPEED_STEP;
        if (newSpeed < MIN_SPEED)
            newSpeed = MIN_SPEED;
        updateFanSpeed(newSpeed);
        fanOn = (currentSpeed > 0);
    }

    if (doc.containsKey("power"))
    {
        bool powerState = doc["power"].as<bool>();
        Serial.println("[MQTT] Processing power command: " + String(powerState ? "ON" : "OFF"));
        if (powerState)
        {
            updateFanSpeed(5);
            fanOn = true;
        }
        else
        {
            updateFanSpeed(0);
            fanOn = false;
        }
    }

    printStatus();
}

void handleIRRemote()
{
    if (IrReceiver.decode())
    {
        uint16_t command = IrReceiver.decodedIRData.command;
        uint16_t address = IrReceiver.decodedIRData.address;

        Serial.print("\n[IR] Received - Address: 0x");
        Serial.print(address, HEX);
        Serial.print(", Command: 0x");
        Serial.println(command, HEX);

        if (command == 2)
        {
            Serial.println("[IR] Speed UP pressed");
            publishSpeedCommand(true);
        }
        else if (command == 152)
        {
            Serial.println("[IR] Speed DOWN pressed");
            publishSpeedCommand(false);
        }
        else if (command == 162)
        {
            Serial.println("[IR] POWER pressed - Toggle fan");
            publishPowerCommand(!fanOn);
        }
        else
        {
            Serial.println("[IR] Unknown button");
        }

        IrReceiver.resume();
    }
}

void publishSpeedCommand(bool speedUp)
{
    if (!mqttClient.connected())
    {
        Serial.println("[MQTT] Not connected, cannot publish");
        return;
    }

    StaticJsonDocument<128> doc;

    if (speedUp)
    {
        doc["speedPlus"] = true;
        doc["speedDown"] = false;
    }
    else
    {
        doc["speedPlus"] = false;
        doc["speedDown"] = true;
    }

    char jsonBuffer[128];
    serializeJson(doc, jsonBuffer);

    Serial.print("[MQTT] Publishing to ");
    Serial.print(mqtt_topic);
    Serial.print(": ");
    Serial.println(jsonBuffer);

    mqttClient.publish(mqtt_topic, jsonBuffer);
}

void publishPowerCommand(bool powerOn)
{
    if (!mqttClient.connected())
    {
        Serial.println("[MQTT] Not connected, cannot publish");
        return;
    }

    StaticJsonDocument<128> doc;
    doc["power"] = powerOn;

    char jsonBuffer[128];
    serializeJson(doc, jsonBuffer);

    Serial.print("[MQTT] Publishing to ");
    Serial.print(mqtt_topic);
    Serial.print(": ");
    Serial.println(jsonBuffer);

    mqttClient.publish(mqtt_topic, jsonBuffer);
}

void updateFanSpeed(int newSpeed)
{
    newSpeed = constrain(newSpeed, MIN_SPEED, MAX_SPEED);

    if (newSpeed != currentSpeed)
    {
        currentSpeed = newSpeed;

        if (currentSpeed > 0)
        {
            stepDelay = 5000 - (currentSpeed * 400);
            if (stepDelay < 500) stepDelay = 500;
        }
        else
        {
            stepDelay = 0;
        }

        Serial.print("[STEPPER] Oscillation speed level: ");
        Serial.print(currentSpeed);
        Serial.print(" (delay: ");
        Serial.print(stepDelay);
        Serial.println("us)");
    }
}

void printStatus()
{
    int speedPercent = (currentSpeed * 100) / MAX_SPEED;
    Serial.println("\n--- Fan Status ---");
    Serial.print("  State: ");
    Serial.println(fanOn ? "ON" : "OFF");
    Serial.print("  Speed: ");
    Serial.print(currentSpeed);
    Serial.print("/");
    Serial.print(MAX_SPEED);
    Serial.print(" (");
    Serial.print(speedPercent);
    Serial.println("%)");
    Serial.println("------------------\n");
}
