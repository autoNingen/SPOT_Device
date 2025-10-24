/*--------------------------------------------------------------------------------------*/
/*-------------------------- VARIABLES AND DEFINITIONS ---------------------------------*/
/*--------------------------------------------------------------------------------------*/

// Verify esp32 device status
#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

// Libraries
#include <Stdio.h>              
#include <Wire.h>               // I2C lib
#include <U8g2lib.h>            // Display lib
#include "Adafruit_VL53L0X.h"   // Time of flight lib
#include "Zigbee.h"             // Zigbee lib

// Create zigbee temp sensor object (used to pass time of flight data)
#define TEMP_SENSOR_ENDPOINT_NUMBER 10
ZigbeeTempSensor zbTempSensor = ZigbeeTempSensor(TEMP_SENSOR_ENDPOINT_NUMBER);

// Create adafruit tof sensor object
#define TOF_GPIO 2
#define XSHUT 3
Adafruit_VL53L0X sensor = Adafruit_VL53L0X();
volatile bool tof_status = true;
volatile float output_value;

// Create U8G2 display object (SH1106 128x64, I2C)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 7, /* data=*/ 6);
// Define SPOT ID here:
const char* spotID = "A1";

// Current Display State
enum DisplayPage 
{ 
  PAGE_HOME, 
  PAGE_DIAGNOSTIC 
};
volatile DisplayPage currentPage = PAGE_HOME;
volatile bool displayOn = true;

// Button Pins
#define POWER 10
#define CYCLE 11
#define ZIGBEE_RESET BOOT_PIN

// Charging Status Pins
#define STAT1 0
#define STAT2 1
/*
1&2 == HIGH -> Charge complete (NOT CHARGING)
1 == LOW & 2 == HIGH -> Normal charge
1 == HIGH & 2 == LOW -> Recoverable Error
1&2 == LOW -> Shit
*/
#define CE 8      // SETTING THIS PIN LOW DISABLES BATTERY CHARGING

// Battery status count
volatile unsigned int count = 10;

/*--------------------------------------------------------------------------------------*/
/*----------------------------- MAIN ARDUINO PROGRAM -----------------------------------*/
/*--------------------------------------------------------------------------------------*/

void setup() 
{
  // Init I2C pins and Serial
  Wire.begin(6,7);
  Serial.begin(115200);
  delay(1000);

  // Verify TOF init
  Serial.printf("Adafruit VL53L0X test\n");
  while (!sensor.begin(0x29))
  {
    Serial.printf(F("Failed to boot VL53L0X"));
    delay(1000);
  }

  // Init buttons
  pinMode(CYCLE, INPUT_PULLUP);
  pinMode(POWER, INPUT_PULLUP);
  pinMode(ZIGBEE_RESET, INPUT_PULLUP);

  // Init status pins
  pinMode(STAT1, INPUT_PULLUP);
  pinMode(STAT2, INPUT_PULLUP);
  pinMode(CE, INPUT_PULLUP);

    // Init TOF pins
  pinMode(TOF_GPIO, INPUT_PULLUP);
  pinMode(XSHUT, INPUT_PULLUP);

  /* ESP ZIGBEE CONFIGURATIONS */
  // Optional: set Zigbee device name and model
  zbTempSensor.setManufacturerAndModel("Espressif", "ZigbeeTempSensor");
  // Set minimum and maximum temperature measurement value (10-50°C is default range for chip temperature measurement)
  zbTempSensor.setMinMaxValue(10, 50);
  // Optional: Set tolerance for temperature measurement in °C (lowest possible value is 0.01°C)
  zbTempSensor.setTolerance(1);
  // Optional: Time cluster configuration (default params, as this device will revieve time from coordinator)
  zbTempSensor.addTimeCluster();
  // Add endpoint to Zigbee Core
  Zigbee.addEndpoint(&zbTempSensor);

// Init display
  u8g2.begin();
  updateDisplay();
  delay(1000);

  // Init Zigbee
  Serial.println("Starting Zigbee...");
  // When all EPs are registered, start Zigbee in End Device mode
  if (!Zigbee.begin()) 
  {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  } 
  else 
  {
    Serial.println("Zigbee started successfully!");
  }
  Serial.println("Connecting to network");
  while (!Zigbee.connected()) 
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println("\nZigbee Device Connected!\n");
  // Create Button Task
  xTaskCreate(
    buttonTask,          // Task function
    "ButtonTask",        // Name
    2048,                // Stack size
    NULL,                // Parameters
    5,                   // Priority
    NULL);               // Task handle

  // Create Counting Task
  xTaskCreate(
    counting,            // Task function
    "CountingTask",      // Name
    2048,                // Stack size
    NULL,                // Parameters
    2,                   // Priority
    NULL);               // Task handle

  xTaskCreate(
    temp_sensor_value_update,            // Task function
    "DistanceTask",      // Name
    2048,                // Stack size
    NULL,                // Parameters
    1,                   // Priority
    NULL);
  // Set reporting interval for temperature measurement in seconds, must be called after Zigbee.begin()
  // min_interval and max_interval in seconds, delta (temp change in 0,1 °C)
  // if min = 1 and max = 0, reporting is sent only when temperature changes by delta
  // if min = 0 and max = 10, reporting is sent every 10 seconds or temperature changes by delta
  // if min = 0, max = 10 and delta = 0, reporting is sent every 10 seconds regardless of temperature change
  zbTempSensor.setReporting(1, 0, 1);
}

