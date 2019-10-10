#include "I2Cdev.h"
#include "MPU6050.h"
#include "SparkFunLSM6DS3.h"

#define IMU_COUNT_PER_MODEL 2
#define IMU_MODEL_COUNT 2
#define AXIS_COUNT 3

#define printfloatx(Name,Variable,Spaces,Precision,EndTxt) { Serial.print(F(Name)); {char S[(Spaces + Precision + 3)];Serial.print(F(" ")); Serial.print(dtostrf((float)Variable,Spaces,Precision ,S));}Serial.print(F(EndTxt)); }//Name,Variable,Spaces,Precision,EndTxt

/***************************************************************************
 * Calibration Instructions
 * - Allow all IMUs to run for ~10 minutes before calibration
 * - Lie all IMUs flat and make sure gravity points in the positive z-axis 
 *   (uncalibrated raw z-acceleration values are positive)
 * - Change the ideal 1g raw acceleration value constant in CalibrateAccel()
 *   inside MPU6050.cpp according to the set full scale range of the 
 *   accelerometer
 * - Open the Serial Monitor when ready to calibrate and prevent from 
 *   introducing vibrations in the process
 * - Ideal offset values for each IMU are the values printed after 1000 
 *   total readings
 ***************************************************************************/

MPU6050 mpu6050[IMU_COUNT_PER_MODEL] = { MPU6050(0x68), MPU6050(0x69) };
LSM6DS3 lsm6ds3[IMU_COUNT_PER_MODEL] = { LSM6DS3(I2C_MODE, 0x6B), LSM6DS3(I2C_MODE, 0x6A) };

/*********************************
 * Unified IMU index:
 *    0 - MPU6050 0x68  (default)
 *    1 - MPU6050 0x69
 *    2 - LSM6DS3 0x6B  (default)
 *    3 - LSM6DS3 0x6A
 *********************************/
bool lsm6ds3ConnState[IMU_COUNT_PER_MODEL] = { false, false };
bool mpu6050ConnState[IMU_COUNT_PER_MODEL] = { false, false };

int16_t lsm6ds3GravityAccZOffset[IMU_MODEL_COUNT] = { 0, 0 };     // assuming gravity points in the positive-z axis
int16_t lsm6ds3AccOffsets[IMU_MODEL_COUNT][AXIS_COUNT] = { { 0, 0, 0 }, { 0, 0, 0 } };
int16_t lsm6ds3GyroOffsets[IMU_MODEL_COUNT][AXIS_COUNT] = { { 0, 0, 0 }, { 0, 0, 0 } };

void initMpu6050(uint8_t);
void initLsm6ds3(uint8_t);
void calibMpu6050(uint8_t);
void calibLsm6ds3(uint8_t);
void calcLsm6ds3Offsets(uint8_t, int);
void printLsm6ds3Offsets(uint8_t);

void setup() {

  Wire.begin();
  Wire.setClock(400000);
  Serial.begin(115200);
  
  delay(2000);

  for (uint8_t imuIndex=0; imuIndex<IMU_COUNT_PER_MODEL; ++imuIndex) {
    initMpu6050(imuIndex);
    initLsm6ds3(imuIndex);
  }

  delay(2000);

  for (uint8_t imuIndex=0; imuIndex<IMU_COUNT_PER_MODEL; ++imuIndex) {
    if ( !(lsm6ds3ConnState[imuIndex] && mpu6050ConnState[imuIndex]) ) {
      Serial.println("Task killed");
      delay(100);
      exit(1);
    }
  }

  for (uint8_t imuIndex=0; imuIndex<IMU_COUNT_PER_MODEL; ++imuIndex) {
    calibMpu6050(imuIndex);
    calibLsm6ds3(imuIndex);
  }
  
}

// this loop gives a system bandwidth of ~71 Hz (~14ms per iteration)
void loop()
{
  
}

