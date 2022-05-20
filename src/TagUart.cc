//
// Created by Louis Adamian on 5/16/22.
//
#include "Arduino.h"
#include "TagUart.hh"
#include "TagRecoveryBoardv0_9.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/sleep.h"
#include "Swarm.h"

TagUart::TagUart(uint8_t txPin, uint8_t rxPin) {
  gpio_set_function(6, GPIO_FUNC_UART);
  gpio_set_function(7, GPIO_FUNC_UART);
  uart_init(tagUartBus, 115200);
}

void TagUart::receiveInterupt() {
  char* buff;
  uart_read_blocking(tagUartBus, buff, 32)
  TagUart::parseReceive(buff);
}

int8_t TagUart::parseReceive(char* buff){
  if(buff[0] == 'G'){
    // gps

  }
  else if (buff[0] == 'S'){
    // sleep
    uart_puts(tagUartBus, "s\n")
    sleep_goto_dormant_until_edge_high(tagSerialRxPin);
  }
  else {
    return -1;
  }
}

uint8_t TagUart::parseStatus(char* status) {
  uint8_t mode = (uint8_t)status[1];
  uint8_t batteryPercent = (uint8_t)status[2];
  uint16_t batteryTime;
  memcpy(batteryTime, );
  uint8_t diveNum = status[4];
}


uint8_t TagUart::sendDetach() {
  tagSerial.println("D");
  while(tagSerial.available()<2);
  char* buff;
  tagSerial.readBytesUntil('\n', buff, 32);
  if (buff[0] == 'R'){
    return 0;
  }
  return 1;
}

// sends gps lat, lon, course, speed, and time
uint8_t TagUart::sendGpsTime() {
  Swarm_M138_GeospatialData_t* gps = new Swarm_M138_GeospatialData_t;
  Swarm_M138_DateTimeData_t* dateTime = new Swarm_M138_DateTimeData_t;
  char message[19];
  message[0] = 'g';
  message[18] = '\n';
  char lat[4];
  char lon[4];
  char speed = round(gps->speed/1.852);
  char course[2];
  memcpy(lat, &gps->lat, sizeof(float));
  memcpy(lon, &gps->lon, sizeof(float));
  memcpy(course, &gps->course, sizeof(uint16_t));
  messageByte=1;
  for(i =0; i< sizeof(lat); i++){
    message[messageByte] = lat[i]
    messageByte++;
  }
  for(i =0; i< sizeof(lon); i++){
    message[messageByte] = lon[i]
    messageByte++;
  }
  message[messageByte] = speed;
  messageByte++;
  for(i =0; i<sizeof(course); i++){
    message[messageByte] = course[i];
  }


}