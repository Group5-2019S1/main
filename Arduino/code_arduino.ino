#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <stdlib.h>
#include <task.h>
#include <Wire.h>
#include <stdio.h>
#include <string.h>

#define STACK_SIZE 1000

// Message buffer
char msgbuffer[4000];

// Variables for handshake
int handshake = 0;
int val = 0;
int ack = 0;

typedef union {
   float f;
   char str[4];
} Data;

void setup() {
  // put your setup code here, to run once:
  //Wire.begin();
  Serial.begin(115200);
  Serial2.begin(115200);
  //pinMode(A1, OUTPUT);
  //pinMode(A2, OUTPUT);
  //pinMode(A3, OUTPUT);
  initializingHandshake();
  //xTaskCreate(testing, "handshake", STACK_SIZE, NULL, 2, NULL);
  xTaskCreate(mainTask, "mainTask", STACK_SIZE, NULL, 1, NULL);
  //xTaskCreate(testing, "sendDate", STACK_SIZE, NULL, 2, NULL);
  vTaskStartScheduler();
}

//void testing() {
//
//  TickType_t xLastWakeTime;
//  xLastWakeTime = xTaskGetTickCount();
//  
//  Serial.println("checking");
//  //strcpy(data.str, "1111" );
//  //Serial.println(data.str);
//  
//}

void initializingHandshake(){
  
  //Serial.println("Initializing handshake");

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

void readData(Data dataBuffer[]){

  Data data;

  // Simulate data reading from IMU sensors
  for(int i = 0; i < 27 ; i++){
    data.f = 1.23;
    dataBuffer[i] = data;
  }
  Serial.print("readDate mode");
  // '#' represents start of data packet
  Serial2.write("#");
  
  for(int j = 0; j < 27; j++){
    Serial2.write(dataBuffer[j].str[3]);
    Serial2.write(dataBuffer[j].str[2]);
    Serial2.write(dataBuffer[j].str[1]);
    Serial2.write(dataBuffer[j].str[0]);

  }
  
  Serial2.write("0");  // Checksum
  Serial2.write("\n");  // End of line
}

void mainTask(void *p) {
//
//  TickType_t xLastWakeTime;
//  xLastWakeTime = xTaskGetTickCount();
  
  Serial.println("Data sending phase initiated");
  
  int flag = 0;
  int ACK;
  int count = 5;

  /* 
   A float is 4 bytes in union data structutre
   4 ( gyro(x,y,z) + acc(x,y,z) ) == 96 bytes
   '#'(start) + Current(4B) + Voltage(4B) + Power(4B) + Checksum = 14 bytes 
   Therefore need array size of 110 bytes for 1 packet of sensor reading
  */
  Data dataBuffer[110];

//  while(1){
//    if(flag==1);
//  }
  
  while(1){
    // Flag to check if NACK received
    flag = 0;
    
    readData(dataBuffer);
    
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
  //Serial.println("HELLOWORLD");
//  int incomingByte;
//  if(Serial2.available() > 0) {
//    incomingByte = Serial2.read();
//    Serial.println(incomingByte);
//  }
}