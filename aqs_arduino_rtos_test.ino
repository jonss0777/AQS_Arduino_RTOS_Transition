// get local host in widows 
// open cmd and type "ipconfig"

// linux 
// open terminal and type "hostname -I"

#include <WiFi.h>
#include <WiFiClient.h> 
#include <HTTPClient.h>
#include <ArduinoJson.h> // Optimization: Structured memory management - ArduinoJson uses a fixed size data structure 
#include <time.h>
#include <FS.h>          // Added for SD card file system support
#include <SD.h>          // Added for SD card hardware interaction

// --- Hardware Pin Configuration ---
//#define SD_CS_PIN 5       // Standard ESP32 SPI Chip Select pin for SD cards. Change if your shield differs.

// --- FreeRTOS Synchronization ---
SemaphoreHandle_t sdMutex; // Mutex to protect SPI bus and SD filesystem across cores

// --- Network Configurations ---
const char* ssid     = "";
const char* password = "";
const char* serverAddress = "http://localIPAddress:3002/api/sensor-reading"; 

// --- Data Structures ---
struct SensorReading {
  char sensorA;  
  float valueA;   
  char sensorB;   
  float valueB;  
  char sensorC;   
  float valueC;  
};

// --- Queue Configuration ---
#define QUEUE_SIZE 100       // Optimization: Increased capacity buffer
#define BATCH_SIZE 10      
QueueHandle_t sensorQueue;  
QueueHandle_t sdQueue;       // Added: Dedicated queue for background SD card logging


// --- Add your heartbeat server address ---
const char* heartbeatAddress = "http://localIPAddress:3002/api/heartbeat";

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
  SensorReading logData;
  bool sdConnected = true; // Track state

  while(1) {
    if (xQueueReceive(sdQueue, &logData, pdMS_TO_TICKS(1000)) == pdPASS) {
      
      if (xSemaphoreTake(sdMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        
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
        
        xSemaphoreGive(sdMutex);
      }
    }
  }
}

void readSensors(void *pvParameters){
  Serial.println("Initialize SCD41");
  Serial.println("Initialize BME280");
  Serial.println("Initialize Sen55");
  
  while(1) {
    SensorReading reading;
    reading.sensorA = 'A';
    reading.valueA = getRandomVoltage();
    vTaskDelay(pdMS_TO_TICKS(100)); 

    reading.sensorB = 'B';
    reading.valueB = getRandomVoltage();
    vTaskDelay(pdMS_TO_TICKS(200)); 
  
    reading.sensorC = 'C';
    reading.valueC = getRandomVoltage();
    vTaskDelay(pdMS_TO_TICKS(200));

    if (xQueueSend(sensorQueue, &reading, pdMS_TO_TICKS(0)) != pdPASS) {
      Serial.println("Queue full! Dropping sample");
    }
    vTaskDelay(pdMS_TO_TICKS(2000)); 
  }
}

void setup() {
  Serial.begin(115200);

  // Create the FreeRTOS Mutex
  sdMutex = xSemaphoreCreateMutex();
  if (sdMutex == NULL) {
    Serial.println("Failed to create SD mutex!");
    while(1);
  }

  // Initialize both FreeRTOS Queues
  sensorQueue = xQueueCreate(QUEUE_SIZE, sizeof(SensorReading));
  sdQueue     = xQueueCreate(QUEUE_SIZE, sizeof(SensorReading));
  
  if (sensorQueue == NULL || sdQueue == NULL) {
    Serial.println("Queue allocation failed!");
    while(1);
  }

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

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Spawn independent threads on Core 0
  xTaskCreatePinnedToCore(LED, "Task_LED", 3072, NULL, 2, NULL, 0);
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
      WiFiClient client; 
      HTTPClient http;
      
      http.begin(client, serverAddress);
      http.addHeader("Content-Type", "application/json");

      DynamicJsonDocument doc(1024);
      JsonArray readingsArray = doc.createNestedArray("readings");
      doc["macAddress"] = WiFi.macAddress();
      
      for (int i = 0; i < BATCH_SIZE; i++) {
        SensorReading poppedData;
        
        // Pop the primary data stream out of the main queue
        if (xQueueReceive(sensorQueue, &poppedData, pdMS_TO_TICKS(5)) == pdPASS) {
          
          // --- CLONE TO SD QUEUE ---
          // Send the copied item directly into the background SD queue without waiting (0 tick timeout)
          if (xQueueSend(sdQueue, &poppedData, pdMS_TO_TICKS(0)) != pdPASS) {
            Serial.println("[Warning] Background SD queue full! Frame dropped from backup log.");
          }

          // Build the JSON payload as normal
          JsonObject readingObject = readingsArray.createNestedObject();
          readingObject["sensorA"] = String(poppedData.sensorA);
          readingObject["valA"] = serialized(String(poppedData.valueA, 2)); 
          readingObject["sensorB"] = String(poppedData.sensorB);
          readingObject["valB"] = serialized(String(poppedData.valueB, 2)); 
          readingObject["sensorC"] = String(poppedData.sensorC);
          readingObject["valC"] = serialized(String(poppedData.valueC, 2)); 
        }
      }
      
      String jsonPayload;
      serializeJson(doc, jsonPayload);

      Serial.print("Sending Optimized HTTP Batch: ");
      Serial.println(jsonPayload);

      int httpResponseCode = http.POST(jsonPayload);
      if (httpResponseCode > 0) {
        Serial.printf("HTTP Success Status: %d\n", httpResponseCode);
        String responseBody = http.getString();
        Serial.println("Server says: " + responseBody);
      } else {
        Serial.printf("HTTP Error: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      http.end();
    } else {
      Serial.println("WiFi Disconnected. Waiting to batch.");
    }
  }

  vTaskDelay(pdMS_TO_TICKS(1000)); 
}
