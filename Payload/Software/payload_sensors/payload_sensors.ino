#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "ICM_20948.h"

#define SEALEVELPRESSURE_HPA (1013.25)
#define AD0_VAL 1

Adafruit_BME680 bme(&Wire);
ICM_20948_I2C myICM;

String debugMsg;

const float vcc = 5.0;
const int adcMax = 1023;
const float sens = 0.185;  // 5A
float current = 0;

float getAverageCurrent(int nSamples) {
  float val = 0;
  for (int i = 0; i < nSamples; i++) {
    val += analogRead(A0);
    delay(1);
  }
  float avgOutput = val / adcMax / nSamples;
  return (vcc / 2 - vcc * avgOutput) / sens;
}

icm_20948_DMP_data_t ICMdata;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10); // ensure Serial is setup

  Wire.begin();
  Wire.setClock(400000);

  // BME sensor initialization
  if (!bme.begin()) { Serial.println("Failed to initialize BME");}
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  // ICM sensor initialization
  myICM.begin(Wire, AD0_VAL);
  if (myICM.status != ICM_20948_Stat_Ok){Serial.println("Failed to initialize ICM");}
  if (myICM.initializeDMP() != ICM_20948_Stat_Ok) {Serial.println("Failed to initialize DMP");}
}

void loop() {
  debugMsg = "Debug Errors: ";
  if (!bme.performReading()){debugMsg += "Error reading BME\t";}

  if (myICM.dataReady()) {
    if (myICM.readDMPdataFromFIFO(&ICMdata) != ICM_20948_Stat_Ok) {debugMsg += "Error reading ICM\t";}
  } else {debugMsg += "ICM data not ready\t";}

  current = getAverageCurrent(100);

  printData();
  
  delay(100);
}

void printData(){
  Serial.print("Temperature (C):");
  Serial.print(bme.temperature);
  Serial.print("\t");

  Serial.print("Pressure (Pa):");
  Serial.print(bme.pressure);
  Serial.print("\t");

  Serial.print("Humidity (%):");
  Serial.print(bme.humidity);
  Serial.print("\t");

  Serial.print("Gas (Ohms):");
  Serial.print(bme.gas_resistance);
  Serial.print("\t");

  Serial.print("Altitude (m):");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.print("\t");

  Serial.print("Accel X:");
  Serial.print((float)ICMdata.Raw_Accel.Data.X); 
  Serial.print("\t");
  Serial.print("Accel Y:");
  Serial.print((float)ICMdata.Raw_Accel.Data.Y); 
  Serial.print("\t");
  Serial.print("Accel Z:");
  Serial.print((float)ICMdata.Raw_Accel.Data.Z); 
  Serial.print("\t");

  Serial.print("Gyro X:");
  Serial.print((float)ICMdata.Raw_Gyro.Data.X); 
  Serial.print("\t");
  Serial.print("Gyro Y:");
  Serial.print((float)ICMdata.Raw_Gyro.Data.Y); 
  Serial.print("\t");
  Serial.print("Gyro Z:");
  Serial.print((float)ICMdata.Raw_Gyro.Data.Z); 
  Serial.print("\t");

  Serial.print("Mag X:");
  Serial.print((float)ICMdata.Compass.Data.X); 
  Serial.print("\t");
  Serial.print("Mag Y:");
  Serial.print((float)ICMdata.Compass.Data.Y); 
  Serial.print("\t");
  Serial.print("Mag Z:");
  Serial.print((float)ICMdata.Compass.Data.Z); 
  Serial.print("\t");

  Serial.print("Current (A):");
  Serial.print(current);

  Serial.println(debugMsg);
}