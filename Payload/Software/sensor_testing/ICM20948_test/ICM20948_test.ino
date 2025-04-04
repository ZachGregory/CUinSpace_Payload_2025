#include <Wire.h>
#include "ICM_20948.h"

#define AD0_VAL 1
ICM_20948_I2C myICM;

float acc_x,acc_y,acc_z,gyro_x,gyro_y,gyro_z,mag_x,mag_y,mag_z;

void setup() {
  Serial.begin(115200);

  Wire.begin();
  Wire.setClock(400000);

  myICM.enableDebugging(); //enables "helpful" debugging messages (optional)

  myICM.begin(Wire, AD0_VAL);
  myICM.statusString();
  if (myICM.status != ICM_20948_Stat_Ok){Serial.println("Failed to initialize ICM");}
  if (myICM.initializeDMP() != ICM_20948_Stat_Ok) {Serial.println("Failed to initialize DMP");}
  delay(100);

}

void loop() {
  if (myICM.dataReady()) {
    icm_20948_DMP_data_t data;
    if (myICM.readDMPdataFromFIFO(&data) == ICM_20948_Stat_Ok) {
      // Extract the raw accelerometer data
      acc_x = (float)data.Raw_Accel.Data.X; 
      acc_y = (float)data.Raw_Accel.Data.Y;
      acc_z = (float)data.Raw_Accel.Data.Z;
      // Extract the raw gyro data
      gyro_x = (float)data.Raw_Gyro.Data.X; 
      gyro_y = (float)data.Raw_Gyro.Data.Y;
      gyro_z = (float)data.Raw_Gyro.Data.Z;
      // Extract the compass data
      mag_x = (float)data.Compass.Data.X; 
      mag_y = (float)data.Compass.Data.Y;
      mag_z = (float)data.Compass.Data.Z;

      // Print result
      Serial.print("Accel: ");
      Serial.print(acc_x); Serial.print(", ");
      Serial.print(acc_y); Serial.print(", ");
      Serial.println(acc_z);

      Serial.print("Gyro: ");
      Serial.print(gyro_x); Serial.print(", ");
      Serial.print(gyro_y); Serial.print(", ");
      Serial.println(gyro_z);

      Serial.print("Mag: ");
      Serial.print(mag_x); Serial.print(", ");
      Serial.print(mag_y); Serial.print(", ");
      Serial.println(mag_z);
      
      Serial.println();
    }
  } else {delay(10);} // Wait until data is ready
}
