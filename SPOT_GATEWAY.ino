// VERIFY WORKING BEFORE PUSHING TO GITHUB

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#define RX_PIN 21
#define TX_PIN 20

/******************** CONFIGURATION *************************/
// WiFi credentials
const char* ssid = "TAMU_IoT"; // "TAMU_IoT"
const char* password = "";     // ""

// Azure IoT Hub info
const char* iothubHost = "mySpotHub.azure-devices.net";
const char* deviceId = "esp32-c6-gateway";
const char* sasToken = "SharedAccessSignature sr=mySpotHub.azure-devices.net%2Fdevices%2Fesp32-c6-gateway&sig=y%2BuZ9W7a6Rg6%2Fyd%2Ft3S4kpMgAEK8o19rBK5EIQ4YiK8%3D&se=1764976278";

/******************** WIFI + AZURE ********************/
WiFiClientSecure espClient;
PubSubClient client(espClient);

String value; // Value to be parsed
String v1;
String carDetected = "vacant";

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
  Serial.println("Connected!");
}
// Device IDs: [1, 2, 3]
// Device Values: [value1, value2, value3]
void sendTelemetry() {
    carTest();
    String topic = "devices/" + String(deviceId) + "/messages/events/";
    String payload = "{\"deviceIds\": [1,2,3], \"deviceValues\": [" 
                 + String(v1) + 
                 "], \"status\": \"" + carDetected + "\"}";

    client.publish(topic.c_str(), payload.c_str());
}

void parseValue() {
  v1 = value;
  // Add other distance values
}

void carTest() {
  if (v1 == "0.00") {
    carDetected = "vacant";
  } else {
    carDetected = "ok";
  }
  
}

/******************** SETUP ********************/
void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

    // Connect WiFi & Azure
    Serial.println("Connecting to WiFi...");
    connectToWiFi();
    connectToAzure();
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
      parseValue();
      sendTelemetry();
  }

  if (!client.connected()) connectToAzure();
  client.loop();
}