// VERIFY WORKING BEFORE PUSHING TO GITHUB

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#define RX_PIN 20
#define TX_PIN 21

/******************** CONFIGURATION *************************/
// WiFi credentials
const char* ssid = "TAMU_IoT"; // "TAMU_IoT"
const char* password = "";     // ""

// Azure IoT Hub info
const char* iothubHost = "mySpotHub.azure-devices.net";
const char* deviceId = "esp32-c6-gateway";
const char* sasToken = "SharedAccessSignature sr=mySpotHub.azure-devices.net%2Fdevices%2Fesp32-c6-gateway&sig=bkrXuYsxuSW0XB3vHBzfwkZ1iR0XzcUfYuUDy%2F68tMc%3D&se=1762820310";

/******************** WIFI + AZURE ********************/
WiFiClientSecure espClient;
PubSubClient client(espClient);

String value; // value to be send via telemetry

void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);
}

void connectToAzure() {
  
  client.setBufferSize(512);
  client.setServer(iothubHost, 8883);
  espClient.setInsecure(); // for testing; replace with root CA in production
  String username = String(iothubHost) + "/" + deviceId + "/?api-version=2020-09-30";
  String clientId = deviceId;

  while (!client.connected()) {
      Serial.println("Connecting to Azure IoT...");
      client.connect(clientId.c_str(), username.c_str(), sasToken);
      delay(1000);
  }
}

void sendTelemetry() {
    String topic = "devices/" + String(deviceId) + "/messages/events/";
    String payload = "{\"Distance\": " + value + ", \"status\": \"ok\"}"; // redundant conversion of string to float to string
    client.publish(topic.c_str(), payload.c_str());
}

/******************** SETUP ********************/
void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

    // Connect WiFi & Azure
    Serial.println("Connecting to WiFi...");
    connectToWiFi();
    connectToAzure();
    Serial.println("Connected!");
}

/******************** LOOP ********************/
void loop() {
  if (Serial1.available()) {
    value = Serial1.readStringUntil('\n');  // Read until newline
    Serial.print("Received struct: ");
    Serial.println(value);
  }
  static uint32_t last_print = 0;
  if (millis() - last_print > 10000) {
      last_print = millis();
      sendTelemetry();
  }

  if (!client.connected()) connectToAzure();
  client.loop();
}
