// SPOT Gateway - Automatic SAS Token Generation
// Eliminates the need to reflash when device loses power

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include "AzureIoTAuth.h"  // Authentication helper for dynamic token generation

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
// IMPORTANT: After first successful flash, you can comment this out for security
const char* deviceKey = "OoNaHkKg8sAWlv8jPYVubJZWVi2k8SMwrAIoTPenYNA=";

/******************** WIFI + AZURE ********************/
WiFiClientSecure espClient;
PubSubClient client(espClient);
AzureIoTAuth azureAuth;  // Token generator instance

String currentSasToken = "";  // Dynamically generated token
unsigned long tokenExpiryTime = 0;  // Token expiry tracking

String value; // Value to be parsed
String v1;
String carDetected = "vacant";

void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);
}

// Sync time with NTP server
void syncTime() {
    Serial.println("Syncing time with NTP server...");
    configTime(-6 * 3600, 0, "pool.ntp.org", "time.nist.gov");  // CST timezone (UTC-6)

    struct tm timeinfo;
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < 10) {
        Serial.print(".");
        delay(1000);
        retry++;
    }

    if (retry < 10) {
        Serial.println("\nTime synced! Current time: " + String(asctime(&timeinfo)));
    } else {
        Serial.println("\nWarning: Time sync failed, using fallback timestamp");
    }
}

// Generate a fresh SAS token
void refreshSasToken() {
  Serial.println("Generating fresh SAS token...");
  // Token valid for 1 hour (3600 seconds)
  currentSasToken = azureAuth.generateSasToken(3600);
  // Remember when it expires (with 5 min safety margin)
  tokenExpiryTime = millis() + (3600 - 300) * 1000UL;
}

// Check if token needs refreshing
bool needsTokenRefresh() {
  if (currentSasToken.length() == 0) return true;  // No token yet
  if (millis() >= tokenExpiryTime) return true;    // Token expired
  return false;
}

void connectToAzure() {
  // Generate token if needed
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
      Serial.println("Connecting to Azure IoT Hub...");
      bool connected = client.connect(clientId.c_str(), username.c_str(), currentSasToken.c_str());

      if (connected) {
        Serial.println("Successfully connected to Azure IoT Hub");
        return;
      } else {
        Serial.printf("Connection failed! MQTT State: %d, retrying...\n", client.state());
        retryCount++;
        delay(2000);
      }
  }

  if (!client.connected()) {
    Serial.println("Could not connect after 5 attempts. Will retry later.");
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

    Serial.println("SPOT Gateway initializing...");

    // Initialize Azure auth (saves key to NVS on first run)
    Serial.println("Initializing Azure authentication...");
    if (!azureAuth.begin(iothubHost, deviceId, deviceKey)) {
        Serial.println("ERROR: Failed to initialize Azure authentication. Check device key.");
        while(1) { delay(1000); }  // Stop here if auth fails
    }

    // Connect WiFi & Azure
    Serial.println("Connecting to WiFi...");
    connectToWiFi();
    Serial.println("WiFi connected. IP: " + WiFi.localIP().toString());

    // Sync time with NTP (required for SAS token generation)
    syncTime();

    connectToAzure();
}

/******************** LOOP ********************/
void loop() {
  // Check if token needs refreshing
  if (needsTokenRefresh() && client.connected()) {
    Serial.println("Token expiring soon. Refreshing and reconnecting...");
    client.disconnect();
    delay(100);
  }

  // Handle incoming sensor data from coordinator
  if (Serial1.available()) {
    value = Serial1.readStringUntil('\n');
    Serial.print("Received sensor data: ");
    Serial.println(value);
  }

  // Send telemetry every 10 seconds
  static uint32_t last_print = 0;
  if (millis() - last_print > 10000) {
      last_print = millis();
      parseValue();
      sendTelemetry();
  }

  // Reconnect if disconnected (with fresh token if needed)
  if (!client.connected()) {
    connectToAzure();
  }

  client.loop();
}
