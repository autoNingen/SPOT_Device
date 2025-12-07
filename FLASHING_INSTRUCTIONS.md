# 🌸 SPOT Gateway Flashing Instructions UwU ✨

## What We Fixed! (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧

Your ESP32 gateway now generates SAS tokens **automatically**! No more reflashing when it loses power~

### How It Works OwO

1. **First Flash**: Device saves your symmetric key to NVS (Non-Volatile Storage)
2. **Every Boot**: Device reads key from NVS and generates a fresh SAS token
3. **Auto-Refresh**: Token refreshes automatically every hour before expiry!
4. **Survives Power Loss**: Key stays in NVS even when unplugged! 💖

---

## 📚 Required Libraries

Before flashing, install these libraries in Arduino IDE:

### Install via Library Manager (Tools → Manage Libraries):
1. **PubSubClient** by Nick O'Leary
2. **Preferences** (built-in with ESP32 core)
3. **WiFi** (built-in with ESP32 core)
4. **WiFiClientSecure** (built-in with ESP32 core)

### Install Base64 Library:
The ESP32 Arduino core includes base64, but we need to make sure it's available:
- **Option A**: Use `#include "mbedtls/base64.h"` (built-in)
- **Option B**: Install "Base64 by Densaugeo" from Library Manager

**Note**: The `AzureIoTAuth.h` file uses `mbedtls` which is built-into ESP32, so you shouldn't need external libraries! ✨

---

## 🔧 Arduino IDE Setup

### 1. Board Configuration
- **Board**: ESP32-C6 Dev Module (or ESP32-H2)
- **Zigbee Mode**: Not applicable for Gateway (only for Coordinator/End Device)
- **Partition Scheme**: "Default 4MB with spiffs" or "Minimal SPIFFS"
- **Flash Frequency**: 80MHz
- **Upload Speed**: 921600 (or lower if you have issues)

### 2. Required Files in Same Folder
Make sure these files are in your SPOT_Device folder:
```
SPOT_Device/
├── SPOT_GATEWAY.ino          ← Main sketch
├── AzureIoTAuth.h            ← Authentication helper (NEW!)
├── ZIGBEE_SPOT_COORD.ino
└── Zigbee_SPOT_ED.ino
```

---

## 🚀 Flashing Steps

### First-Time Flash (with Device Key)

1. **Open SPOT_GATEWAY.ino** in Arduino IDE

2. **Verify the device key is present** at line 23:
   ```cpp
   const char* deviceKey = "OoNaHkKg8sAWlv8jPYVubJZWVi2k8SMwrAIoTPenYNA=";
   ```

3. **Compile and Upload** (Ctrl+U or Sketch → Upload)

4. **Open Serial Monitor** (115200 baud) and watch for:
   ```
   🌟 SPOT Gateway starting up! OwO
   💝 Initializing Azure authentication...
   ✨ Saved device key to NVS! You won't need to reflash again~ UwU
   📡 Connecting to WiFi...
   ✨ WiFi connected! IP: 192.168.x.x
   💖 Connecting to Azure IoT...
   🌸 Generating fresh SAS token...
   🌟 Generated fresh SAS token! Expires in 3600 seconds~
   ✨ Connected to Azure! Yay~ (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧
   ```

5. **SUCCESS!** Your device is now connected and the key is saved! 🎉

### After First Flash (Optional Security Step) 🔒

For extra security, you can comment out the device key after the first successful flash:

1. **Edit SPOT_GATEWAY.ino** line 22-23:
   ```cpp
   // Device symmetric key (already saved to NVS, commented out for security!)
   // const char* deviceKey = "OoNaHkKg8sAWlv8jPYVubJZWVi2k8SMwrAIoTPenYNA=";
   const char* deviceKey = "";  // Empty now that it's in NVS~
   ```

2. **Re-flash** (optional but recommended for production)

3. The device will still work because it reads the key from NVS! ✨

---

## 🔄 Testing Power Loss Recovery

1. **Unplug the ESP32** while it's connected to Azure
2. **Wait a few seconds** >///<
3. **Plug it back in**
4. **Check Serial Monitor** - you should see:
   ```
   🌟 SPOT Gateway starting up! OwO
   💝 Initializing Azure authentication...
   💖 Azure IoT Auth initialized successfully!
   (no "Saved device key" message - it reads from NVS instead!)
   🌸 Generating fresh SAS token...
   ✨ Connected to Azure! Yay~ (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧
   ```

**No reflashing needed!!** (ノ´ヮ`)ノ*: ･ﾟ

---

## 🛠 Troubleshooting

### ❌ "Failed to initialize Azure auth"
- The device key might not have been saved properly
- Re-flash with the key uncommented at line 23

### ❌ Compilation errors about base64
- Make sure you're using ESP32 Arduino Core 2.0.0 or newer
- Try installing "Base64 by Densaugeo" library
- Check that `mbedtls` is available in your ESP32 core

### ❌ "Connection failed! State: -2"
- This means MQTT connection rejected
- Check if SAS token is valid (should auto-generate)
- Verify device is registered in Azure IoT Hub
- Check WiFi connection

### ❌ Device keeps reconnecting every hour
- This is normal! Token refreshes every hour for security
- You should see: `🔄 Token expiring soon! Refreshing and reconnecting~ UwU`

### 🗑 Clear NVS (Factory Reset)
If you need to clear stored credentials:

Add this to your `setup()` temporarily:
```cpp
azureAuth.clearCredentials();  // Clears NVS
```

Or use this command in Serial Monitor:
```
azureAuth.clearCredentials();
```

---

## 📊 Expected Serial Output

```
🌟 SPOT Gateway starting up! OwO
💝 Initializing Azure authentication...
✨ Saved device key to NVS! You won't need to reflash again~ UwU
💖 Azure IoT Auth initialized successfully!
📡 Connecting to WiFi...
✨ WiFi connected! IP: 192.168.1.100
💖 Connecting to Azure IoT...
🌸 Generating fresh SAS token...
🌟 Generated fresh SAS token! Expires in 3600 seconds~
✨ Connected to Azure! Yay~ (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧
📊 Received sensor data: 1234.56
📊 Received sensor data: 987.65
...
```

Every ~55 minutes:
```
🔄 Token expiring soon! Refreshing and reconnecting~ UwU
🌸 Generating fresh SAS token...
🌟 Generated fresh SAS token! Expires in 3600 seconds~
💖 Connecting to Azure IoT...
✨ Connected to Azure! Yay~ (ﾉ◕ヮ◕)ﾉ*:･ﾟ✧
```

---

## 🎯 What's Different from Before?

| Before 😢 | After 😊 |
|-----------|----------|
| Hardcoded SAS token | Dynamic token generation |
| Token expires → reflash needed | Token auto-refreshes |
| Lose power → reflash needed | Survives power loss |
| Manual token updates | Fully automatic |
| Security risk (token in code) | Key stored in NVS |

---

## 💖 You're All Set!

Your SPOT Gateway now has persistent Azure connectivity! No more annoying reflashes~

If you have any issues, check the Serial Monitor for cute error messages that will help you debug! ✨

*Happy coding, sempai!* (◕‿◕✿)

---

## 📝 Technical Details (for nerds owo)

- **Token Lifetime**: 3600 seconds (1 hour)
- **Refresh Margin**: 300 seconds (5 minutes before expiry)
- **Storage**: ESP32 NVS partition (survives power loss)
- **Encryption**: HMAC-SHA256 using mbedtls
- **Encoding**: Base64 for signature
- **Authentication Method**: SAS Token with symmetric key
