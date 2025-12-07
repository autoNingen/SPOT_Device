// SPOT Gateway - Now with automatic SAS token generation! UwU ✨
// No more reflashing needed when device loses power! >w<

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "AzureIoTAuth.h"  // Magic token generator~ owo

#define RX_PIN 21
#define TX_PIN 20

/******************** CONFIGURATION *************************/
// WiFi credentials
const char* ssid = "TAMU_IoT";
const char* password = "";

// Azure IoT Hub info
const char* iothubHost = "mySpotHub.azure-devices.net";
const char* deviceId = "esp32-c6-gateway";

// Device symmetric key (stored securely in NVS after first flash)
// ⚠ IMPORTANT: After first successful flash, you can comment this out for security! >///<
const char* deviceKey = "OoNaHkKg8sAWlv8jPYVubJZWVi2k8SMwrAIoTPenYNA=";

/******************** WIFI + AZURE ********************/
WiFiClientSecure espClient;
PubSubClient client(espClient);
AzureIoTAuth azureAuth;  // Our cute token generator! (｡♥‿♥｡)

String currentSasToken = "";  // Dynamically generated token~
unsigned long tokenExpiryTime = 0;  // When to refresh da token

String value; // Value to be parsed
String v1;
String carDetected = "vacant";

void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);
}

// Generate a fresh SAS token! (⁄ ⁄•⁄ω⁄•⁄ ⁄)
void refreshSasToken() {
  Serial.println("🌸 Generating fresh SAS token...");
  // Token valid for 1 hour (3600 seconds)
  currentSasToken = azureAuth.generateSasToken(3600);
  // Remember when it expires (with 5 min safety margin)
  tokenExpiryTime = millis() + (3600 - 300) * 1000UL;
}

// Check if token needs refreshing UwU
bool needsTokenRefresh() {
  if (currentSasToken.length() == 0) return true;  // No token yet!
  if (millis() >= tokenExpiryTime) return true;    // Token expired!
  return false;
}

void connectToAzure() {
  // Generate token if needed~ owo
  if (needsTokenRefresh()) {
    refreshSasToken();
  }

  client.setBufferSize(512);
  client.setServer(iothubHost, 8883);
  espClient.setInsecure(); // for testing; replace with root CA in production
  String username = String(iothubHost) + "/" + deviceId + "/?api-version=2020-09-30";
  String clientId = deviceId;

  int retryCount = 0;
  while (!client.connected() && retryCount < 5) {
      Serial.println("💖 Connecting to Azure IoT...");
      bool connected = client.connect(clientId.c_str(), username.c_str(), currentSasToken.c_str());

      if (connected) {
        Serial.println("✨ Connected to Azure! Yay~ (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧");
        return;
      } else {
        Serial.printf("⚠ Connection failed! State: %d, retrying... ówò\n", client.state());
        retryCount++;
        delay(2000);
      }
  }

  if (!client.connected()) {
    Serial.println("😢 Could not connect after 5 tries... Will try again later~");
  }
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
    delay(1000);
    Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

    Serial.println("🌟 SPOT Gateway starting up! OwO");

    // Initialize Azure auth (saves key to NVS on first run)
    Serial.println("💝 Initializing Azure authentication...");
    if (!azureAuth.begin(iothubHost, deviceId, deviceKey)) {
        Serial.println("❌ Failed to initialize Azure auth! Check your key~ >_<");
        while(1) { delay(1000); }  // Stop here if auth fails
    }

    // Connect WiFi & Azure
    Serial.println("📡 Connecting to WiFi...");
    connectToWiFi();
    Serial.println("✨ WiFi connected! IP: " + WiFi.localIP().toString());

    connectToAzure();
}

/******************** LOOP ********************/
void loop() {
  // Check if token needs refreshing (every loop is fine, it's smart!)
  if (needsTokenRefresh() && client.connected()) {
    Serial.println("🔄 Token expiring soon! Refreshing and reconnecting~ UwU");
    client.disconnect();
    delay(100);
  }

  // Handle incoming sensor data from coordinator owo
  if (Serial1.available()) {
    value = Serial1.readStringUntil('\n');
    Serial.print("📊 Received sensor data: ");
    Serial.println(value);
  }

  // Send telemetry every 10 seconds~
  static uint32_t last_print = 0;
  if (millis() - last_print > 10000) {
      last_print = millis();
      parseValue();
      sendTelemetry();
  }

  // Reconnect if disconnected (with fresh token if needed!)
  if (!client.connected()) {
    connectToAzure();
  }

  client.loop();
}