/*
  Wifi secure connection example for ESP32
  Running on TLS 1.2 using mbedTLS
  Suporting the following chipersuites:
*/

#include "CSL_AQS_ESP32_V1.h"

// #define PRE_PAYLOAD_APPEND_ROW "{\"command\":\"appendRow\",\"sheet_name\":\"Sheet1\",\"values\":\""
// #define PRE_PAYLOAD_ADD_HEADER "{\"command\":\"addHeader\",\"sheet_name\":\"Sheet1\",\"values\":\""
#define POST_PAYLOAD "\"}"
// --- Add your heartbeat server address ---

// Google Certificate
const char* test_root_ca = R"string_literal(
// -----BEGIN CERTIFICATE(Old/Previous)-----
// MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw
// CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU
// MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw
// MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp
// Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA
// A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo
// 27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w
// Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw
// TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl
// qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH
// szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8
// Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk
// MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92
// wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p
// aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN
// VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID
// AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E
// FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb
// C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe
// QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy
// h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4
// 7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J
// ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef
// MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/
// Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT
// 6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ
// 0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm
// 2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb
// bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c
// -----END CERTIFICATE-----

-----BEGIN CERTIFICATE-----
MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD
VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG
A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw
WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz
IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi
AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi
QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR
HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW
BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D
9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8
p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD
-----END CERTIFICATE-----


)string_literal";



void initializeClient() {
  client.setCACert(test_root_ca);
}

// --- Payload Upload (Runs independently on Core 0) ---
void doPost(String outstr) {

  if (uxQueueMessagesWaiting(wclientQueue) >= WIFI_QUEUE_BATCH_SIZE) {

    String payload = String(PRE_PAYLOAD_APPEND_ROW) + outstr + POST_PAYLOAD;
    Serial.println(payload);

    Serial.print("\nStarting connection to server... ");
    if (!client.connect(SERVER, 443))
      Serial.println("Connection failed");
    else {
      Serial.println("Connected to server");
      // Make a HTTP request:
      // client.println("POST /macros/s/AKfycbxwxxCaHA24OhuHJWrZQ79a6qOfYCm4-fPbDFGRt9JSZEGv345UuFR-kJw6Sgv7wZq3Qw/exec? HTTP/1.0");//value=Hello HTTP/1.0");
      client.println("POST /macros/s/1m-DzGKNjXxQZ0cHaV1RwegLUvsFRtjrnix6AFdpQs737rCze7R-ehQbb/exec? HTTP/1.0");  //value=Hello HTTP/1.0");
      client.println("Host: " SERVER);
      client.println("Content-Type: application/x-www-form-urlencoded");
      //client.println("Connection: close");
      client.print("Content-Length: ");
      client.println(payload.length());
      client.println();
      //Serial.println(String(PRE_PAYLOAD) + String(payload) + String(POST_PAYLOAD));
      client.print(payload);
      client.println();
      delay(200);
      Serial.println("\nResponse from client: ");

      while (client.connected()) {
        while (client.available()) {
          char c = client.read();
          Serial.write(c);
        }
      }
      client.stop();
    }
  }
}

// --- Heartbeat Task (Runs independently on Core 0) ---
void sendHeartbeat() {
  Serial.println("Initialize Heartbeat Task");
  String payload = String(PRE_PAYLOAD_APPEND_ROW) + "Alive" + POST_PAYLOAD;
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nStarting connection to server... ");
    if (!client.connect(SERVER, 443)) {
      Serial.println("Connection failed");
    } else {
      Serial.println("Connected to server");
      client.println("POST /macros/s/1m-DzGKNjXxQZ0cHaV1RwegLUvsFRtjrnix6AFdpQs737rCze7R-ehQbb/exec? HTTP/1.0");  // value=Hello HTTP/1.0");
      client.println("Host: " SERVER);
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.println("Connection: close");
      client.print("Content-Length: ");
      client.println(payload.length());
      client.println();
      client.print(payload);
      client.println();
      delay(200);
      //Serial.println("\nResponse from client: ");
      // Read Response
      //   while (client.connected()) {
      //     while (client.available()) {
      //       char c = client.read();
      //       Serial.write(c);
      //     }
      //   }
      client.stop();
    }
  } else {
    Serial.println("[Heartbeat] Wi-Fi disconnected, skipping pulse.");
  }
}

String preparePayload(data& sensorData) {
  return (sensorData.now + String(", ")             // Datetime
          + String(sensorData.Tbme) + String(", ")  // BME
          + String(sensorData.Pbme) + String(", ")
          + String(sensorData.RHbme) + String(", ")

          + String(sensorData.CO2) + String(", ")  // SCD41
          + String(sensorData.Tco2) + String(", ")
          + String(sensorData.RHco2) + String(", ")

          + String(sensorData.mPm1_0) + String(", ")  // SEN55
          + String(sensorData.mPm2_5) + String(", ")
          + String(sensorData.mPm4_0) + String(", ")
          + String(sensorData.mPm10) + String(", ")

          + String(sensorData.cPm0_5) + String(", ")
          + String(sensorData.cPm1_0) + String(", ")
          + String(sensorData.cPm2_5) + String(", ")
          + String(sensorData.cPm4_0) + String(", ")
          + String(sensorData.cPm10) + String(", ")
          + String(sensorData.tpSize) + String(", ")

          + String(sensorData.RHsen) + String(", ")
          + String(sensorData.Tsen) + String(", ")
          + String(sensorData.VOCs) + String(", ")
          + String(sensorData.NOx) + String(", "));
}