void initMpu6050(uint8_t imuIndex) {
  Serial.print("Initializing MPU6050 ");
  Serial.println(imuIndex);
  
  mpu6050[imuIndex].initialize();
  mpu6050[imuIndex].setRate(4);     // divisor. Gyroscope sampling rate = 1 kHz (if DLPF is enabled, else 8kHz) / (1 + divisor)
  mpu6050[imuIndex].setDLPFMode(MPU6050_DLPF_BW_42);
  mpu6050[imuIndex].setFullScaleGyroRange(MPU6050_GYRO_FS_1000);
  mpu6050[imuIndex].setFullScaleAccelRange(MPU6050_ACCEL_FS_4);   // Remember to change 1g raw value constant in CalibrateAccel()
  
  Serial.print("Testing connection for MPU6050 ");
  Serial.println(imuIndex);

  mpu6050ConnState[imuIndex] = mpu6050[imuIndex].testConnection();
  if (mpu6050ConnState[imuIndex]) {
    Serial.print("MPU6050 ");
    Serial.print(imuIndex);
    Serial.println(" connection successful");
  }
  else {
    Serial.print("MPU6050 ");
    Serial.print(imuIndex);
    Serial.println(" connection failed");
  } 
    
}

void initLsm6ds3(uint8_t imuIndex) {

  Serial.print("Initializing LSM6DS3 ");
  Serial.println(imuIndex);

  lsm6ds3[imuIndex].settings.gyroRange = 1000;      // max deg/s. Can be: 125, 245, 500, 1000, 2000
  lsm6ds3[imuIndex].settings.gyroBandWidth = 50;    // Hz. Can be: 50, 100, 200, 400;
  lsm6ds3[imuIndex].settings.gyroSampleRate = 208;  // Hz. Can be: 13, 26, 52, 104, 208, 416, 833, 1666
  lsm6ds3[imuIndex].settings.accelRange = 4;        // max g-force. Can be: 2, 4, 8, 16
  lsm6ds3[imuIndex].settings.accelBandWidth = 50;   // Hz. Can be: 50, 100, 200, 400;  
  lsm6ds3[imuIndex].settings.accelSampleRate = 208; // Hz. Can be: 13, 26, 52, 104, 208, 416, 833, 1666, 3332, 6664, 13330
  lsm6ds3[imuIndex].settings.tempEnabled = 0;
  
  lsm6ds3ConnState[imuIndex] = (lsm6ds3[imuIndex].begin() == IMU_SUCCESS);

  Serial.print("Testing connection for LSM6DS3 ");
  Serial.println(imuIndex);
    
  if (lsm6ds3ConnState[imuIndex]) {
    Serial.print("LSM6DS3 ");
    Serial.print(imuIndex);
    Serial.println(" connection successful");
  }
  else {
    Serial.print("LSM6DS3 ");
    Serial.print(imuIndex);
    Serial.println(" connection failed");
  }
      
}

void calibMpu6050(uint8_t imuIndex) {

  Serial.print("Calibrating MPU6050 ");
  Serial.println(imuIndex);
  
  Serial.println("PID tuning, each dot = 100 readings");
  
  mpu6050[imuIndex].CalibrateAccel(6);
  mpu6050[imuIndex].CalibrateGyro(6);
  Serial.println("600 Readings");
  mpu6050[imuIndex].PrintActiveOffsets();
  Serial.println();
  
  mpu6050[imuIndex].CalibrateAccel(1);
  mpu6050[imuIndex].CalibrateGyro(1);
  Serial.println("700 Total Readings");
  mpu6050[imuIndex].PrintActiveOffsets();
  Serial.println();
  
  mpu6050[imuIndex].CalibrateAccel(1);
  mpu6050[imuIndex].CalibrateGyro(1);
  Serial.println("800 Total Readings");
  mpu6050[imuIndex].PrintActiveOffsets();
  Serial.println();
  
  mpu6050[imuIndex].CalibrateAccel(1);
  mpu6050[imuIndex].CalibrateGyro(1);
  Serial.println("900 Total Readings");
  mpu6050[imuIndex].PrintActiveOffsets();
  Serial.println();
  
  mpu6050[imuIndex].CalibrateAccel(1);
  mpu6050[imuIndex].CalibrateGyro(1);
  Serial.println("1000 Total Readings");
  mpu6050[imuIndex].PrintActiveOffsets();
  Serial.println();
  
}

