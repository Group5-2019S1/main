#include <Arduino_FreeRTOS.h>
#include <task.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include "SparkFunLSM6DS3.h"

#define LED_PIN 13
#define SENSOR_PIN 4  // finding current
#define VOLTAGE_PIN 5 // finding voltage
#define LED_TOGGLE_PERIOD 250 // ms 
#define IMU_COUNT_PER_MODEL 2
#define IMU_MODEL_COUNT 2
#define AXIS_COUNT 3
#define IMU_SENSOR_TYPE_COUNT 2
#define RS 0.1
#define VOLTAGE_REF 5

#define STACK_SIZE 3000

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
bool lsm6ds3ConnState[IMU_COUNT_PER_MODEL] = { false, false };
bool mpu6050ConnState[IMU_COUNT_PER_MODEL] = { false, false };
bool ledBlinkState = false;
unsigned long prevBlinkMs = 0;

// Variables for power
float sensorValue;   // Variable to store value from analog read
float voltageValue; // for voltage
float current;       // Calculated current value
float power;         // Calculated power value
unsigned long energyVal;
unsigned long timeVal;
unsigned long prevTime = 0;
unsigned long currTime = 0;

// Variables for handshake
int handshake = 0;
int val = 0;
int ack = 0;

// Message buffer
uint8_t msgBuffer[3000];

typedef union {
   int16_t i;
   uint8_t str[2];
} Data_int;

typedef union {
   float f;
   uint8_t str[4];
} Data_float;

void initMpu6050(uint8_t);
void initLsm6ds3(uint8_t);
void readMpu6050(uint8_t, Data_int[]);
void readLsm6ds3(uint8_t, Data_int[]);
void blinkLed();
void displayImuData(Data_int[]);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
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
  initializingHandshake();
  xTaskCreate(mainTask, "mainTask", STACK_SIZE, NULL, 1, NULL);
  vTaskStartScheduler();
}
void initializingHandshake(){
  
  Serial.println("Initializing handshake");

  while(handshake < 2){
    while(handshake == 0){
      if(Serial2.available()){
        val = Serial2.read();
        //Serial.println(val);
        if(val == 49){
          Serial.println("Request received. Sending ACK to RPi");
          Serial2.write("1");
          handshake = 1;
          val = 0;
        }
      }
    }
    while(handshake == 1){
      if(Serial2.available()){
        val = Serial2.read();
        if(val == 50){
          Serial.println("Received ACK from RPi");
          handshake = 2;
        }
      }
      else{
        Serial.println("Yet to receive ACK from RPi");
      }
    }
  }

  if(handshake == 2){
    Serial.println("Handshake has been established.");
  }

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

void readMpu6050(uint8_t imuIndex, Data_int dataBuffer[]) {
  
  uint16_t unifiedImuIndex = imuIndex;
  
  mpu6050[imuIndex]
    .getMotion6(&dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 3].i,  // accX
                &dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 4].i,  // accY
                &dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 5].i,  // accZ 
                &dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 0].i,  // gyroX
                &dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 1].i,  // gyroY 
                &dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 2].i); // gyroZ
                
}

void readLsm6ds3(uint8_t imuIndex, Data_int dataBuffer[]) {
  
  uint16_t unifiedImuIndex = imuIndex + IMU_COUNT_PER_MODEL;
  
  dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 3].i 
    = lsm6ds3[imuIndex].readRawAccelX() - lsm6ds3AccOffsets[imuIndex][0];
  dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 4].i
    = lsm6ds3[imuIndex].readRawAccelY() - lsm6ds3AccOffsets[imuIndex][1];
  dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 5].i 
    = lsm6ds3[imuIndex].readRawAccelZ() - lsm6ds3AccOffsets[imuIndex][2];
  dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 0].i 
    = lsm6ds3[imuIndex].readRawGyroX() - lsm6ds3GyroOffsets[imuIndex][0];
  dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 1].i 
    = lsm6ds3[imuIndex].readRawGyroY() - lsm6ds3GyroOffsets[imuIndex][1];
  dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 2].i 
    = lsm6ds3[imuIndex].readRawGyroZ() - lsm6ds3GyroOffsets[imuIndex][2]; 
  
}

void displayImuData(Data_int dataBuffer[]) {
  
  Serial.print("a/g");
  for (uint16_t unifiedImuIndex=0; 
       unifiedImuIndex<IMU_MODEL_COUNT * IMU_COUNT_PER_MODEL; 
       ++unifiedImuIndex) {
    Serial.print("\t");
    Serial.print(dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 3].i); Serial.print("\t");
    Serial.print(dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 4].i); Serial.print("\t");
    Serial.print(dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 5].i); Serial.print("\t");
    Serial.print(dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 0].i); Serial.print("\t");
    Serial.print(dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 1].i); Serial.print("\t");
    Serial.print(dataBuffer[unifiedImuIndex * IMU_SENSOR_TYPE_COUNT * AXIS_COUNT + 2].i); Serial.print("\t");
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

