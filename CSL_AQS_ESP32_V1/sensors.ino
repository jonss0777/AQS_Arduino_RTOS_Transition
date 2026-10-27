#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SensirionI2CSen5x.h> 
#include "SparkFun_SCD4x_Arduino_Library.h"
#include "CSL_AQS_ESP32_V1.h"


/* BME280 TEMP, PRESS, RH BOARD FROM ADAFRUIT USING 'Adafruit_BME280.h' */

void initializeBME() {
  Serial.print("Starting BME280 TPRH...\t\t");
  // You can also pass in a Wire library object like &Wire2
  // bmeStatus = bme.begin(0x76, &Wire2)
  if (!bme.begin()) {
    Serial.println("BME280 not detected");
    display.println("BME280 not detected");
    display.display();
  } else {
    Serial.println("BME280 connected");
    display.println("BME280 connected");
    display.display();
  }
}

void readBME(data &reading) {
  reading.Tbme = bme.readTemperature();
  reading.Pbme = bme.readPressure() / 100;  // for hPa
  reading.RHbme = bme.readHumidity();
}

/* SCD41 CO2 SENSOR FROM SENSIRION USING "SparkFun_SCD4x_Arduino_Library.h"*/

void initializeSCD41() {
  Serial.print("Starting SCD41 CO2...\t\t");
  //mySensor.enableDebugging(); // Uncomment this line to get helpful debug messages on Serial
  if (CO2sensor.begin(false, false, false) == false)  // measBegin, autoCal, skipStopPeriodicMeasurements i.e. Do not start periodic measurements
  {
    Serial.println("SCD41 sensor not detected");
    display.println("SCD41 sensor not detected");
    display.display();
  }
  if (!CO2sensor.measureSingleShot()) {
    Serial.println("SCD41 measureSingleShot failed");
    display.println("SCD41 measureSingleShot failed");
    display.display();
  } else
    Serial.println("SCD41 connected");
  display.println("SCD41 connected");
  display.display();
}

void readSCD41(data &reading) {
  CO2sensor.measureSingleShot();        // Request fresh data (should take 5 seconds)
  while (!CO2sensor.readMeasurement())  // readMeasurement will return true when fresh data is available
  {
    Serial.print("+");
    delay(500);
  }
  Serial.println();
  reading.CO2 = CO2sensor.getCO2();
  reading.Tco2 = CO2sensor.getTemperature();
  reading.RHco2 = CO2sensor.getHumidity();

}

/* SEN55 PM VOC NOX SENSOR FROM SENSIRION USING "SensirionI2CSen5x.h" */

#define MAXBUF_REQUIREMENT 48
#if ((defined(I2C_BUFFER_LENGTH) && (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT))
#define USE_PRODUCT_INFO
#endif

void initializeSEN55() {
  Serial.print("Starting SEN55 PM2.5...\t\t");
  Wire.begin();
  SEN55.begin(Wire);

  uint16_t error;
  char errorMessage[256];
  error = SEN55.deviceReset();
  if (error) {
    Serial.print("Error trying to execute deviceReset(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  float tempOffset = 0.0;
  error = SEN55.setTemperatureOffsetSimple(tempOffset);
  if (error) {
    Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    //Serial.printf("T offset %.2f C...\t", tempOffset);
  }
  error = SEN55.startMeasurement();
  if (error) {
    Serial.print("Error trying to execute startMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    Serial.println("SEN55 connected");
    display.println("SEN55 connected");
    display.display();
  }
}

void readSEN55(data &reading) {
  uint16_t error;
  char errorMessage[256];
  delay(1000);

  error = SEN55.readMeasuredValues( 
            reading.mPm1_0,
            reading.mPm2_5,
            reading.mPm4_0,
            reading.mPm10,
            reading.RHsen,
            reading.Tsen,
            reading.VOCs,
            reading.NOx );

  if (error) {
    Serial.print("Error trying to execute readMeasuredValues(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  error = SEN55.readMeasuredPmValues( 
            reading.mPm1_0,
            reading.mPm2_5,
            reading.mPm4_0,
            reading.mPm10,            
            reading.cPm0_5,
            reading.cPm1_0,
            reading.cPm2_5,
            reading.cPm4_0,
            reading.cPm10,            
            reading.tpSize);

  if (error) {
    Serial.print("Error trying to execute readMeasuredPmValues(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
 
}