void loop() 
{
  // Do nothing
}

/*--------------------------------------------------------------------------------------*/
/*----------------------------- TOF DATA AQUISITION ------------------------------------*/
/*--------------------------------------------------------------------------------------*/

// Sensing from VL53L0X and input validation
float readSensorData() 
{
  float rangeData;
  VL53L0X_RangingMeasurementData_t measure;
  sensor.rangingTest(&measure, false);

  // Verify measurement is in range
  if (measure.RangeStatus != 4) 
  {  // phase failures have incorrect data
    rangeData = (measure.RangeMilliMeter / 100.0) + 30.0;
  } 
  else 
  {
    Serial.printf(" out of range \n");
    rangeData = -1.0; // NEGATIVE VALUE TO INDICATE "OUT OF RANGE"
  }

  return rangeData;
}

// WE NEED TO ADD SOME WAY TO NOT CHECK DISTANCE USING TOF_GPIO  (its pretty much an interrupt)
// FreeRTOS task to update the temp sensor value
static void temp_sensor_value_update(void *arg)
{
  for (;;) 
  {
    // Ensure TOF needs measurement
    if (tof_status)
    {
      digitalWrite(XSHUT, HIGH);
      float tsens_value = readSensorData();

      // Read sensor value
      output_value = (tsens_value - 30.0) * 100.0;
      if (output_value > 0) Serial.printf("Updated distance sensor value to %.2f mm\r\n", output_value);
      else Serial.printf("TOF Data is out of range\n");

      if (currentPage == PAGE_HOME) {
        updateDisplay();
      }
      // Update value in Temperature sensor EP
      zbTempSensor.setTemperature(tsens_value);
    }
    else 
    {
      digitalWrite(XSHUT, LOW);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Polling interval
  }
}

/*---------------------------------------------------------------------------------------*/
/*----------------------------- DISPLAY AND CONTROLS ------------------------------------*/
/*--------------------------------------------------------------------------------------*/

// Display update function
void updateDisplay() 
{
  if (!displayOn) return;

  u8g2.clearBuffer(); // Clear internal memory

  if (currentPage == PAGE_HOME) 
  {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "SPOT - ParkSense v1.2");
    u8g2.drawHLine(0, 12, 128);  // Horizontal line under title

    // Perimeter box
    uint8_t boxX = 10;
    uint8_t boxY = 20;
    uint8_t boxW = 108;
    uint8_t boxH = 40;
    u8g2.drawFrame(boxX, boxY, boxW, boxH);  // Draw rectangle

    // Centered content inside box
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.setCursor(boxX + 5, boxY + 8);
    if (STAT1 == HIGH && STAT2 == HIGH) u8g2.print("Battery not Charging");
    else if (STAT1 == LOW && STAT2 == HIGH) u8g2.print("Battery Charging");
    else if (STAT1 == HIGH && STAT2 == LOW) u8g2.print("System Error - Battery");
    else u8g2.print("Critical Error - Battery");
    
    u8g2.setCursor(boxX + 5, boxY + 18);
    u8g2.print("Last TOF: ");
    char outputString[10];
    sprintf(outputString, "%.2f m", output_value);
    u8g2.print(outputString);
    
    u8g2.setCursor(boxX + 5, boxY + 28);
    char idString[10];
    sprintf(idString, "ID: %s", spotID);
    u8g2.print(idString);
  } 
  else 
  {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "Diagnostics");
    u8g2.drawHLine(0, 12, 128);

    // Battery label
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(0, 20, "Battery Level:");

    // Battery dimensions
    const int x = 0;
    const int y = 22;
    const int blockW = 10;
    const int blockH = 15;
    const int gap = 2;

    // Draw battery
    for (int i = 0; i < 10; i++) 
    {
    int bx = x + i * (blockW + gap);
    if (i < count)
      u8g2.drawBox(bx, y, blockW, blockH);  // filled block
    else
      u8g2.drawFrame(bx, y, blockW, blockH);  // empty block
    }
    u8g2.drawBox(120, 27, 2, 4);  // filled block
  }

  u8g2.sendBuffer();
}