void readPowerData(Data_float powerBuffer[]){
  sensorValue = analogRead(SENSOR_PIN);     // set at A4
  voltageValue = analogRead(VOLTAGE_PIN);   // set at A5
  
  // Remap the ADC value into a voltage number (5V reference)
  sensorValue = (sensorValue * VOLTAGE_REF) / 1023.0;
  voltageValue = voltageValue / 1023.0 * 2 * 5;

  // Follow the equation given by the INA169 datasheet to
  // determine the current flowing through RS. Assume RL = 10k
  // Is = (Vout x 1k) / (RS x RL)
  // const float RS = 0.1 Shunt resistor value (in ohms)
  current = sensorValue / (10 * RS);

  // calculating the power
  power = voltageValue * current;

  //calculate energy
  currTime = micros();
  timeVal = currTime - prevTime;
  energyVal = energyVal +  (power * timeVal);
  //Serial.println("Reading PoweR");
  //Serial.println( timeVal > 0.0);
  //Serial.println(energyVal / 3600000000.0);
  
  prevTime = currTime;
  
  // Storing the values into powerBuffer
  powerBuffer[0].f = current;
  powerBuffer[1].f = voltageValue;
  powerBuffer[2].f = power;
  powerBuffer[3].f = energyVal / 3600000000.0;
 
}

void readSensorData(Data_int dataBuffer[]){

  Serial.println("Reading Sensor");
  for (uint8_t imuIndex=0; imuIndex<IMU_COUNT_PER_MODEL; ++imuIndex) {
    readMpu6050(imuIndex, dataBuffer);
    readLsm6ds3(imuIndex, dataBuffer);
  }
  //displayImuData(dataBuffer);
  
}

// Function to concatenate sensor data, power data and checksum into msgbuffer
// Each byte from the union data structures are mapped into msgBuffer
void convertData(Data_int dataBuffer[], Data_float powerBuffer[]){

  uint8_t checksum = 0;
  uint8_t uint8_t_x[7];
  uint8_t checksum_uint8_t[7];  

  // empty msgBuffer
  // memset(msgBuffer, '', 66);

  // An int is 2 bytes in union data structutre
  for(int i=0; i<48; i+=2){
    msgBuffer[i] = dataBuffer[i/2].str[1];
    msgBuffer[i+1] = dataBuffer[i/2].str[0];
  }

  // A float is 4 bytes in union data structure
  int x = 0;
  for(int k=48; k<64; k+=4){
    msgBuffer[k]= powerBuffer[x].str[3];
    msgBuffer[k+1]= powerBuffer[x].str[2];
    msgBuffer[k+2]= powerBuffer[x].str[1];
    msgBuffer[k+3]= powerBuffer[x].str[0];
    x+=1;
  }

  for (int j=0; j < 64; j++) {
    checksum ^= msgBuffer[j];
  }
  msgBuffer[64] = checksum;

}

void mainTask(void *p) {

//  TickType_t xLastWakeTime;
//  xLastWakeTime = xTaskGetTickCount();
  
  Serial.println("Data sending phase initiated");

  // Variables for resending in case of incorrect checksum
  int flag = 0;
  int ACK;
  int count = 3;
  
  while(1){
    // reset flag
    flag = 0;
    
    blinkLed();
    
    /* 
     sensor data -> 4 x ( gyro(x,y,z) + acc(x,y,z) ) == 24
     Each sensor data uses Data_int union structure
    */
    Data_int dataBuffer[24];
    readSensorData(dataBuffer);

    /*
     Power data -> Current, Voltage, Power, Energy (All in float) == 4
     Each power data uses Data_float union structure
    */
    
    Data_float powerBuffer[4];
    readPowerData(powerBuffer);
    
    convertData(dataBuffer, powerBuffer);

    // To notify RPi the start of a message buffer
    Serial2.write("#");

    /*
     msgBuffer = [sensor Data] + [power Data] + checksum
     Each int of Data_int is 2 bytes in string array
     Each float of Data_float is 4 bytes in string array
     sensor Data = 24 * 2 = 48 bytes, power Data = 4 * 4 = 16 bytes, checksum = 1 byte
     Total bytes = 65 bytes (excluding "#" start byte)
    */
    for (int j=0; j < 65; j++) {
      Serial2.write(msgBuffer[j]);
      //Serial.print(msgBuffer[j]);
    }
    
    Serial.println();
    
    while( flag == 0 ){  
      if (Serial2.available()){
        ACK = Serial2.read();
        if( ACK == 78 ){
          Serial.println("NACK received. Resend");
          if(count != 0){
            count--;
            Serial.print("Resending data.");
            Serial.print(count);
            Serial.println(" time(s) left");
            for (int j=0; j < 65; j++) {
              Serial2.write(msgBuffer[j]);
              //Serial.print(msgBuffer[j]);
            }
          } else {
            Serial.println("Data packet dropped. Proceeding to sending next data packet.");
            flag = 1;
          }
        }
        if( ACK == 65 ){
          Serial.println("ACK received!");
          flag = 1;
          count = 5;
        }
        
      }
    }
  }
  
}

void loop() {
  // put your main code here, to run repeatedly:
  //mainTask();
}
