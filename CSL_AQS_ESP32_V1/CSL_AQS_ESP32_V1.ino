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
const char* ssid     = "";
const char* password = "";
const char* serverAddress = "http://192.168.1.243:3002/api/sensor-reading"; 

// --- Data Structures ---

// --- Queue Configuration ---
#define QUEUE_SIZE 100       // Optimization: Increased capacity buffer
#define BATCH_SIZE 10      
QueueHandle_t sensorQueue;  
QueueHandle_t sdQueue;       // Added: Dedicated queue for background SD card logging


// --- Add your heartbeat server address ---
const char* heartbeatAddress = "http://192.168.1.243:3002/api/heartbeat";

// --- Heartbeat Task (Runs independently on Core 0) ---
void HeartbeatTask(void * pvParameters) {
  Serial.println("Initialize Heartbeat Task");
  
  while(1) {
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      http.begin(client, heartbeatAddress);
      http.addHeader("Content-Type", "application/json");

      DynamicJsonDocument doc(128);
      doc["macAddress"] = WiFi.macAddress();
      doc["uptimeSeconds"] = millis() / 1000;

      String payload;
      serializeJson(doc, payload);

      int httpResponseCode = http.POST(payload);
      if (httpResponseCode > 0) {
        Serial.printf("[Heartbeat] Sent successfully (Status: %d)\n", httpResponseCode);
      } else {
        Serial.printf("[Heartbeat] Failed: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      http.end();
    } else {
      Serial.println("[Heartbeat] Wi-Fi disconnected, skipping pulse.");
    }

    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

// --- Helper Function to Generate a Random Float (0.0 to 3.3) ---
float getRandomVoltage() {
  uint32_t raw_rand = esp_random();
  return ((float)raw_rand / (float)UINT32_MAX) * 3.3f;
}

void LED(void * pvParameters) {
  Serial.println("Initialize LED");
  while(1) {
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

void RTC(void * pvParameters) {
  Serial.println("Initialize RTC");
  while(1) {
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

// --- Background SD Card Task (Runs on Core 0) ---
void writingToSD(void *pvParameters) {
  Serial.println("Initializing SD Writer Background Task...");

   // --- SD Card Hardware Initialization ---
  Serial.print("Initializing SD card hardware...");
  if (!SD.begin()) {
    Serial.println(" Failed! Check wiring or CS Pin. Running without logger backup.");
  } else {
    Serial.println(" Success.");
    // Write standard CSV structure header if creating a brand new file
    if (!SD.exists("/datalog.csv")) {
      File file = SD.open("/datalog.csv", FILE_WRITE);
      if (file) {
        file.println("SensorA,ValA,SensorB,ValB,SensorC,ValC");
        file.close();
      }
    }
  }

  data logData;
  bool sdConnected = true; // Track state

  while(1) {
    if (xQueueReceive(sdQueue, &logData, pdMS_TO_TICKS(1000)) == pdPASS) {
      
        
        // If previously disconnected, try to re-initialize before writing
        if (!sdConnected) {
          Serial.println("[SD] Attempting to re-mount SD card...");
          if (SD.begin()) {
            sdConnected = true;
            Serial.println("[SD] Re-connected successfully!");
          }
        }

        if (sdConnected) {
          File file = SD.open("/datalog.csv", FILE_APPEND);
          if (file) {
            file.printf("%c,%.2f,%c,%.2f,%c,%.2f\n", 
                        logData.sensorA, logData.valueA,
                        logData.sensorB, logData.valueB,
                        logData.sensorC, logData.valueC);
            file.close(); 
            Serial.println("[SD Background Task] Wrote reading successfully.");
          } else {
            // Write failed — card likely removed or corrupted
            Serial.println("[SD Background Task] Error: Could not open datalog.csv! Marking disconnected.");
            SD.end(); // Unmount cleanly
            sdConnected = false;
          }
        } else {
          Serial.println("[SD] Card offline. Dropping log or saving to fallback queue.");
        }
        
    
    }
  }
}

void readSensors(void *pvParameters){
  // Sensors
  SensirionI2CSen5x SEN55;
  Adafruit_BME280 bme;
  //SCD30 CO2sensor;
  SCD4x CO2sensor(SCD4x_SENSOR_SCD41);

  initializeBME();
  initializeSCD41();
  initializeSEN55();

  while(1) {
    SensorReading reading;
    readBME(&reading);
    readSEN55(&reading);
    readSCD41(&reading);
    
    if (xQueueSend(sensorQueue, &reading, pdMS_TO_TICKS(0)) != pdPASS) {
      Serial.println("Queue full! Dropping sample");
    }

    vTaskDelay(pdMS_TO_TICKS(2000)); 
  }
}

void setup() {
  Serial.begin(115200);
  // Initialize both FreeRTOS Queues
  sensorQueue = xQueueCreate(QUEUE_SIZE, sizeof(SensorReading));
  sdQueue     = xQueueCreate(QUEUE_SIZE, sizeof(SensorReading));
  
  if (sensorQueue == NULL || sdQueue == NULL) {
    Serial.println("Queue allocation failed!");
    while(1);
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Spawn independent threads on Core 0
  xTaskCreatePinnedToCore(RTC, "Task_RTC", 3072, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(readSensors, "readSensors", 3070, NULL, 2, NULL, 0);
  
  // Note: Increased stack memory to 4096 bytes. File system libraries need room.
  xTaskCreatePinnedToCore(writingToSD, "Task_SD", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(HeartbeatTask, "Task_Heartbeat", 3072, NULL, 2, NULL, 0);
}

// --- Main Thread: HTTP Batching & Duplication Loop (Runs on Core 1) ---
void loop() {
  Serial.printf("Queue Usage: %d items waiting\n", uxQueueMessagesWaiting(sensorQueue));

  if (uxQueueMessagesWaiting(sensorQueue) >= BATCH_SIZE) {
    if (WiFi.status() == WL_CONNECTED) {
    
      for (int i = 0; i < BATCH_SIZE; i++) {
        data poppedData;
        
        // Pop the primary data stream out of the main queue
        if (xQueueReceive(sensorQueue, &poppedData, pdMS_TO_TICKS(5)) == pdPASS) {
          
          // --- CLONE TO SD QUEUE ---
          // Send the copied item directly into the background SD queue without waiting (0 tick timeout)
          if (xQueueSend(sdQueue, &poppedData, pdMS_TO_TICKS(0)) != pdPASS) {
            Serial.println("[Warning] Background SD queue full! Frame dropped from backup log.");
          }

          String oustr = String(sensorData.Tbme) + String(", ")  // BME
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

  vTaskDelay(pdMS_TO_TICKS(1000)); 
}
