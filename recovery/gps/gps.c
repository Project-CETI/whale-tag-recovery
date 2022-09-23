#include "gps.h"
#include "../../lib/minmea.h"
#include "../constants.h"
#include "ublox-config.h"
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
            // printf("[RMC] ");
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, line)) {
                memcpy(gps_dat->lastGpsBuffer[GPS_RMC], line, buf_len);
                float latitude = minmea_tocoord(&frame.latitude);
                float longitude = minmea_tocoord(&frame.longitude);
                if (isnan(latitude) || isnan(longitude)) {
                    gps_dat->latlon[GPS_RMC][0] = DEFAULT_LAT;
                    gps_dat->latlon[GPS_RMC][1] = DEFAULT_LON;
                } else {
                    gps_dat->latlon[GPS_RMC][0] = latitude;
                    gps_dat->latlon[GPS_RMC][1] = longitude;
                    gps_dat->gpsReadFlags[GPS_RMC] = 1;
                    gps_dat->posCheck = true;
                    gps_dat->inDominica = inDominica(latitude);
                    gps_dat->timeCheck = true;
                    // Multiple hours by 100 to get the time in military time
                    // printf("RMC time: %d:%d:%d\n", frame.time.hours,
                    //        frame.time.minutes, frame.time.seconds);
                    uint16_t t[3] = {frame.time.hours, frame.time.minutes,
                                    frame.time.seconds};
                    memcpy(gps_dat->timestamp, t, 3);
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
                gps_dat->dateCheck = frame.date.year > 0;
                gps_dat->timeCheck = true;
                // Multiple hours by 100 to get the time in military time
                uint16_t t[3] = {frame.time.hours * 100, frame.time.minutes,
                                 frame.time.seconds};
                memcpy(gps_dat->timestamp, t, 3);
            }

            break;
        }
        case MINMEA_SENTENCE_GLL: {
            // printf("[GLL] ");
            struct minmea_sentence_gll frame;
            if (minmea_parse_gll(&frame, line)) {
                memcpy(gps_dat->lastGpsBuffer[GPS_GLL], line, buf_len);
                float latitude = minmea_tocoord(&frame.latitude);
                float longitude = minmea_tocoord(&frame.longitude);
                if (isnan(latitude) || isnan(longitude)) {
                    gps_dat->latlon[GPS_GLL][0] = DEFAULT_LAT;
                    gps_dat->latlon[GPS_GLL][1] = DEFAULT_LON;
                } else {
                    gps_dat->latlon[GPS_GLL][0] = latitude;
                    gps_dat->latlon[GPS_GLL][1] = longitude;
                    gps_dat->gpsReadFlags[GPS_GLL] = 1;
                    gps_dat->posCheck = true;
                    gps_dat->inDominica = inDominica(latitude);
                    gps_dat->timeCheck = true;
                    // Multiple hours by 100 to get the time in military time
                    // printf("GLL time: %d:%d:%d\n", frame.time.hours,
                    //        frame.time.minutes, frame.time.seconds);

                    uint16_t t[3] = {frame.time.hours, frame.time.minutes,
                                    frame.time.seconds};
                    memcpy(gps_dat->timestamp, t, 3);
                }
            }
            break;
        }
        case MINMEA_SENTENCE_GGA: {
            // printf("[GGA] ");
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, line)) {
                memcpy(gps_dat->lastGpsBuffer[GPS_GGA], line, buf_len);
                float latitude = minmea_tocoord(&frame.latitude);
                float longitude = minmea_tocoord(&frame.longitude);
                if (isnan(latitude) || isnan(longitude)) {
                    gps_dat->latlon[GPS_GGA][0] = DEFAULT_LAT;
                    gps_dat->latlon[GPS_GGA][1] = DEFAULT_LON;
                } else {
                    gps_dat->latlon[GPS_GGA][0] = latitude;
                    gps_dat->latlon[GPS_GGA][1] = longitude;
                    gps_dat->gpsReadFlags[GPS_GGA] = 1;
                    gps_dat->posCheck = true;
                    gps_dat->inDominica = inDominica(latitude);

                    gps_dat->timeCheck = true;
                    // Multiple hours by 100 to get the time in military time
                    // printf("RMC time: %d:%d:%d\n", frame.time.hours,
                    //        frame.time.minutes, frame.time.seconds);

                    uint16_t t[3] = {frame.time.hours, frame.time.minutes,
                                    frame.time.seconds};
                    memcpy(gps_dat->timestamp, t, 3);
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
    // gps_dat->posCheck = false; // todo don't want to reset on just zda
    // datetime messages
    while (uart_is_readable(gps_cfg->uart)) {
        inChar = uart_getc(gps_cfg->uart);
        if (inChar == '$') {
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
            // printf("%s\n", gps_rd_buf);
        }
    }
    return gps_dat->posCheck;
}

void gps_get_lock(const gps_config_s *gps_cfg, gps_data_s *gps_dat,
                  uint32_t timeout) {
    bool lastCheck = gps_dat->posCheck;
    gps_dat->posCheck = false;
    uint32_t startTime = to_ms_since_boot(get_absolute_time());
    while (gps_dat->posCheck != true) {
        if (readFromGps(gps_cfg, gps_dat) ||
            to_ms_since_boot(get_absolute_time()) - startTime > timeout)
            break;
    }
    // TODO delete later
    // if (lastCheck != gps_dat->posCheck) {
    printf("[GPS LOCK] %s  GGA %.2f, %.2f  GLL %.2f, %.2f  RMC %.2f, %.2f\n",
           gps_dat->posCheck ? "true" : "false", gps_dat->latlon[0][0],
           gps_dat->latlon[0][1], gps_dat->latlon[1][0], gps_dat->latlon[1][1],
           gps_dat->latlon[2][0], gps_dat->latlon[2][1]);
    //}
}

void getBestLatLon(gps_data_s *gps_data, gps_lat_lon_s *latlon) {
    if (gps_data->posCheck) {
        uint8_t index = 0;
        for (; index < MAX_GPS_DATA_BUFFER; index++) {
            if (gps_data->gpsReadFlags[index]) {
                latlon->lat = gps_data->latlon[index][0];
                latlon->lon = gps_data->latlon[index][1];
                gps_data->gpsReadFlags[index] = 0;
                if (index == GPS_SIM) {
                    memcpy(latlon->type, "SIM", 4);
                } else if (index == GPS_GGA) {
                    memcpy(latlon->type, "GGA", 4);
                } else if (index == GPS_GLL) {
                    memcpy(latlon->type, "GLL", 4);
                } else {
                    memcpy(latlon->type, "RMC", 4);
                }
                memcpy(latlon->msg, gps_data->lastGpsBuffer[index], MAX_GPS_MSG_LEN);
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

uint8_t getDayOfWeek(datetime_t *dt) {
    uint8_t day = dt->day;
    uint8_t month = dt->month;
    uint16_t year = dt->year;

    // adjust month year
    if (month < 3) {
        month += 12;
        year -= 1;
    }

    // split year
    uint32_t c = year / 100;
    year = year % 100;

    // Zeller's congruence
    return (c / 4 - 2 * c + year + year / 4 + 13 * (month + 1) / 5 + day - 1) %
           7;
}

// Init NEO-M8N functions
uint32_t gpsInit(const gps_config_s *gps_cfg) {
    uint32_t baud = uart_init(gps_cfg->uart, gps_cfg->baudrate);
    gpio_set_function(gps_cfg->txPin, GPIO_FUNC_UART);
    gpio_set_function(gps_cfg->rxPin, GPIO_FUNC_UART);
    return baud;
}

bool inDominica(float lat) { return true; }  // lat < 17.71468; }
/**
 * Calculates the checksum for the current byte stream using the 8-bit Fletcher
 * Algorithm. Requires that the given byte stream has the last two cells
 * reserved to be filled by the calculated checksum. Calculation for the
 * checksum starts at the class field, but does not include the two sync bytes
 * at the start (also known has header bytes). Byte stream should contain both
 * of these sync bytes, as well as space for the two checksum bytes.
 * @param length the length of the byte stream, including the checksum at the
 * end
 * @param byte_stream the array for the byte stream, with the last two cells
 * reserved to be filled by the checksum
 */
void calculateUBXChecksum(uint8_t length, uint8_t *byte_stream) {
    uint8_t CK_A = 0;
    uint8_t CK_B = 0;

    for (uint8_t i = 2; i < length - 2; i++) {
        CK_A = CK_A + byte_stream[i];
        CK_B = CK_B + CK_A;
    }

    byte_stream[length - 2] = CK_A;
    byte_stream[length - 1] = CK_B;
    // printf("check sum: %02x %02x\n", CK_A, CK_B);
}

void createUBXSleepPacket(uint32_t time, ubx_cfg_t *sleep) {
    uint32_t converted = 0;
    converted |= ((0x000000ff & time) << 24);
    converted |= ((0x0000ff00 & time) << 8);
    converted |= ((0x00ff0000 & time) >> 8);
    converted |= ((0xff000000 & time) >> 24);
    // printf("[CREATE SLEEP] T: %d %x\tCON: %x\n", time, time, converted);
    sleep_temp[6] |= converted >> 24;
    sleep_temp[7] |= (converted << 8) >> 24;
    sleep_temp[8] |= (converted << 16) >> 24;
    sleep_temp[9] |= (converted << 24) >> 24;
    calculateUBXChecksum(16, sleep_temp);
    memcpy(sleep, sleep_temp, 16);
}

bool writeAllConfigurationsToUblox(uart_inst_t *uart) {
    for (uint8_t i = 0; i < NUM_UBLOX_CONFIGS; i++) {
        // do bit magic for the length
        uint16_t length = (((uint16_t)ubx_configurations[i][5] << 8) |
                           ubx_configurations[i][4]) +
                          0x8;
        calculateUBXChecksum(length, ubx_configurations[i]);
        writeSingleConfiguration(uart, ubx_configurations[i], length);
    }
}

void writeSingleConfiguration(uart_inst_t *uart, uint8_t *byte_stream,
                              uint8_t len) {
    // printf("Writing Configuration: %x %x\n", byte_stream[2], byte_stream[3]);
    if (!uart_is_writable(uart)) uart_tx_wait_blocking(uart);
    for (uint8_t i = 0; i < len; i++) {
        uart_putc_raw(uart, byte_stream[i]);
        // printf("%02x ", byte_stream[i]);
    }
    // printf("\n");

    sleep_ms(1000);
}

// GPS FUNCTIONS [END] ------------------------------------------------
