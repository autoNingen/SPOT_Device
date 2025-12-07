/*
 * AzureIoTAuth.h - Azure IoT Hub Authentication Helper UwU
 * Generates SAS tokens dynamically so you never need to reflash! ✨
 */

#ifndef AZURE_IOT_AUTH_H
#define AZURE_IOT_AUTH_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <base64.h>
#include <Preferences.h>
#include "mbedtls/md.h"

class AzureIoTAuth {
private:
    String deviceKey;
    String hubHost;
    String deviceId;
    Preferences preferences;

    // HMAC-SHA256 signing function owo
    String hmacSha256(const String& key, const String& data) {
        // Decode base64 key
        int keyLen = base64_dec_len(key.c_str(), key.length());
        uint8_t decodedKey[keyLen];
        base64_decode((char*)decodedKey, (char*)key.c_str(), key.length());

        // Compute HMAC
        uint8_t hmacResult[32];
        mbedtls_md_context_t ctx;
        mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
        mbedtls_md_hmac_starts(&ctx, decodedKey, keyLen);
        mbedtls_md_hmac_update(&ctx, (const uint8_t*)data.c_str(), data.length());
        mbedtls_md_hmac_finish(&ctx, hmacResult);
        mbedtls_md_free(&ctx);

        // Encode result to base64
        int encodedLen = base64_enc_len(32);
        char encoded[encodedLen];
        base64_encode(encoded, (char*)hmacResult, 32);

        return String(encoded);
    }

    // URL encode helper UwU
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

    // Initialize with credentials stored in NVS >///<
    bool begin(const String& hub, const String& device, const String& key = "") {
        hubHost = hub;
        deviceId = device;

        preferences.begin("azure-iot", false);

        // If key provided, save it to NVS (only needed once!)
        if (key.length() > 0) {
            preferences.putString("deviceKey", key);
            Serial.println("✨ Saved device key to NVS! You won't need to reflash again~ UwU");
        }

        // Load key from NVS
        deviceKey = preferences.getString("deviceKey", "");

        if (deviceKey.length() == 0) {
            Serial.println("⚠ No device key found! Please provide key on first run ówò");
            return false;
        }

        Serial.println("💖 Azure IoT Auth initialized successfully!");
        return true;
    }

    // Generate a fresh SAS token! ✨
    // ttl = time to live in seconds (default 1 hour)
    String generateSasToken(unsigned long ttl = 3600) {
        // Calculate expiry time (Unix timestamp)
        unsigned long expiry = (millis() / 1000) + ttl;

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

        Serial.println("🌟 Generated fresh SAS token! Expires in " + String(ttl) + " seconds~");

        return sasToken;
    }

    // Clear stored credentials (for testing)
    void clearCredentials() {
        preferences.clear();
        Serial.println("🗑 Cleared stored credentials from NVS");
    }
};

#endif // AZURE_IOT_AUTH_H
