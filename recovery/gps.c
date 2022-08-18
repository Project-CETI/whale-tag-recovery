#include "gps.h"
#include "constants.h"
#include "minmea.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


// GPS FUNCTIONS [START] ----------------------------------------------
// RX functions from NEO-M8N
void parseGpsOutput(char *line, int buf_len, gps_data_s *gps_dat) {
    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, line)) {
                memcpy(gps_dat->lastGpsBuffer[GPS_RMC], line,
                       buf_len);
                float latitude = minmea_tocoord(&frame.latitude);
                float longitude = minmea_tocoord(&frame.longitude);
                if (isnan(latitude) || isnan(longitude)) {
                    gps_dat->latlon[GPS_RMC][0] =
                        DEFAULT_LAT;
                    gps_dat->latlon[GPS_RMC][1] =
                        DEFAULT_LON;
                } else {
                    gps_dat->latlon[GPS_RMC][0] = latitude;
                    gps_dat->latlon[GPS_RMC][1] = longitude;
                    gps_dat->gpsReadFlags[GPS_RMC] = 1;
                    gps_dat->posCheck = true;
                }
                gps_dat->acs[1] = minmea_rescale(&frame.course, 3);
                gps_dat->acs[2] = minmea_rescale(&frame.speed, 1);
                /* printf("[PARSED]: %d | %f, %f, %d, %d\n", gps_dat->posCheck,
                 */
                /* 			 minmea_tocoord(&frame.latitude), */
                /* 			 minmea_tocoord(&frame.longitude), */
                /* 			 minmea_rescale(&frame.course, 3), */
                /* 			 minmea_rescale(&frame.speed, 1)); */
                // printf("[C/S]: %d, %d, %d, %d, %d, %d\n",
                // &frame.course.value, &frame.course.scale, gps_dat->acs[0],
                // &frame.speed.value, &frame.speed.scale, gps_dat->acs[1]);
            }
            break;
        }
        case MINMEA_SENTENCE_ZDA: {
            struct minmea_sentence_zda frame;
            if (minmea_parse_zda(&frame, line)) {
                memcpy(gps_dat->lastDtBuffer, line, buf_len);
                // todo add a better datatime check to make sure if valid
                gps_dat->datCheck = true;
            }
            // printf("[DT] %s\n", gps_dat->lastDtBuffer);
            break;
        }
        case MINMEA_SENTENCE_GLL: {
            struct minmea_sentence_gll frame;
            if (minmea_parse_gll(&frame, line)) {
                memcpy(gps_dat->lastGpsBuffer[GPS_GLL], line,
                       buf_len);
                float latitude = minmea_tocoord(&frame.latitude);
                float longitude = minmea_tocoord(&frame.longitude);
                if (isnan(latitude) || isnan(longitude)) {
                    gps_dat->latlon[GPS_GLL][0] =
                        DEFAULT_LAT;
                    gps_dat->latlon[GPS_GLL][1] =
                        DEFAULT_LON;
                } else {
                    gps_dat->latlon[GPS_GLL][0] = latitude;
                    gps_dat->latlon[GPS_GLL][1] = longitude;
                    gps_dat->gpsReadFlags[GPS_GLL] = 1;
                    gps_dat->posCheck = true;
                }
            }
            break;
        }
        case MINMEA_SENTENCE_GGA: {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, line)) {
                memcpy(gps_dat->lastGpsBuffer[GPS_GGA], line,
                       buf_len);
                float latitude = minmea_tocoord(&frame.latitude);
                float longitude = minmea_tocoord(&frame.longitude);
                if (isnan(latitude) || isnan(longitude)) {
                    gps_dat->latlon[GPS_GGA][0] =
                        DEFAULT_LAT;
                    gps_dat->latlon[GPS_GGA][1] =
                        DEFAULT_LON;
                } else {
                    gps_dat->latlon[GPS_GGA][0] = latitude;
                    gps_dat->latlon[GPS_GGA][1] = longitude;
                    gps_dat->gpsReadFlags[GPS_GGA] = 1;
                    gps_dat->posCheck = true;
                }
            }
            break;
        }
        case MINMEA_INVALID:
            break;
        default:
            break;
    }
}

