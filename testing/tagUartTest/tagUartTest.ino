#include "pico/stdlib.h"
#include <stdio.h>
#include <math.h>
#include <time.h>

union {
  float coord;
  uint8_t coordByte[4];
} lat;

union {
  float coord;
  uint8_t coordByte[4];
} lon;

lat.coord = 42.364887f;
lon.coord = -71.1246948;
uint16_t sog = 13;
uint16_t course = 359;

//uint8_t buddybite[4] = {(int)((buddy >> 24) & 0xff),
//                        (int)((buddy >> 16) & 0xff),
//                        (int)((buddy >> 8) & 0xff),
//                        (int)(buddy & 0xff)};
//uint8_t olpalbite[4] = {(int)((olpal >> 24) & 0xff),
//                        (int)((olpal >> 16) & 0xff),
//                        (int)((olpal >> 8) & 0xff),
//                        (int)(olpal & 0xff)};
uint8_t sogbit = sog & 0xff;
uint8_t coursebite[2] = {(course >> 8), (course & 0xff)};
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
//  Serial.print(sizeof((uint8_t *) &buddy));
}

void loop() {
  // put your main code here, to run repeatedly:
//  Serial.print(sizeof((uint8_t *) &olpal));
//  Serial.write((uint8_t *) &buddy, 4);
//  Serial.write((uint8_t *) &olpal, 4);
//  Serial.write((uint8_t *) &sog, 1);
//  Serial.write((uint8_t *) &course, 2);
  Serial.print(sizeof(lat.coordByte));
  Serial.print(sizeof(lon.coordByte));
  Serial.print(sizeof(sogbit));
  Serial.print(sizeof(coursebite));
  Serial.write('\n');
  delay(100);
  
}
