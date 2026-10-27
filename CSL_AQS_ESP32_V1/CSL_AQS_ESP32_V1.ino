/*
   COMMUNITY SENSOR LAB - AIR QUALITY SENSOR

   Adafruit Feather ESP32 V2 + Featherwing Adalogger-SD-RTC + SCD30 OR SCD41 -CO2 + BME280 -TPRH + OLED display + SEN55 -PM2.5 VOC NOX

   The SCD30 has a minimum power consumption of 5mA and cannot be stop-started. It's set to 55s (30s nominal)
   sampling period and the featherM0 sleeps for 2 x 16 =32s, wakes and waits for data available.
   Button A toggles display on/off but must be held down for 16s max and then wait 16s to toggle again.

   Logs: sensor, co2, 3x t, 3x rh, press, battery voltage, pm1, pm 2.5, pm 4.0, pm 10, voc, nox 

   https://github.com/Community-Sensor-Lab/Air-Quality-Sensor

   RICARDO TOLEDO-CROW NGENS, ESI, ASRC, CUNY, May 2025
   Edited: Jonathan Suconota
*/
#include "CSL_AQS_ESP32_V1.h"
#include <secrets.h>

// --- Hardware Pin Configuration ---
//#define SD_CS_PIN 5       // Standard ESP32 SPI Chip Select pin for SD cards. Change if your shield differs.

// --- Network Configurations ---
const char* ssid     = SSID;
const char* password = PASSWORD;
#define SAMPLING_RATE 5000;

// --- Queue Configuration ---
#define WIFI_CLIENT_QUEUE_SIZE 100       // Queue capacity
#define WIFI_QUEUE_BATCH_SIZE 1          // How many reading before we start sending to server?
#define SD_QUEUE_SIZE 100                // Queue capacity
#define SD_QUEUE_BATCH_SIZE 1            // How many readings before we start writing to sd card?
#define TESTING_MODE true
QueueHandle_t wclientQueue;  
QueueHandle_t sdQueue;       // Added: Dedicated queue for background SD card logging

 
// --- Helper Function to Generate a Random Float (0.0 to 3.3) ---
float getRandomVoltage() {
  uint32_t raw_rand = esp_random();
  return ((float)raw_rand / (float)UINT32_MAX) * 3.3f;
}

