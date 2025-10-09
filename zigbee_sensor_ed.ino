#ifndef ZIGBEE_MODE_ED
#error "Set Zigbee mode to end device"
#endif

#include "Zigbee.h"
#include "Adafruit_VL53L0X.h"
#include <Wire.h>

#define TEMP_SENSOR_ENDPOINT_NUMBER 10
ZigbeeTempSensor zbTempSensor = ZigbeeTempSensor(TEMP_SENSOR_ENDPOINT_NUMBER);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

static void temp_sensor_value_update(void *arg) {
  float tsens_value = temperatureRead();
  Serial.printf("Updated temp value to %.2f C\r\n", tsens_value);

  zbTempSensor.setTemperature(tsens_value);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(6, 7);
  // wait until serial port opens for native USB devices
  while (! Serial) {
    delay(1);
  }
  
  Serial.println("Adafruit VL53L0X test");
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
  // power 
  Serial.println(F("VL53L0X API Simple Ranging example\n\n")); 

  Zigbee.addEndpoint(&zbTempSensor);
  Serial.println("Starting Zigbee...");
  if (!Zigbee.begin()) {
    Serial.println("Zigbee failed to start!\n");
    Zigbee.factoryReset();
  } else {
    Serial.println("Zigbee started successfully!");
  }
  Serial.println("Connecting to network");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("Connected!");
  zbTempSensor.setReporting(1,0,1);
}


void loop() {
  VL53L0X_RangingMeasurementData_t measure;
    
  Serial.print("Reading a measurement... ");
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    Serial.print("Distance (mm): "); Serial.println(measure.RangeMilliMeter);
  } else {
    Serial.println(" out of range ");
  }
  float distFloat = (float)measure.RangeMilliMeter; // typecast to float
  // xTaskCreate(temp_sensor_value_update, "temp_sensor_update", 2048, NULL, 10, NULL);
  zbTempSensor.setTemperature(distFloat);
  
  if(!zbTempSensor.reportTemperature()) {
    Serial.println("Failed to report temperature!\n");
  }  
  delay(1000);
}
