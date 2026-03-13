#include <PubSubClient.h>
#include <WiFi.h>

const char* MQTT_SERVER = "10.151.23.86";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "wokwi-esp-client";

void onMsg(const char* topic, const byte* payload, unsigned int length) {
    Serial.print("[");
    Serial.print(topic);
    Serial.print("]: ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

WiFiClient espClient;
PubSubClient mqtt_client(
    MQTT_SERVER,
    MQTT_PORT,
    onMsg,
    espClient
);

void connectWifi() {
    WiFi.begin("Wokwi-GUEST", "");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.print("\nWifi Connected!");  // added semicolon
}

void connectMqtt() {
    while (!mqtt_client.connected()) {
        if (mqtt_client.connect(MQTT_CLIENT_ID)) {
            mqtt_client.subscribe("/home/krill");
            Serial.println("MQTT Server connected!");
        } else {
            Serial.print("Connection failed. RC: ");
            Serial.println(mqtt_client.state());
        }
    }
}

void setup() {
    Serial.begin(115200);
    connectWifi();
}

void loop() {
    if (!mqtt_client.connected())
        connectMqtt();
    mqtt_client.loop();
    mqtt_client.publish("/home/krill", "Krill is here.");  // added semicolon
    delay(10);
}
