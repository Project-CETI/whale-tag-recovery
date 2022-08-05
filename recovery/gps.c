#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "gps.h"
#include "minmea.h"

// GPS DEFS [START] ---------------------------------------------------
#define MAX_GPS_MSG_LEN 500
// GPS DEFS [END] -----------------------------------------------------

// GPS VARS [START] ---------------------------------------------------
// GPS communication processes
uart_inst_t *uart;
char gps_rd_buf[MAX_GPS_MSG_LEN];
int gps_rd_buf_pos = 0;
int gps_buf_len = 0;
char endNmeaString = '*';
uint8_t nmeaCS;
bool sInteractive = false;
bool datCheck, posiCheck;

// GPS data hacks
float latlonBuf[2] = {42.3648,0};
uint16_t acsBuf[3] = {0,0,0};
char lastGpsBuffer[MAX_GPS_MSG_LEN] = "$GN 42.3648,-71.1247,0,360,0*38";
int lastGpsBufSize = 31;
char lastDtBuffer[MAX_GPS_MSG_LEN] = "$DT 20190408195123,V*41";
int lastDtBufSize = 23;

// GPS VARS [END] -----------------------------------------------------

// GPS FUNCTIONS [START] ----------------------------------------------
// RX functions from NEO-M8N

void parseGpsOutput(char *line) {
  switch (minmea_sentence_id(line, false)) {
  case MINMEA_SENTENCE_RMC: {
    struct minmea_sentence_rmc frame;
    if (minmea_parse_rmc(&frame, line)) {
      memcpy(lastGpsBuffer, gps_rd_buf, gps_buf_len);
      latlonBuf[0] = minmea_tocoord(&frame.latitude);
      latlonBuf[1] = minmea_tocoord(&frame.longitude);
      if (isnan(latlonBuf[0])) latlonBuf[0] = 42.3648;
      if (isnan(latlonBuf[1])) latlonBuf[1] = 0.0;
      acsBuf[1] = minmea_rescale(&frame.course, 3);
      acsBuf[2] = minmea_rescale(&frame.speed, 1);
      lastGpsBufSize = gps_buf_len;
      // printf("[PARSED]: %f, %f, %d, %d\n", latlonBuf[0], latlonBuf[1], acsBuf[1], acsBuf[2]);
      printf("[C/S]: %d, %d, %d, %d, %d, %d\n", &frame.course.value, &frame.course.scale, acsBuf[0], &frame.speed.value, &frame.speed.scale, acsBuf[1]);
			posiCheck = true;
    }
    break;
  }
  case MINMEA_SENTENCE_ZDA: {
    struct minmea_sentence_zda frame;
    if (minmea_parse_zda(&frame, line)) memcpy(lastDtBuffer, gps_rd_buf, gps_buf_len);
    lastDtBufSize = gps_buf_len;
    // printf("[DT]: %s", lastDtBuffer);
    break;
  }
  case MINMEA_INVALID:
    break;
  default:
    break;
  }
}

void readFromGps(void) {
  char inChar;
  if (uart_is_readable(uart)) {
    inChar = uart_getc(uart);
    if (inChar == '$') {
			datCheck = true;
      int i = 0;
      gps_rd_buf[i++] = inChar;
      while (inChar != '\r') {
        inChar = uart_getc(uart);
				gps_rd_buf[i++] = inChar;
      }
      gps_rd_buf[i-1] = '\0';
      gps_buf_len = i-1;
      parseGpsOutput(gps_rd_buf);
      // printf("%s\n",gps_rd_buf);
    }
  }
}

void drainGpsFifo(void) {
	if (uart_is_readable(uart)) uart_read_blocking(uart, gps_rd_buf, uart_is_readable(uart));
	datCheck = false;
	posiCheck = false;
}

void echoGpsOutput(void) {
  for (int i = 0; i < gps_buf_len; i++) {
    if (gps_rd_buf[i]=='\r') puts_raw("\n[NEW]");
    if (gps_rd_buf[i]=='\n') printf("[%d]",i);
    putchar_raw(gps_rd_buf[i]);
  }
}

// Init NEO-M8N functions
void gpsInit(int txPin, int rxPin, int baudrate, uart_inst_t *uartNum, bool *dCheck, bool *pCheck) {
  // store vars
  uart = uartNum;
	datCheck = dCheck;
	posiCheck = pCheck;
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

// GPS FUNCTIONS [END] ------------------------------------------------
