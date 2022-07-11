#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pico/time.h"
#include "swarm.h"

// SWARM DEFS [START] ---------------------------------------------------
#define MAX_SWARM_MSG_LEN 500
// SWARM DEFS [END] -----------------------------------------------------

// SWARM VARS [START] ---------------------------------------------------
// swarm-communication processes
uart_inst_t *uart;
bool sRunning = false;
char modem_wr_buf[MAX_SWARM_MSG_LEN];
int modem_wr_buf_pos = 0;
char modem_rd_buf[MAX_SWARM_MSG_LEN];
int modem_rd_buf_pos = 0;
int modem_buf_len = 0;
char endNmeaString = '*';
uint8_t nmeaCS;
char swarmTxBuffer[MAX_SWARM_MSG_LEN];
bool sInteractive = false;

// Parse SWARM
bool bootConfirmed = false;
bool initDTAck = false;
bool initPosAck = false;
uint32_t maxWaitForSwarmBoot = 30000;
uint32_t maxWaitForDTPAcks = 60000;
// GPS data hacks
float latlonBuf[2] = {0,0};
uint16_t acsBuf[3] = {0,0,0};
int gpsReadPos = 4;
int gpsCopyPos = 0;
char gpsParseBuf[MAX_SWARM_MSG_LEN];
int gpsInsertPos = 0;
int gpsMult = 1;
bool gpsFloatQ = false;
char lastGpsBuffer[MAX_SWARM_MSG_LEN] = "$GN 42.3648,-71.1247,0,360,0*38";
int lastGpsBufSize = 31;
// DT data hacks
uint8_t datetimeBuf[6] = {70,1,1};
uint8_t dtReadPos = 6;
uint8_t dtLength = 2;
char dtParseBuf[MAX_SWARM_MSG_LEN];
char lastDtBuffer[MAX_SWARM_MSG_LEN] = "$DT 20190408195123,V*41";
int lastDtBufSize = 23;
struct tm dt;

// SWARM VARS [END] -----------------------------------------------------

// SWARM FUNCTIONS [START] ----------------------------------------------
// RX functions from SWARM
int parseModemOutput(void) {
  if (modem_rd_buf[2] == 'N' && modem_rd_buf[4] != 'E' && modem_rd_buf[4] != 'O') {
    storeGpsData();
  }
  else if (modem_rd_buf[1] == 'D' && modem_rd_buf[modem_buf_len - 4] == 'V') {
    storeDTData();
  }
  else if (modem_rd_buf[1] == 'T' && modem_rd_buf[4] == 'S') {
    txSwarm();
  }
  return 0;
}

void readFromModem(void) {
  if(uart_is_readable(uart) && sRunning) {
    modem_rd_buf[modem_rd_buf_pos] = uart_getc(uart);
    
    if (modem_rd_buf[modem_rd_buf_pos] != '\n') {
      modem_rd_buf_pos++;
    }
    else {
      modem_rd_buf[modem_rd_buf_pos] = '\0';
      modem_buf_len = modem_rd_buf_pos;
      modem_rd_buf_pos = 0;
      if (sInteractive) {
	printf("%s\n",modem_rd_buf);
      }
      parseModemOutput();
    }
  }
}

// TX functions from SWARM
uint8_t nmeaChecksum(const char *sz, size_t len) {
  size_t i = 0;
  uint8_t cs;
  if (sz[0] == '$')
    i++;
  for (cs = 0; (i < len) && sz [i]; i++)
    cs ^= ((uint8_t) sz[i]);
  return cs;
}

void writeToModem(char *sz) {
  nmeaCS = nmeaChecksum(sz, strlen(sz));
  if (sInteractive) printf("[SENT] %s%c%02X\n",sz,endNmeaString,nmeaCS);
  sprintf(swarmTxBuffer, "%s%c%02X\n\0",sz,endNmeaString,nmeaCS);
  if (!uart_is_writable(uart)) uart_tx_wait_blocking(uart);
  uart_puts(uart, swarmTxBuffer);
}

