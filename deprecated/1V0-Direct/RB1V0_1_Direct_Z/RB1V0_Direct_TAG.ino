#include "pico/stdlib.h"
#include <stdio.h>
#include <math.h>
#include <time.h>

// TAG DEFS [START] -----------------------------------------------------
#define MAX_TAG_MSG_LEN 20
// TAG DEFS [END] -------------------------------------------------------

// TAG VARS [START] -----------------------------------------------------
bool tagResponseIn = false;
char tag_rd_buf[MAX_TAG_MSG_LEN];
int tag_rd_buf_pos = 0;
int tag_buf_len = 0;
uint32_t tMsgSent = 0;
uint32_t ackWaitT= 2000;
// TAG VARS [END] -------------------------------------------------------

// TAG FUNCTIONS [START] ------------------------------------------------
int resetTagReading() {tag_rd_buf_pos = 0; tagResponseIn = true; return 1;}
void readFromTag() {
  if (Serial2.available()) {
    tag_rd_buf[tag_rd_buf_pos] = Serial2.read();
    (tag_rd_buf[tag_rd_buf_pos] != '\n') ? tag_rd_buf_pos++ : resetTagReading();
  }
}

int parseTagStatus() {
  return 0;
}

void getTagAckGps() {
  // we legitimately could not care about this ack; do nothing
  tagResponseIn = false;
}

bool getDetachAck() {
  if (tag_rd_buf[0] != 'd') return false; return true;
}

void writeGpsToTag(char *sz) {
//  Serial.printf("[TO TAG] %s%c%02X\n",sz,endNmeaString,nmeaCS);
  Serial2.printf("%s\n",sz);
  tagResponseIn = false;
  tMsgSent = millis();
  while (millis() - tMsgSent < ackWaitT) {while(!tagResponseIn && (millis() - tMsgSent < ackWaitT)) {readFromTag();}}
  getTagAckGps();
}

void detachTag() {
  tMsgSent = millis();
  while(millis() - tMsgSent < ackWaitT) {
    Serial2.println('D');
    while (!tagResponseIn && (millis() - tMsgSent < ackWaitT)) {readFromTag();}
    if (getDetachAck()) break;
  }
}

void reqTagState() {
  tMsgSent = millis();
  Serial2.println('T');
  while (millis() - tMsgSent < ackWaitT) {while(!tagResponseIn && (millis() - tMsgSent < ackWaitT)) {readFromTag();}}
  parseTagStatus();
}
// TAG FUNCTIONS [END] --------------------------------------------------
