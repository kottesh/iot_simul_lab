#include <PubSubClient.h>
#include <Wifi.h>


const char* MQTT_SERVER = "143.110.186.168";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "wokwi-esp-client";

void onMsg( const char* topic, const byte* payload, const int length) {
    Serial.print("[");
    Serial.print(topic);
    Serial.print("]: ");
    for (int i = 0; i < length; i++) {
        Serial.print(payload[i]);
    }
    Serial.println();
}

WifiClient espClient;

PubSubClient mqtt_client(
    MQTT_SERVER,
    MQTT_PORT,
    onMsg,
    espClient
);

void connectWifi() {
    Wifi.begin();
    while (Wifi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.print("\nWifi Connected!")
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

  mqqt_client.publish("/home/krill", "Krill is here.")
  delay(10);
}