void readSensorsAndRTC(void *pvParameters){
  Serial.println("Calling Read SensorAndRTC");

  if(TESTING_MODE){
    
    Serial.println("
    Serial.println(""
    initializeBME();
    initializeSCD41();
    initializeSEN55();
    initializeRTC()
    ");

    while(1) {
      data payload;
      pyload.now = rtc.now();
      //readBME
      payload.Tbme = getRandomVoltage();
      payload.Pbme = getRandomVoltage();
      payload.RHbme = getRandomVoltage();
      //readSEN55
      payload.CO2 = getRandomVoltage();
      payload.Tco2 = getRandomVoltage();
      //readSCD41
      payload.mPm1_0 = getRandomVoltage();
      payload.mPm2_5 = getRandomVoltage();
      payload.mPm4_0 = getRandomVoltage();
      payload.mPm10 = getRandomVoltage();
      payload.cPm0_5 = getRandomVoltage();
      payload.cPm1_0 = getRandomVoltage();
      payload.cPm2_5 = getRandomVoltage();
      payload.cPm4_0 = getRandomVoltage();
      payload.cPm10 = getRandomVoltage();
      payload.tpSize = getRandomVoltage();
      payload.RHbme = getRandomVoltage();
      payload.Tsen = getRandomVoltage();
      payload.VOCs = getRandomVoltage();
      pyaload.NOx = getRandomVoltage();
      
      if (xQueueSend(sensorQueue, &payload, pdMS_TO_TICKS(0)) != pdPASS) {
        Serial.println("Queue full! Dropping sample");
      }
      vTaskDelay(pdMS_TO_TICKS(2000)); 
    }

  }
  else{
    SensirionI2CSen5x SEN55;
    Adafruit_BME280 bme;
    SCD4x CO2sensor(SCD4x_SENSOR_SCD41);
    RTC_PCF8523 rtc;  
    initializeBME();
    initializeSCD41();
    initializeSEN55();
    initializeRTC()

    while(1) {
      data payload;
      pyload.now = rtc.now();
      readBME(&payload);
      readSEN55(&payload);
      readSCD41(&payload);
      if (xQueueSend(sensorQueue, &payload, pdMS_TO_TICKS(0)) != pdPASS) {
        Serial.println("Queue full! Dropping sample");
      }
      vTaskDelay(pdMS_TO_TICKS(2000)); 
    }

  }
}

void setup() {
  Serial.begin(115200);
  // Initialize both FreeRTOS Queues
  wclientQueue = xQueueCreate(WIFI_CLIENT_QUEUE_SIZE, sizeof(date));
  sdQueue     = xQueueCreate(SD_QUEUE_SIZE, sizeof(data));
  
  if (wclientQueue == NULL || sdQueue == NULL) {
    Serial.println("Queue allocation failed!");
    while(1);
  }

  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Init global vatiables to make it available for both cores
  if(TESTING_MODE){
    Serial.println(" initializeOLED()");
  }
  else{
    initializeOLED();
  }
  
  initializeClient() // Set certificate

  // Spawn independent threads on Core 0
  xTaskCreatePinnedToCore(readSensors, "readSensors", 3070, NULL, 2, NULL, 0);
  // Note: Increased stack memory to 4096 bytes. File system libraries need room.
  xTaskCreatePinnedToCore(writingToSD, "Task_SD", 4096, NULL, 1, NULL, 0);
}

// --- Main Thread: (Runs on Core 1) ---
void loop() {
  Serial.printf("Queue Usage: %d items waiting\n", uxQueueMessagesWaiting(sensorQueue));

  if (uxQueueMessagesWaiting(sensorQueue) >= WIFI_BATCH_SIZE) {
    if (WiFi.status() == WL_CONNECTED) {
    
      for (int i = 0; i < BATCH_SIZE; i++) {
        data sensorData;
        // Retrieve data from payload
        if (xQueueReceive(sensorQueue, &sensorData, pdMS_TO_TICKS(5)) == pdPASS) {
          
        String oustr = String(sensorData.now) +  // Datetime 

                      + String(sensorData.Tbme) + String(", ")  // BME
                      + String(sensorData.Pbme) + String(", ") 
                      + String(sensorData.RHbme) + String(", ") 

                      + String(poppedData.CO2) + String(", ")   // SCD41
                      + String(poppedData.Tco2) + String(", ") 
                      + String(poppedData.RHco2) + String(", ") 

                      + String(sensorData.mPm1_0) + String(", ") // SEN55
                      + String(sensorData.mPm2_5) + String(", ") 
                      + String(sensorData.mPm4_0) + String(", ")
                      + String(sensorData.mPm10)  + String(", ")

                      + String(sensorData.cPm0_5) + String(", ") 
                      + String(sensorData.cPm1_0) + String(", ") 
                      + String(sensorData.cPm2_5) + String(", ") 
                      + String(sensorData.cPm4_0) + String(", ")
                      + String(sensorData.cPm10)  + String(", ")
                      + String(sensorData.tpSize) + String(", ")

                      + String(sensorData.RHsen) + String(", ") 
                      + String(sensorData.Tsen)  + String(", ") 
                      + String(sensorData.VOCs)  + String(", ") 
                      + String(sensorData.NOx)   + String(", ");

          doPost(outstr);
        }
      }
    } 
    else {
      Serial.println("WiFi Disconnected. Waiting to batch.");
    }
  }

  sendHeartbeat();  

  vTaskDelay(pdMS_TO_TICKS(SAMPLING_RATE)); 
}
