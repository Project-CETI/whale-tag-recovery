#include "pico/stdlib.h"
#include <stdio.h>
#include <math.h>
#include <time.h>

// SWARM DEFS [START] ---------------------------------------------------
#define MAX_SWARM_MSG_LEN 500
// SWARM DEFS [END] -----------------------------------------------------

// SWARM VARS [START] ---------------------------------------------------
// swarm-communication processes
char modem_wr_buf[MAX_SWARM_MSG_LEN];
int modem_wr_buf_pos = 0;
char modem_rd_buf[MAX_SWARM_MSG_LEN];
int modem_rd_buf_pos = 0;
int modem_buf_len = 0;
char endNmeaString = '*';
uint8_t nmeaCS;
bool swarmInteractive = false;

// Parse SWARM
bool bootConfirmed = false;
bool initDTAck = false;
bool initPosAck = false;
// GPS data hacks
float latlon[2] = {0,0};
int acs[3] = {0,0,0};
int gpsReadPos = 4;
int gpsCopyPos = 0;
char gpsParseBuf[MAX_SWARM_MSG_LEN];
int gpsInsertPos = 0;
int gpsMult = 1;
bool gpsFloatQ = false;
// SWARM VARS [END] -----------------------------------------------------

// SWARM FUNCTIONS [START] ----------------------------------------------
uint8_t nmeaChecksum(const char *sz, size_t len) {
  size_t i = 0;
  uint8_t cs;
  if (sz[0] == '$')
    i++;
  for (cs = 0; (i < len) && sz [i]; i++)
    cs ^= ((uint8_t) sz[i]);
  return cs;
}

void storeGpsData() {
  while(modem_rd_buf[gpsReadPos] != '*') {
    while (modem_rd_buf[gpsReadPos] != ',' && modem_rd_buf[gpsReadPos] != '*') {
      if (modem_rd_buf[gpsReadPos] == '-') {
        gpsMult = -1;
      }
      else {
        if (modem_rd_buf[gpsReadPos] == '.') {
          gpsFloatQ = true;
        }
        gpsParseBuf[gpsCopyPos] = modem_rd_buf[gpsReadPos];
        gpsCopyPos++;
      }
      gpsReadPos++;
    }
    gpsParseBuf[gpsCopyPos] = '\0';
    if (gpsFloatQ) {
      latlon[gpsInsertPos] = ((float)gpsMult) * atof(gpsParseBuf);
    }
    else {
      acs[gpsInsertPos] = gpsMult * atoi(gpsParseBuf);
    }
    if (modem_rd_buf[gpsReadPos] == '*') {
      gpsReadPos = 4;
      gpsCopyPos = 0;
      gpsInsertPos = 0;
      gpsMult = 1;
      break;
    }
    gpsReadPos++;
    gpsCopyPos = 0;
    (gpsFloatQ && gpsInsertPos==1) ? gpsInsertPos-- : gpsInsertPos++;
    gpsFloatQ = false;
    gpsMult = 1;
  }
}

int parseModemOutput() {
  if (modem_rd_buf[2] == 'N' && modem_rd_buf[4] != 'E' && modem_rd_buf[4] != 'O') {
    storeGpsData();
  }
  else if (modem_rd_buf[1] == 'T' && modem_rd_buf[4] == 'S') {
    txSwarm();
  }
  return 0;
}

void readFromModem() {
  if (Serial1.available()) {
    modem_rd_buf[modem_rd_buf_pos] = Serial1.read();
    
    if (modem_rd_buf[modem_rd_buf_pos] != '\n') {
      modem_rd_buf_pos++;
    }
    else {
      modem_rd_buf[modem_rd_buf_pos] = '\0';
      modem_buf_len = modem_rd_buf_pos;
      modem_rd_buf_pos = 0;
      if (swarmInteractive) Serial.println(modem_rd_buf);
      parseModemOutput();
    }
  }
}

int checkSwarmBooted() {
  if (modem_buf_len != 21) {
    return 0; // Can't be BOOT,RUNNING message
  }
  if (modem_rd_buf[6] != 'B') {
    return 0; // Not BOOT sequence
  }
  if (modem_rd_buf[modem_buf_len-1] != 'a') {
    return 0; // Not the right checksum
  }
  bootConfirmed = true;
  return 0;
}

int checkInitAck() {
  if (modem_rd_buf[16] == '6') {
    initDTAck = true;
    return 0;
  }
  else if (modem_rd_buf[16] == 'e') {
    initPosAck = true;
  }
  return 0;
}

void swarmBootSequence(bool waitForAcks) {
  while(!bootConfirmed) {
    readFromModem();
    checkSwarmBooted();
  }
  while((!initDTAck || !initPosAck) && waitForAcks) {
    serialCopyToModem();
    readFromModem();
    checkInitAck();
  }
}

void serialCopyToModem() {
  if (swarmInteractive) {
    if (Serial.available() ) {
      modem_wr_buf[modem_wr_buf_pos] = Serial.read();
      
      if (modem_wr_buf[modem_wr_buf_pos] != '\n') {
        modem_wr_buf_pos++;
      }
      else {
        modem_wr_buf[modem_wr_buf_pos] = '\0';
        modem_wr_buf_pos = 0;
        if (modem_wr_buf[0] == '$') {
          writeToModem(modem_wr_buf);
        }
      }
    }
  }
}

void writeToModem(char *sz) {
  nmeaCS = nmeaChecksum(sz, strlen(sz));
  if (swarmInteractive) Serial.printf("[SENT] %s%c%02X\n",sz,endNmeaString,nmeaCS);
  Serial1.printf("%s%c%02X\n",sz,endNmeaString,nmeaCS);
}

void getRxTestOutput(bool onQ) {
  onQ ? writeToModem("$RT 1") : writeToModem("$RT 0");
}

void getQCount() {
  writeToModem("$MT C=U");
}

void swarmInit(bool swarmDebug) {
  getRxTestOutput(false);
  getQCount();
  if (swarmDebug) {
    writeToModem("$TD \"Test 1\"");
    writeToModem("$TD \"Test 2\"");
    writeToModem("$TD \"Test 3\"");
  }
  writeToModem("$GN 5");
}
// SWARM FUNCTIONS [END] ------------------------------------------------