/*****DON'T USE THIS!!!**************************/
void serialCopyToModem(void) {
  if (sInteractive) {
    char inChar = getchar_timeout_us(10);
    inChar = PICO_ERROR_TIMEOUT;
    if (inChar != PICO_ERROR_TIMEOUT) {
      modem_wr_buf[modem_wr_buf_pos] = inChar;
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

// Unsolicited response callbacks
void storeGpsData(void) {
  memcpy(lastGpsBuffer, modem_rd_buf, modem_buf_len);
  lastGpsBufSize = modem_buf_len;
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
      latlonBuf[gpsInsertPos] = ((float)gpsMult) * atof(gpsParseBuf);
    }
    else {
      acsBuf[gpsInsertPos] = gpsMult * atoi(gpsParseBuf);
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
    initDTAck = true;
    bootConfirmed = true;
  }
}

void storeDTData(void) {
  memcpy(lastDtBuffer, modem_rd_buf, modem_buf_len);
  lastDtBufSize = modem_buf_len;
  int i, j;
  for (j = 0; j < 6; j++) {
    for (i = 0; i < dtLength; i++)
      dtParseBuf[i] = modem_rd_buf[dtReadPos + i];
    dtParseBuf[i] = '\0';
    datetimeBuf[j] = (uint8_t) atoi(dtParseBuf);
    dtReadPos += dtLength;
  }
  dtReadPos = 6;
  dt.tm_year = 2000 + datetimeBuf[0];
  dt.tm_mon = datetimeBuf[1] - 1;
  dt.tm_mday = datetimeBuf[2];
  dt.tm_hour = datetimeBuf[3];
  dt.tm_min = datetimeBuf[4];
  dt.tm_sec = datetimeBuf[5];
}

void txSwarm(void) {
  if (initDTAck) {
    writeToModem("$TD \"Adding Q...\"");
  }
}

// Init SWARM functions
void swarmInit(int txPin, int rxPin, int baudrate, uart_inst_t *uartNum, bool running, bool wAcks, bool interact, bool debug) {
  // store vars
  uart = uartNum;
  sRunning = running;
  swarmSetInteractive(interact);
  // init uart
  uart_init(uart, baudrate);
  gpio_set_function(txPin, GPIO_FUNC_UART);
  gpio_set_function(rxPin, GPIO_FUNC_UART);
  // boot sequence with SWARM
  swarmBootSequence(running, wAcks);
  swarmResponseInit(debug);
}

int checkSwarmBooted(void) {
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

int checkInitAck(void) {
  if (modem_rd_buf[16] == '6') {
    initDTAck = true;
    return 0;
  }
  else if (modem_rd_buf[16] == 'e') {
    initPosAck = true;
  }
  return 0;
}

void swarmBootSequence(bool running, bool wAcks) {
  uint32_t waitStart = to_ms_since_boot(get_absolute_time());
  uint32_t waitTime = to_ms_since_boot(get_absolute_time());
  while(!bootConfirmed && running && (waitTime <= maxWaitForSwarmBoot)) {
    readFromModem();
    checkSwarmBooted();
    waitTime = to_ms_since_boot(get_absolute_time()) - waitStart;
  }
  waitStart = to_ms_since_boot(get_absolute_time());
  waitTime = waitStart;
  while((!initDTAck || !initPosAck) && wAcks && (waitTime <= maxWaitForDTPAcks)) {
    // serialCopyToModem();
    readFromModem();
    checkInitAck();
    waitTime = to_ms_since_boot(get_absolute_time()) - waitStart;
  }
}

void swarmResponseInit(bool debug) {
  getRxTestOutput(true);
  getQCount();
  if (debug) {
    writeToModem("$TD \"Test 1\"");
    writeToModem("$TD \"Test 2\"");
    writeToModem("$TD \"Test 3\"");
  }
  writeToModem("$GN 60");
  writeToModem("$DT 61");
}

// Queue management functions
void getQCount(void) {
  writeToModem("$MT C=U");
}

// Modem management functions
void getRxTestOutput(bool onQ) {
  onQ ? writeToModem("$RT 10") : writeToModem("$RT 0");
}

// Parameter setting functions
void swarmSetInteractive(bool interact) {
  sInteractive = interact;
}

// Data retrieval functions

void getPos(float *dstPosBuf) {
  memcpy(dstPosBuf, latlonBuf, sizeof(latlonBuf));
}

void getACS(uint16_t *dstACSBuf) {
  memcpy(dstACSBuf, acsBuf, sizeof(acsBuf));
}

void getLastPDtBufs(char *dstGpsBuf, char *dstDtBuf) {
  memcpy(dstGpsBuf, lastGpsBuffer, lastGpsBufSize);
  memcpy(dstDtBuf, lastDtBuffer, lastDtBufSize);
}

// SWARM FUNCTIONS [END] ------------------------------------------------
