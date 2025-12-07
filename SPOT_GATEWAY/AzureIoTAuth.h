/*
 * AzureIoTAuth.h - Azure IoT Hub Authentication Helper
 * Generates SAS tokens dynamically eliminating the need to reflash
 */

#ifndef AZURE_IOT_AUTH_H
#define AZURE_IOT_AUTH_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <time.h>
#include "mbedtls/md.h"
#include "mbedtls/base64.h"

class AzureIoTAuth {
private:
    String deviceKey;
    String hubHost;
    String deviceId;
    Preferences preferences;

    // HMAC-SHA256 signing function
    String hmacSha256(const String& key, const String& data) {
        // Decode base64 key using mbedtls
        size_t keyLen;
        uint8_t decodedKey[128];  // Max key size

        int ret = mbedtls_base64_decode(decodedKey, sizeof(decodedKey), &keyLen,
                                        (const unsigned char*)key.c_str(), key.length());

        if (ret != 0) {
            Serial.println("ERROR: Base64 decode failed");
            return "";
        }

        // Compute HMAC-SHA256
        uint8_t hmacResult[32];
        mbedtls_md_context_t ctx;
        mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
        mbedtls_md_hmac_starts(&ctx, decodedKey, keyLen);
        mbedtls_md_hmac_update(&ctx, (const uint8_t*)data.c_str(), data.length());
        mbedtls_md_hmac_finish(&ctx, hmacResult);
        mbedtls_md_free(&ctx);

        // Encode result to base64 using mbedtls
        size_t encodedLen;
        char encoded[128];  // Max encoded size

        ret = mbedtls_base64_encode((unsigned char*)encoded, sizeof(encoded), &encodedLen,
                                    hmacResult, 32);

        if (ret != 0) {
            Serial.println("ERROR: Base64 encode failed");
            return "";
        }

        encoded[encodedLen] = '\0';  // Null terminate
        return String(encoded);
    }

    // URL encode helper
    String urlEncode(const String& str) {
        String encoded = "";
        char c;
        for (size_t i = 0; i < str.length(); i++) {
            c = str.charAt(i);
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else {
                encoded += '%';
                char hex[3];
                sprintf(hex, "%02X", c);
                encoded += hex;
            }
        }
        return encoded;
    }

public:
    AzureIoTAuth() {}

    // Initialize with credentials stored in NVS
    bool begin(const String& hub, const String& device, const String& key = "") {
        hubHost = hub;
        deviceId = device;

        preferences.begin("azure-iot", false);

        // If key provided, save it to NVS (only needed once)
        if (key.length() > 0) {
            preferences.putString("deviceKey", key);
            Serial.println("Device key saved to NVS. No reflashing required on power loss.");
        }

        // Load key from NVS
        deviceKey = preferences.getString("deviceKey", "");

        if (deviceKey.length() == 0) {
            Serial.println("WARNING: No device key found. Please provide key on first run.");
            return false;
        }

        Serial.println("Azure IoT authentication initialized successfully");
        return true;
    }

    // Get current Unix timestamp (seconds since 1970)
    unsigned long getUnixTime() {
        time_t now;
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            // If time not synced yet, use a far future timestamp as fallback
            // This will be corrected after NTP sync
            return 1735689600 + (millis() / 1000);  // Jan 1, 2025 + uptime
        }
        time(&now);
        return (unsigned long)now;
    }

    // Generate a fresh SAS token
    // ttl = time to live in seconds (default 1 hour)
    String generateSasToken(unsigned long ttl = 3600) {
        // Calculate expiry time (Unix timestamp)
        unsigned long expiry = getUnixTime() + ttl;

        // Build the string to sign
        String resourceUri = hubHost + "/devices/" + deviceId;
        String stringToSign = resourceUri + "\n" + String(expiry);

        // Sign the string with HMAC-SHA256
        String signature = hmacSha256(deviceKey, stringToSign);

        // URL encode the signature
        String encodedSignature = urlEncode(signature);

        // Build the SAS token
        String sasToken = "SharedAccessSignature sr=" + resourceUri;
        sasToken += "&sig=" + encodedSignature;
        sasToken += "&se=" + String(expiry);

        Serial.println("Generated fresh SAS token. Expires at: " + String(expiry));

        return sasToken;
    }

    // Clear stored credentials (for testing)
    void clearCredentials() {
        preferences.clear();
        Serial.println("Cleared stored credentials from NVS");
    }
};

#endif // AZURE_IOT_AUTH_H
