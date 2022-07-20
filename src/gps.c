#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gps.h"

// SWARM DEFS [START] ---------------------------------------------------
#define MAX_GPS_MSG_LEN 500
// SWARM DEFS [END] -----------------------------------------------------

// SWARM VARS [START] ---------------------------------------------------
// swarm-communication processes
uart_inst_t *uart;
char gps_rd_buf[MAX_GPS_MSG_LEN];
int gps_rd_buf_pos = 0;
int gps_buf_len = 0;
char endNmeaString = '*';
uint8_t nmeaCS;
bool sInteractive = false;

// GPS data hacks
char gnrmc[5]="$GNRMC";
float latlonBuf[2] = {0,0};
uint16_t acsBuf[3] = {0,0,0};
int gpsReadPos = 4;
int gpsCopyPos = 0;
char gpsParseBuf[MAX_GPS_MSG_LEN];
int gpsInsertPos = 0;
int gpsMult = 1;
bool gpsFloatQ = false;
char lastGpsBuffer[MAX_GPS_MSG_LEN] = "$GN 42.3648,-71.1247,0,360,0*38";
int lastGpsBufSize = 31;
// DT data hacks
char lastDtBuffer[MAX_GPS_MSG_LEN] = "$DT 20190408195123,V*41";
int lastDtBufSize = 23;

// SWARM VARS [END] -----------------------------------------------------

// SWARM FUNCTIONS [START] ----------------------------------------------
// RX functions from SWARM

int parseGpsOutput(void) {
  if (strcmp(gps_rd_buf, gnrmc
  if (modem_rd_buf[2] == 'N' && modem_rd_buf[4] != 'E' && modem_rd_buf[4] != 'O') {
    storeGpsData();
  }
  else if (modem_rd_buf[1] == 'D' && modem_rd_buf[modem_buf_len - 4] == 'V') {
    storeDTData();
  }
  else if (modem_rd_buf[1] == 'M' && modem_rd_buf[2] == 'T') {
    storeQCount();
  }
  else if (modem_rd_buf[1] == 'T' && modem_rd_buf[4] == 'S') {
    txSwarm(false);
  }
  return 0;
}

void readFromGps(void) {
  char inChar;
  if (uart_is_readable(uart)) {
    inChar = uart_getc(uart);
    if (inChar == '$') {
      int i = 0;
      gps_rd_buf[i++] = inChar;
      while (inChar != '\r') {
        inChar = uart_getc(uart);
	gps_rd_buf[i++] = inChar;
      }
      gps_rd_buf[i-1] = '\0';
      gps_buf_len = i-1;
      parseGpsOutput();
      printf("%s\n",gps_rd_buf);
    }
  }
}

void echoGpsOutput(void) {
  for (int i = 0; i < gps_buf_len; i++) {
    if (gps_rd_buf[i]=='\r') puts_raw("\n[NEW]");
    if (gps_rd_buf[i]=='\n') printf("[%d]",i);
    putchar_raw(gps_rd_buf[i]);
  }
}

// Unsolicited response callbacks
void storeGpsData(void) {
  memcpy(lastGpsBuffer, gps_rd_buf, gps_buf_len);
  lastGpsBufSize = gps_buf_len;
  while(gps_rd_buf[gpsReadPos] != '*') {
    while (gps_rd_buf[gpsReadPos] != ',' && gps_rd_buf[gpsReadPos] != '*') {
      if (gps_rd_buf[gpsReadPos] == '-') {
        gpsMult = -1;
      }
      else {
        if (gps_rd_buf[gpsReadPos] == '.') {
          gpsFloatQ = true;
        }
        gpsParseBuf[gpsCopyPos] = gps_rd_buf[gpsReadPos];
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
    if (gps_rd_buf[gpsReadPos] == '*') {
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

void storeDTData(void) {
  memcpy(lastDtBuffer, gps_rd_buf, gps_buf_len);
  lastDtBufSize = gps_buf_len;
}

// Init SWARM functions
void gpsInit(int txPin, int rxPin, int baudrate, uart_inst_t *uartNum, bool interact) {
  // store vars
  uart = uartNum;
  sInteractive = interact;
  // init uart
  uart_init(uart, baudrate);
  gpio_set_function(txPin, GPIO_FUNC_UART);
  gpio_set_function(rxPin, GPIO_FUNC_UART);
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
