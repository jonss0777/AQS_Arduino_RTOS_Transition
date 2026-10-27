#include "CSL_AQS_ESP32_V1.h"

/* INITIALIZE SD CARD */
void writingToSD() {

  if (!SD.begin()) {  // see if the card is present and can be initialized:
    Serial.println("SD card mount failed");
    return;
  } else {
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
      Serial.println("No SD card attached");
      return;
    }

    Serial.print("Starting SD card...\t\t");

    char filename[] = "/LOG0000.TXT";  // create a new file//increase 0000
    bool fileCreated = false;
    for (uint16_t i = 0; i <= 9999; i++) {
      filename[4] = i / 1000 + '0';  //48 ascii character value of 0 //integer division by
      filename[5] = i / 100 - 10 * (i / 1000) + '0';
      filename[6] = i / 10 - 10 * (i / 100) + '0';  //integer division 100
      filename[7] = i % 10 + '0';                   //modulo 10
      if (!SD.exists(filename)) {                   // only open a new file if it doesn't exist
        logfile = SD.open(filename, FILE_WRITE);
        // TODO: Set up header

        //if (file) {
          //file.println()
          //logfile.close();
        //}
        fileCreated = true;
        Serial.printf("Logging to file: %s\n",filename);
        break;  // leave the loop
      }
    }

    if (!fileCreated) {
      Serial.println("Couldn't create file");
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
          File file = SD.open(filename, FILE_APPEND);
          if (file) {
            file.printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2.2f\n", 
                        logData.Tbme, logData.Pbme,logData.RHbme,
                        logData.CO2, logData.Tco2, logData.RHco2, 
                        logData.mPm1_0, logData.mPm2_5, logData.mPm4_0, logData.mPm10, logData.cPm0_5, logData.cPm1_0, logData.cPm2_5, 
                        sensorData.cPm2_5) + String(", ") 
                      sensorData.cPm4_0) + String(", ")
                      sensorData.cPm10)  + String(", ")
                      + StrsensorData.tpSize) + String(", ")

                      + String(sensorData.RHsen) + String(", ") 
                      + String(sensorData.Tsen)  + String(", ") 
                      + String(sensorData.VOCs)  + String(", ") 
                      + String(sensorData.NOx)   + String(", ");  );
            file.close(); 
            Serial.println("[SD Background Task] Wrote reading successfully.");
          } else {
            // Write failed — card likely removed or corrupted
            Serial.println("[SD Background Task] Error: Could not open file! Marking disconnected.");
            SD.end(); // Unmount cleanly
            sdConnected = false;
          }
        } else {
          Serial.println("[SD] Card offline. Dropping log or saving to fallback queue.");
        }
    }
  vTaskDelay(pdMS_TO_TICKS(2000));    
  }
}

/* INITIALIZE RTC */
void initializeRTC() {
  RTC_PCF8523 rtc;                                                 // Real Time Clock for RevB Adafruit logger shield

  Serial.print("Starting RTC...\t\t\t");
  Wire.begin();  // connect to RTC
  if (!rtc.begin()) {
    Serial.println("RTC failed");
    display.println("RTC failed");
    display.display();
  } else {
    Serial.println("RTC connected");
    display.println("RTC connected");
    
    DateTime now= rtc.now(); //UPDATE JULY 29

    char tstring[128];
    sprintf(tstring, "%02u/%02u/%02u %02u:%02u:%02u", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    Serial.printf("Date time...\t\t\t%s\n",tstring);
    display.println(tstring);
    display.display();
  }
   //TO SET TIME at compile : run once to syncro then run again with line commented out
   //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
}

void syncRTCFromNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Skipping RTC sync: WiFi not connected");
    return;
  }

  Serial.println("Syncing RTC from NTP...");

  //configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  configTime(-5 * 3600, 3600, "pool.ntp.org", "time.nist.gov"); // added daylight savings

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10000)) {
    Serial.println("RTC sync failed: no NTP time received");
    return;
  }

  DateTime ntpTime(
    timeinfo.tm_year + 1900,
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  );

  DateTime rtcTime = rtc.now();

  int32_t driftSeconds = (int32_t)((int64_t)ntpTime.unixtime() - (int64_t)rtcTime.unixtime());

  Serial.printf("RTC drift: %ld seconds\n", (long)driftSeconds);

  if (driftSeconds > 5 || driftSeconds < -5) {
    rtc.adjust(ntpTime);
    Serial.println("RTC adjusted from NTP");
  } else {
    Serial.println("RTC already close enough");
  }
}