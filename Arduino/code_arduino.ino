#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <stdlib.h>
#include <task.h>
#include <Wire.h>
#include <stdio.h>
#include <string.h>

#define STACK_SIZE 200

// Message buffer
char msgbuffer[4000];

// Variables for handshake
int handshake = 0;
int val = 0;
int ack = 0;


//char array of 4 bytes as data from 1 axis of IMU is 4 bytes
typedef union {
   float f;
   char str[4];
} Data;

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);
  Serial3.begin(115200);
  //pinMode(A1, OUTPUT);
  //pinMode(A2, OUTPUT);
  //pinMode(A3, OUTPUT);
  initializingHandshake();
  xTaskCreate(mainTask, "mainTask", STACK_SIZE, NULL, 1, NULL);
  //xTaskCreate(testing, "testing", STACK_SIZE, NULL, 1, NULL);
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
  
  Serial.println("Initializing handshake");

  while(handshake < 2){
    while(handshake == 0){
      if(Serial3.available()){
        val = Serial3.read();
        //Serial.println(val);
        if(val == 49){
          Serial.println("Request received. Sending ACK to RPi");
          Serial3.write("1");
          handshake = 1;
          val = 0;
        }
      }
    }
    while(handshake == 1){
      if(Serial3.available()){
        Serial3.write("1");
        val = Serial3.read();
        //Serial.println(val);
        if(val == 50){
          Serial.println("Received ACK from RPi");
          handshake = 2;
        }
      }
      else{
        Serial3.write("1");
        Serial.println("Yet to receive ACK from RPi");
      }
    }
  }

  if(handshake == 2){
    Serial.println("Handshake has been established.");
  }

}

void mainTask(void *p) {

  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  
  Serial.println("Data sending phase initiated");
  
  int flag = 0;
  int ACK;
  int count = 5;


/*   Data data;

  data.f = 1.23;
  Serial3.write("#");
  Serial3.write(data.str[3]);
  Serial3.write(data.str[2]);
  Serial3.write(data.str[1]);
  Serial3.write(data.str[0]);
  Serial3.write("\n");
 */
 
  while(1){
    if(flag==1);
  }
  
  while(1){
    flag = 0;
    // An int is 2 bytes
    // 4 * ( gyro(x,y,z) + acc(x,y,z) ) == 48 bytes
    // Therefore need array size of 24 for 1 full array of sensor reading
    // This is without power readings
    int sensorReadings [24];
    //Empty msg buffer
    strcpy(msgbuffer, "");
    strcat(msgbuffer, "#");
	
	//Testing sending of 109 bytes to simulate actual data packet
    strcat(msgbuffer, "1234567890");
    strcat(msgbuffer, "1234567890");
    strcat(msgbuffer, "1234567890");
    strcat(msgbuffer, "1234567890");
    strcat(msgbuffer, "1234567890");
    strcat(msgbuffer, "1234567890");
    strcat(msgbuffer, "1234567890");
    strcat(msgbuffer, "1234567890");
    strcat(msgbuffer, "1234567890");
    strcat(msgbuffer, "1234567890");
    strcat(msgbuffer, "123456789\n");
    Serial.println(msgbuffer);
    int ret = Serial3.write(msgbuffer);
    Serial.println(ret);


	//Error correction
    while( flag == 0 ){  
      if (Serial3.available()){
        ACK = Serial3.read();
        if( ACK == 78 ){
          Serial.println("NACK received. Resend");
          if(count != 0){
            count--;
            Serial.print("Resending data.");
            Serial.print(count);
            Serial.println(" time(s) left");
            Serial3.write(msgbuffer);
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
    //calculate checksum and append in msgbuffer
  }
  
}

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println("HELLOWORLD");
//  int incomingByte;
//  if(Serial3.available() > 0) {
//    incomingByte = Serial3.read();
//    Serial.println(incomingByte);
//  }
}