bool readFromGps(const gps_config_s *gps_cfg, gps_data_s *gps_dat) {
    char inChar;
    char gps_rd_buf[MAX_GPS_MSG_LEN];
    int gps_rd_buf_pos = 0;
    int gps_buf_len = 0;
    gps_dat->posCheck = false;
    gps_dat->datCheck = false;
    while (uart_is_readable(gps_cfg->uart)) {
        inChar = uart_getc(gps_cfg->uart);
        if (inChar == '$') {
            gps_dat->datCheck = true;
            gps_rd_buf[gps_rd_buf_pos++] = inChar;
            while (inChar != '\r') {
                inChar = uart_getc(gps_cfg->uart);
                if (inChar == '$') {
                    gps_rd_buf_pos = 0;
                }
                gps_rd_buf[gps_rd_buf_pos++] = inChar;
            }
            gps_rd_buf[gps_rd_buf_pos - 1] = '\0';
            gps_buf_len = gps_rd_buf_pos - 1;
            parseGpsOutput(gps_rd_buf, gps_buf_len, gps_dat);
        }
    }
    return gps_dat->posCheck;
}

void gps_get_lock(const gps_config_s *gps_cfg, gps_data_s *gps_dat) {
    gps_dat->posCheck = false;
    uint32_t startTime = to_ms_since_boot(get_absolute_time());
    while (gps_dat->posCheck != true) {
        if (readFromGps(gps_cfg, gps_dat) ||
            to_ms_since_boot(get_absolute_time()) - startTime > 1000)
            break;
    }
    printf("[GPS LOCK] %s  GGA %.2f, %.2f  GLL %.2f, %.2f RMC %.2f, %.2f\n",
           gps_dat->posCheck ? "true" : "false", gps_dat->latlon[0][0],
           gps_dat->latlon[0][1], gps_dat->latlon[1][0], gps_dat->latlon[1][1],
           gps_dat->latlon[2][0], gps_dat->latlon[2][1]);
}

void getBestLatLon(gps_data_s *gps_data, gps_lat_lon_s *latlon) {
    if (gps_data->posCheck) {
        uint8_t index = 0;
        for (; index < MAX_GPS_DATA_BUFFER; index++) {
            if (gps_data->gpsReadFlags[index]) {
                latlon->lat = gps_data->latlon[index][0];
                latlon->lon = gps_data->latlon[index][1];
                gps_data->gpsReadFlags[index] = 0;
                if (index == GPS_GGA) {
                    memcpy(latlon->type, "GGA", 4);
                } else if (index == GPS_GLL) {
                    memcpy(latlon->type, "GLL", 4);
                } else {
                    memcpy(latlon->type, "RMC", 4);
                }

                break;
            }
        }
    } else {
        latlon->notAvail = true;
        latlon->lat = DEFAULT_LAT;
        latlon->lon = DEFAULT_LON;
        memcpy(latlon->type, "NONE", 5);
    }
}

void echoGpsOutput(char *line, int buf_len) {
    for (int i = 0; i < buf_len; i++) {
        if (line[i] == '\r') puts_raw("\n[NEW]");
        if (line[i] == '\n') printf("[%d]", i);
        putchar_raw(line[i]);
    }
}

// Init NEO-M8N functions
uint32_t gpsInit(const gps_config_s *gps_cfg) {
    uint32_t baud = uart_init(gps_cfg->uart, gps_cfg->baudrate);
    gpio_set_function(gps_cfg->txPin, GPIO_FUNC_UART);
    gpio_set_function(gps_cfg->rxPin, GPIO_FUNC_UART);
    return baud;
}

// GPS FUNCTIONS [END] ------------------------------------------------
