#include "I2Cdev.h"
#include "MPU6050.h"
#include "SparkFunLSM6DS3.h"

#define LED_PIN 13
#define LED_TOGGLE_PERIOD 250 // ms 
#define IMU_COUNT_PER_MODEL 2
#define IMU_MODEL_COUNT 2
#define AXIS_COUNT 3

MPU6050 mpu6050[IMU_COUNT_PER_MODEL] = { MPU6050(0x68), MPU6050(0x69) };
LSM6DS3 lsm6ds3[IMU_COUNT_PER_MODEL] = { LSM6DS3(I2C_MODE, 0x6B), LSM6DS3(I2C_MODE, 0x6A) };

// Sensor offsets obtained from calibration 
int16_t mpu6050AccOffsets[IMU_MODEL_COUNT][AXIS_COUNT] 
  = { { -2170, -284, 1178 }, 
      { -1940, 1550, 1800 } };
int16_t mpu6050GyroOffsets[IMU_MODEL_COUNT][AXIS_COUNT] 
  = { { 69, -51, -1 }, 
      { 48, -37, 5 } };
int16_t lsm6ds3AccOffsets[IMU_MODEL_COUNT][AXIS_COUNT] 
  = { { 302, -183, -81 }, 
      { 410, -308, 22 } };
int16_t lsm6ds3GyroOffsets[IMU_MODEL_COUNT][AXIS_COUNT] 
  = { { -20, -77, -4 }, 
      { -5, -225, -77 } };

/*********************************
 * Unified IMU index:
 *    0 - MPU6050 0x68  (default)
 *    1 - MPU6050 0x69
 *    2 - LSM6DS3 0x6B  (default)
 *    3 - LSM6DS3 0x6A
 *********************************/
int16_t accelerationX[IMU_MODEL_COUNT * IMU_COUNT_PER_MODEL];
int16_t accelerationY[IMU_MODEL_COUNT * IMU_COUNT_PER_MODEL];
int16_t accelerationZ[IMU_MODEL_COUNT * IMU_COUNT_PER_MODEL];
int16_t angularVelocityX[IMU_MODEL_COUNT * IMU_COUNT_PER_MODEL];
int16_t angularVelocityY[IMU_MODEL_COUNT * IMU_COUNT_PER_MODEL];
int16_t angularVelocityZ[IMU_MODEL_COUNT * IMU_COUNT_PER_MODEL];

bool lsm6ds3ConnState[IMU_COUNT_PER_MODEL] = { false, false };
bool mpu6050ConnState[IMU_COUNT_PER_MODEL] = { false, false };
bool ledBlinkState = false;
unsigned long prevBlinkMs = 0;

void initMpu6050(uint8_t);
void initLsm6ds3(uint8_t);
void readMpu6050(uint8_t);
void readLsm6ds3(uint8_t);
void blinkLed();
void displayImuData();

void setup() {

  Wire.begin();
  Wire.setClock(400000);
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);  
  
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
  
}

// this loop gives a system bandwidth of ~71 Hz (~14ms per iteration)
void loop()
{

  for (uint8_t imuIndex=0; imuIndex<IMU_COUNT_PER_MODEL; ++imuIndex) {
    readMpu6050(imuIndex);
    readLsm6ds3(imuIndex);
  }
  displayImuData();
  blinkLed();
  
}

void initMpu6050(uint8_t imuIndex) {
  Serial.print("Initializing MPU6050 ");
  Serial.println(imuIndex);
  
  mpu6050[imuIndex].initialize();
  mpu6050[imuIndex].setRate(4);     // divisor. Gyroscope sampling rate = 1 kHz (if DLPF is enabled, else 8kHz) / (1 + divisor)
  mpu6050[imuIndex].setDLPFMode(MPU6050_DLPF_BW_42);
  mpu6050[imuIndex].setFullScaleGyroRange(MPU6050_GYRO_FS_1000);
  mpu6050[imuIndex].setFullScaleAccelRange(MPU6050_ACCEL_FS_4);

  mpu6050[imuIndex].setXAccelOffset(mpu6050AccOffsets[imuIndex][0]);
  mpu6050[imuIndex].setYAccelOffset(mpu6050AccOffsets[imuIndex][1]);
  mpu6050[imuIndex].setZAccelOffset(mpu6050AccOffsets[imuIndex][2]);
  mpu6050[imuIndex].setXGyroOffset (mpu6050GyroOffsets[imuIndex][0]);
  mpu6050[imuIndex].setYGyroOffset (mpu6050GyroOffsets[imuIndex][1]);
  mpu6050[imuIndex].setZGyroOffset (mpu6050GyroOffsets[imuIndex][2]);  
  
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

  lsm6ds3[imuIndex].settings.gyroRange = 1000;       // max deg/s. Can be: 125, 245, 500, 1000, 2000
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

void readMpu6050(uint8_t imuIndex) {
  
  uint8_t unifiedImuIndex = imuIndex;
  
  mpu6050[imuIndex]
    .getMotion6(&accelerationX[unifiedImuIndex], 
                &accelerationY[unifiedImuIndex],
                &accelerationZ[unifiedImuIndex], 
                &angularVelocityX[unifiedImuIndex],
                &angularVelocityY[unifiedImuIndex], 
                &angularVelocityZ[unifiedImuIndex]);
                
}

void readLsm6ds3(uint8_t imuIndex) {
  
  uint8_t unifiedImuIndex = imuIndex + IMU_COUNT_PER_MODEL;
  
  accelerationX[unifiedImuIndex] = lsm6ds3[imuIndex].readRawAccelX() - lsm6ds3AccOffsets[imuIndex][0];
  accelerationY[unifiedImuIndex] = lsm6ds3[imuIndex].readRawAccelY() - lsm6ds3AccOffsets[imuIndex][1];
  accelerationZ[unifiedImuIndex] = lsm6ds3[imuIndex].readRawAccelZ() - lsm6ds3AccOffsets[imuIndex][2];
  angularVelocityX[unifiedImuIndex] = lsm6ds3[imuIndex].readRawGyroX() - lsm6ds3GyroOffsets[imuIndex][0];
  angularVelocityY[unifiedImuIndex] = lsm6ds3[imuIndex].readRawGyroY() - lsm6ds3GyroOffsets[imuIndex][1];
  angularVelocityZ[unifiedImuIndex] = lsm6ds3[imuIndex].readRawGyroZ() - lsm6ds3GyroOffsets[imuIndex][2]; 
  
}

void displayImuData() {
  
  Serial.print("a/g");
  for (uint8_t unifiedImuIndex=0; 
       unifiedImuIndex<IMU_MODEL_COUNT * IMU_COUNT_PER_MODEL; 
       ++unifiedImuIndex) {
    Serial.print("\t");
    Serial.print(accelerationX[unifiedImuIndex]); Serial.print("\t");
    Serial.print(accelerationY[unifiedImuIndex]); Serial.print("\t");
    Serial.print(accelerationZ[unifiedImuIndex]); Serial.print("\t");
    Serial.print(angularVelocityX[unifiedImuIndex]); Serial.print("\t");
    Serial.print(angularVelocityY[unifiedImuIndex]); Serial.print("\t");
    Serial.print(angularVelocityZ[unifiedImuIndex]); Serial.print("\t");
  }
  Serial.println();
  
}

void blinkLed() {

  unsigned long currMs = millis();
  if (currMs - prevBlinkMs > LED_TOGGLE_PERIOD) {
    ledBlinkState = !ledBlinkState;
    prevBlinkMs = currMs;
    digitalWrite(LED_PIN, ledBlinkState);
  }
  
}
