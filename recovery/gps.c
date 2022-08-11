#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "gps.h"
#include "minmea.h"

// GPS FUNCTIONS [START] ----------------------------------------------
// RX functions from NEO-M8N

void parseGpsOutput(char *line, int buf_len, gps_data_s * gps_dat) {
  switch (minmea_sentence_id(line, false)) {
  case MINMEA_SENTENCE_RMC: {
    struct minmea_sentence_rmc frame;
    if (minmea_parse_rmc(&frame, line)) {
      memcpy(gps_dat->lastGpsBuffer, line, buf_len);
      gps_dat->latlon[0] = minmea_tocoord(&frame.latitude);
      gps_dat->latlon[1] = minmea_tocoord(&frame.longitude);
      if (isnan(gps_dat->latlon[0])) gps_dat->latlon[0] = 42.3648;
      if (isnan(gps_dat->latlon[1])) gps_dat->latlon[1] = 0.0;
      gps_dat->acs[1] = minmea_rescale(&frame.course, 3);
      gps_dat->acs[2] = minmea_rescale(&frame.speed, 1);
      // printf("[PARSED]: %f, %f, %d, %d\n", gps_dat->latlon[0], gps_dat->latlon[1], gps_dat->acs[1], gps_dat->acs[2]);
      // printf("[C/S]: %d, %d, %d, %d, %d, %d\n", &frame.course.value, &frame.course.scale, gps_dat->acs[0], &frame.speed.value, &frame.speed.scale, gps_dat->acs[1]);
			gps_dat->posCheck = true;
    }
    break;
  }
  case MINMEA_SENTENCE_ZDA: {
    struct minmea_sentence_zda frame;
    if (minmea_parse_zda(&frame, line))
			memcpy(gps_dat->lastDtBuffer, line, buf_len);
    // printf("[DT]: %s", gps_dat->lastDtBuffer);
    break;
  }
  case MINMEA_INVALID:
    break;
  default:
    break;
  }
}

void readFromGps(const gps_config_s * gps_cfg, gps_data_s * gps_dat) {
  char inChar;
	char gps_rd_buf[MAX_GPS_MSG_LEN];
	int gps_rd_buf_pos = 0;
	int gps_buf_len = 0;
  if (uart_is_readable(gps_cfg->uart)) {
    inChar = uart_getc(gps_cfg->uart);
    if (inChar == '$') {
			gps_dat->datCheck = true;
      int i = 0;
      gps_rd_buf[i++] = inChar;
      while (inChar != '\r') {
        inChar = uart_getc(gps_cfg->uart);
				if (inChar == '$') i = 0;
				gps_rd_buf[i++] = inChar;
      }
      gps_rd_buf[i-1] = '\0';
      gps_buf_len = i-1;
      parseGpsOutput(gps_rd_buf, gps_buf_len, gps_dat);
      // printf("%s\r\n",gps_rd_buf);
    }
  }
}

void drainGpsFifo(const gps_config_s * gps_cfg, gps_data_s * gps_dat) {
	// printf("Starting draining.\n");
	char gps_rd_buf[MAX_GPS_MSG_LEN];
	if (uart_is_readable(gps_cfg->uart)) {
		// printf("How much to read: %d\n", uart_is_readable(gps_cfg->uart));
		uart_read_blocking(gps_cfg->uart,
											 gps_rd_buf,
											 uart_is_readable(gps_cfg->uart));
	}
	// printf("Finished draining.\n");
	gps_dat->datCheck = false;
	gps_dat->posCheck = false;
}

void echoGpsOutput(char *line, int buf_len) {
  for (int i = 0; i < buf_len; i++) {
    if (line[i]=='\r') puts_raw("\n[NEW]");
    if (line[i]=='\n') printf("[%d]",i);
    putchar_raw(line[i]);
  }
}

// Init NEO-M8N functions
void gpsInit(const gps_config_s * gps_cfg) {
	uart_init(gps_cfg->uart, gps_cfg->baudrate);
	gpio_set_function(gps_cfg->txPin, GPIO_FUNC_UART);
	gpio_set_function(gps_cfg->rxPin, GPIO_FUNC_UART);
}

// GPS FUNCTIONS [END] ------------------------------------------------
