#ifndef SWARM_H
#define SWARM_H
#include "pico/stdlib.h"
#include "stdint.h"

// GPS DEFS [START] ---------------------------------------------------
#define MAX_GPS_MSG_LEN 255
#define MAX_GPS_DATA_BUFFER 4
#define MAX_GPS_ACK_LENGTH 11
// GPS DEFS [END] -----------------------------------------------------

// Critical typedefs
// the GPS configuration for UBLOX
typedef struct gps_config_t {
    int txPin;
    int rxPin;
    int baudrate;
    uart_inst_t *uart;
    bool gpsInteractive;
} gps_config_s;

typedef struct gps_data_t {
    float latlon[MAX_GPS_DATA_BUFFER][2];
    uint16_t acs[3];
    char lastGpsBuffer[MAX_GPS_DATA_BUFFER][MAX_GPS_MSG_LEN];
    char lastDtBuffer[MAX_GPS_MSG_LEN];
    uint8_t gpsReadFlags[MAX_GPS_DATA_BUFFER];

    uint32_t quality;
    bool daytime;
    bool outsideDominica;
    bool posCheck;
} gps_data_s;

typedef struct gps_lat_lon_t {
    float lat;
    float lon;
    char type[5];
    char msg[MAX_GPS_MSG_LEN];
    bool notAvail;
} gps_lat_lon_s;

enum GPSDataBufferOrder{
    GPS_GGA = 0,
    GPS_GLL = 1,
    GPS_RMC = 2
};

// RX functions from NEO-M8N
void parseGpsOutput(char *line, int buf_len, gps_data_s *gps_dat);
bool readFromGps(const gps_config_s *gps_cfg, gps_data_s *gps_dat);
void gps_get_lock(const gps_config_s *gps_cfg, gps_data_s *gps_dat);
void echoGpsOutput(char *line, int buf_len);
void getBestLatLon(gps_data_s *gps_dat, gps_lat_lon_s *latlon);

// Init NEO-M8N functions
uint32_t gpsInit(const gps_config_s *);

// Ublox configuration functions
void calculateUBXChecksum(uint8_t length, uint8_t* byte_stream);
bool writeAllConfigurationsToUblox(uart_inst_t* uart);
void writeSingleConfiguration(uart_inst_t* uart, uint8_t* byte_stream, uint8_t len);


#endif
