#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_HTU21DF.h" // Humidity sensor
#include <Adafruit_TMP117.h> // Temperature sensor
#include <pas-co2-ino.hpp> // CO2 sensor
#include "FastIMU.h" // IMU sensor

#define I2C_FREQ_HZ  400000                     
#define PERIODIC_MEAS_INTERVAL_IN_SECONDS  10 /* demo-mode value; not recommended for long-term measurements */
// #define PERIODIC_MEAS_INTERVAL_IN_SECONDS 60L /* specification value for stable operation (uncomment for long-time-measurements) */
#define PRESSURE_REFERENCE  900

#define IMU_ADDRESS 0x68    //Change to the address of the IMU
//#define PERFORM_CALIBRATION //Calibration method not added here
MPU6500 IMU;               //Change to the name of any supported IMU! 

Adafruit_HTU21DF htu = Adafruit_HTU21DF();
Adafruit_TMP117 tmp;
PASCO2Ino co2;
AccelData accelData; 
GyroData gyroData;
calData calib = { 0 };  //Calibration data for IMU

unsigned long time_last_co2_read;
Error_t err;


void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10); // ensure Serial is setup

  /* Initialize the i2c interface used by the sensor */
  Wire.begin();
  Wire.setClock(I2C_FREQ_HZ);

  // initalize sensors
  if (!htu.begin()) Serial.println("Error initializing HTU sensor");
  if (!tmp.begin()) Serial.println("Error initializing TMP sensor");

  err = co2.begin();
  if (XENSIV_PASCO2_OK != err) Serial.println("Error initializing CO2 sensor");
  err = co2.setPressRef(PRESSURE_REFERENCE); //Set reference pressure
  if (XENSIV_PASCO2_OK != err) Serial.println("Refrence pressure error CO2 sensor");
  err = co2.startMeasure(PERIODIC_MEAS_INTERVAL_IN_SECONDS); // Configure sensor ro measure every 60 seconds
  if (XENSIV_PASCO2_OK != err) Serial.println("Error with start measure CO2 sensor");  

  err = IMU.init(calib, IMU_ADDRESS);
  if (err != 0) Serial.println("Error initializing IMU sensor");

  delay(1000); // Give time for everything to setup
  time_last_co2_read = millis();
}

void loop() {
  // Get humidity
  float rel_hum = htu.readHumidity();

  // Get temperature
  sensors_event_t temp_sensor; // create an empty event to be filled
  tmp.getEvent(&temp_sensor); //fill the empty event object with the current measurements
  float temp = temp_sensor.temperature;

  // Get CO2 ppm
  int16_t co2ppm;
  if (millis() - time_last_co2_read > PERIODIC_MEAS_INTERVAL_IN_SECONDS*1000){ // Wait for the value to be ready
    err = co2.getCO2(co2ppm);
    if (XENSIV_PASCO2_OK != err){
      // Retry in case of timing synch mismatch
      if (XENSIV_PASCO2_ERR_COMM == err){
        delay(600);
        err = co2.getCO2(co2ppm);
        if (XENSIV_PASCO2_OK != err){
          Serial.print("get co2 error: ");
          Serial.println(err);
        }
      }
      time_last_co2_read = millis();
    }
  }

  IMU.update();
  IMU.getAccel(&accelData);
  IMU.getGyro(&gyroData);

  delay(500);
}