void calibLsm6ds3(uint8_t imuIndex) {

  Serial.print("Calibrating LSM6DS3 ");
  Serial.println(imuIndex);  

  calcLsm6ds3Offsets(imuIndex, 600);
  Serial.println("600 Readings");
  printLsm6ds3Offsets(imuIndex);
  Serial.println();

  calcLsm6ds3Offsets(imuIndex, 100);
  Serial.println("700 Total Readings");
  printLsm6ds3Offsets(imuIndex);
  Serial.println();

  calcLsm6ds3Offsets(imuIndex, 100);
  Serial.println("800 Total Readings");
  printLsm6ds3Offsets(imuIndex);
  Serial.println();  

  calcLsm6ds3Offsets(imuIndex, 100);
  Serial.println("900 Total Readings");
  printLsm6ds3Offsets(imuIndex);
  Serial.println();  

  calcLsm6ds3Offsets(imuIndex, 100);
  Serial.println("1000 Total Readings");
  printLsm6ds3Offsets(imuIndex);
  Serial.println();        

}

void calcLsm6ds3Offsets(uint8_t imuIndex, int calibLoopCount) {

  lsm6ds3GravityAccZOffset[imuIndex] = (int16_t) (((int32_t) 1 << 16) / (2 * (int32_t) lsm6ds3[imuIndex].settings.accelRange));  // lsb/g

  // simple averaging to calculate ideal offsets
  int32_t sumAcc[AXIS_COUNT] = { 0, 0, 0 };
  int32_t sumGyro[AXIS_COUNT] = { 0, 0, 0 };

  Serial.print('>');
  for (int index=0; index<calibLoopCount; ++index) {
    sumAcc[0] += lsm6ds3[imuIndex].readRawAccelX() - lsm6ds3AccOffsets[imuIndex][0];
    sumAcc[1] += lsm6ds3[imuIndex].readRawAccelY() - lsm6ds3AccOffsets[imuIndex][1];
    sumAcc[2] += lsm6ds3[imuIndex].readRawAccelZ() - lsm6ds3AccOffsets[imuIndex][2] - lsm6ds3GravityAccZOffset[imuIndex];
    sumGyro[0] += lsm6ds3[imuIndex].readRawGyroX() - lsm6ds3GyroOffsets[imuIndex][0];
    sumGyro[1] += lsm6ds3[imuIndex].readRawGyroY() - lsm6ds3GyroOffsets[imuIndex][1];
    sumGyro[2] += lsm6ds3[imuIndex].readRawGyroZ() - lsm6ds3GyroOffsets[imuIndex][2];

    if (index % 100 == 99)
      Serial.print('.');
  }

  for (int axisIndex=0; axisIndex<AXIS_COUNT; ++axisIndex) {
    lsm6ds3AccOffsets[imuIndex][axisIndex] += (int16_t) (sumAcc[axisIndex] / calibLoopCount);
    lsm6ds3GyroOffsets[imuIndex][axisIndex] += (int16_t) (sumGyro[axisIndex] / calibLoopCount);
  }
  
}

void printLsm6ds3Offsets(uint8_t imuIndex) {
  
  Serial.print("\n//           X Accel  Y Accel  Z Accel   X Gyro   Y Gyro   Z Gyro\n//OFFSETS   ");
  printfloatx("", lsm6ds3AccOffsets[imuIndex][0], 5, 0, ",  ");
  printfloatx("", lsm6ds3AccOffsets[imuIndex][1], 5, 0, ",  ");
  printfloatx("", lsm6ds3AccOffsets[imuIndex][2], 5, 0, ",  ");
  printfloatx("", lsm6ds3GyroOffsets[imuIndex][0], 5, 0, ",  ");
  printfloatx("", lsm6ds3GyroOffsets[imuIndex][1], 5, 0, ",  ");
  printfloatx("", lsm6ds3GyroOffsets[imuIndex][2], 5, 0, ",  ");  
  
}