// FreeRTOS task to handle buttons
void buttonTask(void *arg) 
{
  bool lastCycleBtn = HIGH;
  bool lastPowerBtn = HIGH;
  bool lastResetBtn = HIGH;
  unsigned long lastDebounceTimeCycle = 0;
  unsigned long lastDebounceTimePower = 0;
  unsigned long lastDebounceTimeReset = 0;

  for (;;) 
  {
    bool cycleBtn = digitalRead(CYCLE);
    bool powerBtn = digitalRead(POWER);
    bool resetBtn = digitalRead(ZIGBEE_RESET);
    unsigned long now = millis();

    // Page cycle button (hold for 0.2 seconds)
    if (lastCycleBtn == HIGH && cycleBtn == LOW && now - lastDebounceTimeCycle > 200) 
    {
      currentPage = (currentPage == PAGE_HOME) ? PAGE_DIAGNOSTIC : PAGE_HOME;
      updateDisplay();

      lastDebounceTimeCycle = now;
    }

    // Power button (hold for 3 seconds)
    if (lastPowerBtn == HIGH && powerBtn == LOW && now - lastDebounceTimePower > 3000) 
    {
      
      /* THIS WILL TURN ON AND OFF THE OLED DISPLAY
      displayOn = !displayOn;
      
      if (displayOn) 
      {
        u8g2.setPowerSave(0);
        updateDisplay();
      } 
      else 
      {
        u8g2.setPowerSave(1);
      }
      */

      lastDebounceTimePower = now;
    }

    // Zigbee reset button (hold for 3 seconds)
    if (lastResetBtn == HIGH && resetBtn == LOW && now - lastDebounceTimeReset > 3000) 
    {
      Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
      delay(1000);
      Zigbee.factoryReset();

      lastDebounceTimeReset = now;
    }

    // Update button status
    lastCycleBtn = cycleBtn;
    lastPowerBtn = powerBtn;
    lastResetBtn = resetBtn;

    vTaskDelay(50 / portTICK_PERIOD_MS);  // Polling interval
  }
}

/*--------------------------------------------------------------------------------------*/
/*--------------------------------- DIAGNOSTICS ----------------------------------------*/
/*--------------------------------------------------------------------------------------*/

// FreeRTOS task used to demo battery usage screen
void counting(void *arg) 
{
  for (;;) 
  {
    count--;
    if (count == -1) count = 10;
    if (currentPage == PAGE_DIAGNOSTIC && displayOn) updateDisplay();

    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Polling interval
  }
}