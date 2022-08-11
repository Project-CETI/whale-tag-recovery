#ifndef SWARM_H
#define SWARM_H
#include "stdint.h"
#include "stdbool.h"
#include "pico/stdlib.h"

// GPS DEFS [START] ---------------------------------------------------
#define MAX_GPS_MSG_LEN 500
#define MAX_GPS_ACK_LENGTH 11
// GPS DEFS [END] -----------------------------------------------------

// Critical typedefs
typedef struct gps_config_t {
	int txPin;
	int rxPin;
	int baudrate;
	uart_inst_t *uart;
	bool gpsInteractive;
} gps_config_s;
typedef struct gps_data_t {
	float latlon[2];
	uint16_t acs[3];
	char lastGpsBuffer[MAX_GPS_MSG_LEN];
	char lastDtBuffer[MAX_GPS_MSG_LEN];
	bool datCheck;
	bool posCheck;
} gps_data_s;

// RX functions from MAX-M8
void parseGpsOutput(char *line, int buf_len, gps_data_s * gps_dat);
void readFromGps(const gps_config_s * gps_cfg, gps_data_s * gps_dat);
void drainGpsFifo(const gps_config_s * gps_cfg, gps_data_s * gps_dat);
void echoGpsOutput(char *line, int buf_len);

// Init MAX-M8 functions
void gpsInit(const gps_config_s *);

// Ublox configuration functions
void calculateUBXChecksum(uint8_t length, uint8_t* byte_stream);
bool writeAllConfigurationsToUblox(uart_inst_t* uart);
void writeSingleConfiguration(uart_inst_t* uart, uint8_t* byte_stream);


#endif
