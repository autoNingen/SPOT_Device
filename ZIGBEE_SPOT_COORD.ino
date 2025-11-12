#ifndef ZIGBEE_MODE_ZCZR
#error "Zigbee coordinator mode is not selected in Tools->Zigbee mode"
#endif

#define RX_PIN 20
#define TX_PIN 21

#include "Zigbee.h"

/* Zigbee configuration */
#define THERMOSTAT_ENDPOINT_NUMBER   1
#define USE_RECEIVE_TEMP_WITH_SOURCE 1

ZigbeeThermostat zbThermostat = ZigbeeThermostat(THERMOSTAT_ENDPOINT_NUMBER);

// Save temperature sensor data
float sensor_temp;
float sensor_max_temp;
float sensor_min_temp;
float sensor_tolerance;

float validateSensorData(float temperature) {
  float output = (temperature - 30.0) * 100.0;
  return output;
}
/****************** Temperature cluster handling *******************/
#if USE_RECEIVE_TEMP_WITH_SOURCE == 0
void receiveSensorTemp(float temperature) {
  Serial.printf("Temperature sensor value: %.2f°C\n", temperature);
  sensor_temp = temperature;
}
#else
void receiveSensorTempWithSource(float temperature, uint8_t src_endpoint, esp_zb_zcl_addr_t src_address) {
  if (src_address.addr_type == ESP_ZB_ZCL_ADDR_TYPE_SHORT) {
    Serial.printf("Distance sensor value: %.2f mm from endpoint %d, address 0x%04x\n", validateSensorData(temperature), src_endpoint, src_address.u.short_addr);
  } else {
    Serial.printf(
      "Distance sensor value: %.2f°C from endpoint %d, address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", temperature, src_endpoint,
      src_address.u.ieee_addr[7], src_address.u.ieee_addr[6], src_address.u.ieee_addr[5], src_address.u.ieee_addr[4], src_address.u.ieee_addr[3],
      src_address.u.ieee_addr[2], src_address.u.ieee_addr[1], src_address.u.ieee_addr[0]
    );
  }
  sensor_temp = validateSensorData(temperature);
}
#endif

void receiveSensorConfig(float min_temp, float max_temp, float tolerance) {
  Serial.printf("Distance sensor config: min %.2f°C, max %.2f°C, tolerance %.2f°C\n", min_temp, max_temp, tolerance);
  sensor_min_temp = min_temp;
  sensor_max_temp = max_temp;
  sensor_tolerance = tolerance;
}
/********************* Arduino functions **************************/
void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);  // UART1

// Set callback function for receiving temperature from sensor - Use only one option
#if USE_RECEIVE_TEMP_WITH_SOURCE == 0
  zbThermostat.onTempReceive(receiveSensorTemp);  // If you bound only one sensor or you don't need to know the source of the temperature
#else
  zbThermostat.onTempReceiveWithSource(receiveSensorTempWithSource);
#endif

  // Set callback function for receiving sensor configuration
  zbThermostat.onConfigReceive(receiveSensorConfig);

  //Add endpoint to Zigbee Core
  Zigbee.addEndpoint(&zbThermostat);

  //Open network for 180 seconds after boot
  Zigbee.setRebootOpenNetwork(300);

  // When all EPs are registered, start Zigbee with ZIGBEE_COORDINATOR mode
  if (!Zigbee.begin(ZIGBEE_COORDINATOR)) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  }

  Serial.println("Waiting for end device to bind to the coordinator");
  while (!zbThermostat.bound()) {
    Serial.printf(".");
    delay(500);
  }

  Serial.println();

  // Get temperature sensor configuration for all bound sensors by endpoint number and address
  std::list<zb_device_params_t *> boundSensors = zbThermostat.getBoundDevices();
  for (const auto &device : boundSensors) {
    Serial.println("--------------------------------");
    if (device->short_addr == 0x0000 || device->short_addr == 0xFFFF) {  //End devices never have 0x0000 short address or 0xFFFF group address
      Serial.printf(
        "Device on endpoint %d, IEEE Address: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\r\n", device->endpoint, device->ieee_addr[7], device->ieee_addr[6],
        device->ieee_addr[5], device->ieee_addr[4], device->ieee_addr[3], device->ieee_addr[2], device->ieee_addr[1], device->ieee_addr[0]
      );
      zbThermostat.getSensorSettings(device->endpoint, device->ieee_addr);
    } else {
      Serial.printf("Device on endpoint %d, short address: 0x%x\r\n", device->endpoint, device->short_addr);
      zbThermostat.getSensorSettings(device->endpoint, device->short_addr);
    }
  }
}

void loop() {
  // Removed button functonality

  // Periodically check if any end devices are connected
  static uint32_t last_check = 0;
  if (millis() - last_check > 30000) { // every 30 seconds
    last_check = millis();

    std::list<zb_device_params_t *> boundSensors = zbThermostat.getBoundDevices();
    if (boundSensors.empty()) {
      Serial.println("No end devices connected, reopening Zigbee network for 180 seconds...");
      Zigbee.openNetwork(180); // reopen for 180 seconds
    } else {
      Serial.printf("%d end device(s) currently connected.\n", boundSensors.size());
    }
    Serial.printf("No bound devices: %d\n", boundSensors.empty());
  }

  // Print time-of-flight sensor data each 10 seconds
  static uint32_t last_print = 0;
  if (millis() - last_print > 10000) {
    last_print = millis();
    zbThermostat.getTemperature();
    sendFloat(Serial1, sensor_temp);

    Serial.print("Sent: ");
    Serial.println(sensor_temp);
  }
}

void sendFloat(HardwareSerial &serialPort, float value) {
  // Convert float to string with 2 decimal places
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%.2f\n", value);

  // Send over UART
  serialPort.print(buffer);
}
