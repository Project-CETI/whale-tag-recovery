#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "gps.h"
#include "minmea.h"
#include "ublox-config.h"

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
      // printf("0x%x ", inChar);
      while (inChar != '\r') {
        inChar = uart_getc(gps_cfg->uart);
				if (inChar == '$') i = 0;
				gps_rd_buf[i++] = inChar;
        // printf("0x%x ", inChar);
      }
      gps_rd_buf[i-1] = '\0';
      gps_buf_len = i-1;
      parseGpsOutput(gps_rd_buf, gps_buf_len, gps_dat);
<<<<<<< HEAD
      printf("\n%s\r\n",gps_rd_buf);
=======
      printf("%s\r\n",gps_rd_buf);
>>>>>>> sdk
    }

  }
}

void drainGpsFifo(const gps_config_s * gps_cfg, gps_data_s * gps_dat) {
	char gps_rd_buf[MAX_GPS_MSG_LEN];
	if (uart_is_readable(gps_cfg->uart))
		uart_read_blocking(gps_cfg->uart,
											 gps_rd_buf,
											 uart_is_readable(gps_cfg->uart));
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

/**
 * Calculates the checksum for the current byte stream using the 8-bit Fletcher Algorithm.
 * Requires that the given byte stream has the last two cells reserved to be filled by the 
 * calculated checksum. Calculation for the checksum starts at the class field, but does not
 * include the two sync bytes at the start (also known has header bytes). 
 * Byte stream should contain both of these sync bytes, as well as space for the two 
 * checksum bytes. 
 * @param length the length of the byte stream, including the checksum at the end
 * @param byte_stream the array for the byte stream, with the last two cells reserved 
 * to be filled by the checksum
 */
void calculateUBXChecksum(uint8_t length, uint8_t* byte_stream) {
    
    uint8_t CK_A = 0;
    uint8_t CK_B = 0;
    
    for(uint8_t i = 2; i < length - 2; i++) {
        CK_A = CK_A + byte_stream[i];
        CK_B = CK_B + CK_A;
    }

    byte_stream[length] = CK_A;
    byte_stream[length+1] = CK_B;
}

bool writeAllConfigurationsToUblox(uart_inst_t* uart) {
  for (uint8_t i = 0; i < NUM_UBLOX_CONFIGS; i++) {
    // do bit magic for the length
    uint16_t length = (((uint16_t)ubx_configurations[i][5] << 8) | ubx_configurations[i][4]) + 0x8;
    // calculateUBXChecksum(length, ubx_configurations[i]);
    // writeSingleConfiguration(uart, ubx_configurations[i], length);
  }

  
}

void writeSingleConfiguration(uart_inst_t* uart, uint8_t* byte_stream, uint8_t len) {
  // printf("Writing Configuration: %x %x\n", byte_stream[2], byte_stream[3]);
  if (!uart_is_writable(uart)) uart_tx_wait_blocking(uart);
  for (uint8_t i = 0; i < len; i++) {
    uart_putc_raw(uart, byte_stream[i]);
  }
  // uart_puts(uart, (char *)byte_stream);
  sleep_ms(500);
  // TODO: fix this to have check for ACKs
  // char inChar;
	// char ack_rd_buf[MAX_GPS_ACK_LENGTH];
	// uint8_t ack_rd_buf_pos = 0;
	// uint8_t ack_buf_len = 0;
  // bool ack = true;
  // if (uart_is_readable_within_us(uart, 10000)) {
  //   // uart_read_blocking(uart, ack_rd_buf, MAX_GPS_ACK_LENGTH);
  //   inChar = uart_getc(uart);
  //   int i = 0;
  //   ack_rd_buf[i] = inChar;
  //   ack = ack && (inChar == ack_header[i++]);
  //   while (inChar != '\r' && i < 100) {
  //     inChar = uart_getc(uart);
  //     ack_rd_buf[i] = inChar;
  //     ack = ack && (inChar == ack_header[i++]);
  //     // printf("0x%x ", inChar);
  //   }
  //   ack_rd_buf[i-1] = '\0';
  //   ack_buf_len = i-1;
  //   // printf("\nAck: %s: %s\r\n", ack_rd_buf, ack ? "true" : "false");
  // }

}

// GPS FUNCTIONS [END] ------------------------------------------------
