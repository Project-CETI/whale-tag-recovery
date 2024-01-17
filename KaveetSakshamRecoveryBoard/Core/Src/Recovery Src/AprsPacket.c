/*
 * AprsPacket.c
 *
 *  Created on: Jul 5, 2023
 *      Author: Kaveet
 *              Michael Salino-Hugg (msalinohugg@seas.harvard.edu)
 */

#include "Recovery Inc/AprsPacket.h"
#include "Recovery Inc/Aprs.h"
#include "Sensor Inc/BatteryMonitoring.h"
#include "main.h"
#include "timing.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct callsign_t {
    char callsign[7];
    uint8_t ssid;
} Callsign;

static struct {
    Callsign src;
    Callsign msg_recipient;
    char comment[41];
} aprs_config = {
    .src = {
        .callsign 	= APRS_SOURCE_CALLSIGN,
        .ssid 		= APRS_SOURCE_SSID,
    },
	.msg_recipient = {
		.callsign   = APRS_MESSAGE_RECIPIENT_CALLSIGN,
		.ssid       = APRS_MESSAGE_RECIPIENT_SSID,
	},
    .comment = "",
};

typedef struct ax25_frame_t {
	Callsign destination;
	Callsign source;
	Callsign digipeter[8];
	struct {
		uint8_t *value;
		size_t len;
	} information;
} AX25Frame;

static void append_gps_data(uint8_t * buffer, float lat, float lon);
static void append_compressed_gps_data(uint8_t *buffer, float lat, float lon);
static void append_timestamp(uint8_t *buffer, const uint16_t timestamp[3]);
static void append_comment(uint8_t *buffer, size_t max_len, const char *comment);

/* Callsign ******************************************************************/
// generates a valid AX.25 address from a callsign
static void callsign_to_ax25_address(const Callsign *self, uint8_t dst[static 7], uint8_t **dst_end){
    //Determine the length of the callsign
    uint8_t length  = strlen(self->callsign);

    //Append the callsign to the buffer. Note that ASCII characters must be left shifted by 1 bit as per APRS101 standard
    for (uint8_t index = 0; index < length; index++){
        dst[index] = (self->callsign[index] << 1);
    }

    //The callsign field must be atleast 6 characters long, so fill any missing spots with blanks
    if (length < APRS_CALLSIGN_LENGTH){
        memset(dst + length, (' ' << 1), APRS_CALLSIGN_LENGTH - length);
    }

    //Now, we've filled the first 6 bytes with the callsign (index 0-5), so the SSID must be in the 6th index.
    //We can find its ASCII character by adding the integer value to the ascii value of '0'. Still need to shift left by 1 bit.
    dst[APRS_CALLSIGN_LENGTH] = (self->ssid + '0') << 1;
    *dst_end = dst + 7;
}

// converts callsign into a valid string
static void callsign_to_string(const Callsign *self, char str[static 10]) {
    if(self->ssid == 0) {
        strncpy(str, self->callsign, 6);
    } else {
        snprintf(str, 10, "%s-%d", self->callsign, self->ssid);
    }
}

/* AX25Frame *****************************************************************/
static void ax25_frame_generate_bytes(const AX25Frame *self, uint8_t *dst, uint8_t **dst_end){
	uint8_t *dst_next = dst;

	callsign_to_ax25_address(&self->destination, dst_next, &dst_next);
	callsign_to_ax25_address(&self->source, dst_next, &dst_next);
	for(int i = 0;  (i < 8) && (self->digipeter[i].callsign[0] != 0); i++){
		callsign_to_ax25_address(&self->digipeter[0], dst_next, &dst_next);
		if(i == 0) {
			dst_next[-1] += 1;
		}
	}

	*(dst_next++) = APRS_CONTROL_FIELD;
	*(dst_next++) = APRS_PROTOCOL_ID;

    // fill in data
	memcpy(dst_next, self->information.value, self->information.len);
	dst_next += self->information.len;

	//Calculates and appends the CRC frame checker. Follows the CRC-16 CCITT standard.
    uint16_t crc = 0xFFFF;
    for (uint8_t *i_byte = dst; i_byte < dst_next; i_byte++){
        for (uint8_t bit_index = 0; bit_index < 8; bit_index++){
            bool bit = (*i_byte >> bit_index) & 0x01;
            //Bit magic for the CRC
            unsigned short xorIn;
            xorIn = crc ^ bit;
            crc >>= 1;
            if (xorIn & 0x01) crc ^= 0x8408;
        }
    }
    *(dst_next++) = (crc & 0xFF) ^ 0xFF;
    *(dst_next++) = (crc >> 8) ^ 0xFF;

    if(dst_end != NULL){
        *dst_end = dst_next;
    }
}

static void __ax25_generate_packet(uint8_t *buffer, uint8_t **buffer_end, uint8_t *aprs_data, size_t aprs_data_size){
    uint8_t *buffer_position = buffer;
    AX25Frame frame = {
        .destination = {.callsign = APRS_DESTINATION_CALLSIGN, .ssid = APRS_DESTINATION_SSID},
        .source = aprs_config.src,
        .digipeter = {
                [0] = {.callsign = APRS_DIGI_PATH, .ssid = (APRS_DIGI_SSID)},
        },
        .information = {
            .value = aprs_data,
            .len = aprs_data_size,
        },
	};

	//TXDelayFlags
	memset(buffer_position, APRS_FLAG, AX25_FLAG_COUNT);
	buffer_position += AX25_FLAG_COUNT;

	ax25_frame_generate_bytes(&frame, buffer_position, &buffer_position);

	//EndFlag
	memset(buffer_position, APRS_FLAG, 3);
	buffer_position += 3;
    if(buffer_end != NULL) {
        *buffer_end = buffer_position;
    }
}

    static uint16_t message_index = 0;
void aprs_generate_location_packet(uint8_t * buffer, uint8_t **buffer_end, float lat, float lon){
    char index_buffer[6];
    uint8_t gps_data[256];
	size_t  gps_data_size = 0;

	//generate APRS gps packet
    gps_data[0] = APRS_DT_POS_CHARACTER;
    gps_data_size += 1;

	append_compressed_gps_data(&gps_data[gps_data_size], lat, lon);
    gps_data_size += 13;

    //message_index
    sprintf(index_buffer, "%04x:", message_index);
    append_comment(&gps_data[gps_data_size], 40, index_buffer);
    message_index++;
    gps_data_size += 5;

    //voltage_level
    sprintf(index_buffer, "%3.1f;", battery_monitor_get_true_voltage());
    append_comment(&gps_data[gps_data_size], 35, index_buffer);
    gps_data_size += 4;

    append_comment(&gps_data[gps_data_size], 31, aprs_config.comment);
	gps_data_size += strlen(aprs_config.comment);

	//package inside AX.25 frame
    __ax25_generate_packet(buffer, buffer_end, gps_data, gps_data_size);
}

void aprs_generate_location_packet_w_timestamp(uint8_t * buffer, uint8_t **buffer_end, float lat, float lon, const uint16_t timestamp[3]){
    char index_buffer[6];
    uint8_t gps_data[256];
	size_t  gps_data_size = 0;

	//generate APRS gps packet
    gps_data[0] = '/';
    gps_data_size += 1;

    append_timestamp(&gps_data[gps_data_size], timestamp);
    gps_data_size += 7;

	append_compressed_gps_data(&gps_data[gps_data_size], lat, lon);
    gps_data_size += 13;

    sprintf(index_buffer, "%04d:", (message_index - 1));
    append_comment(&gps_data[gps_data_size], 40, index_buffer);
    gps_data_size += 5;

    append_comment(&gps_data[gps_data_size], 35, APRS_COMMENT);
	gps_data_size += strlen(APRS_COMMENT);

	//package inside AX.25 frame
    __ax25_generate_packet(buffer, buffer_end, gps_data, gps_data_size);
}

void aprs_generate_message_packet(uint8_t *buffer, uint8_t **buffer_end, const char* message, size_t message_len){
	char addressee[10] = "KC1QXQ-8";
    uint8_t message_bytes[256] = {':'};
	size_t  byte_len = 11 + message_len;
    
    //generate callsign string
    callsign_to_string(&aprs_config.msg_recipient, addressee);

	//address message
    memset(message_bytes + 1, ' ', 9);
    memcpy(message_bytes + 1, addressee, strlen(addressee));
    message_bytes[10] = ':';

    //append message
    memcpy(message_bytes + 11, message, message_len);
    
    //package inside AX.25 frame
	__ax25_generate_packet(buffer, buffer_end, message_bytes, byte_len);
}

//Appends the GPS data (latitude and longitude) to the buffer
__attribute((unused))
static void append_gps_data(uint8_t * buffer, float lat, float lon){

    //indicate start of real-time transmission
    buffer[0] = APRS_DT_POS_CHARACTER;

    //First, create the string containing the latitude and longitude data, then save it into our buffer
    bool is_north = true;

    //If we have a negative value, then the location is in the southern hemisphere.
    //Recognize this and then just use the magnitude of the latitude for future calculations.
    if (lat < 0){
        is_north = false;
        is_north *= -1;
    }

    //The coordinates we get from the GPS are in degrees and fractional degrees
    //We need to extract the whole degrees from this, then the whole minutes and finally the fractional minutes

    //The degrees are just the rounded-down integer
    uint8_t lat_deg_whole = (uint8_t) lat;

    //Find the remainder (fractional degrees) and multiply it by 60 to get the minutes (fractional and whole)
    float lat_minutes = (lat - lat_deg_whole) * 60;

    //Whole number minutes is just the fractional component.
    uint8_t lat_minutes_whole = (uint8_t) lat_minutes;

    //Find the remainder (fractional component) and save it to two decimal points (multiply by 100 and cast to int)
    uint8_t lat_minutes_frac = (lat_minutes - lat_minutes_whole) * 100;

    //Find our direction indicator (N for North of S for south)
    char lat_direction = (is_north) ? 'N' : 'S';

    //Create our string. We use the format ddmm.hh(N/S), where "d" is degrees, "m" is minutes and "h" is fractional minutes.
    //Store this in our buffer.
    snprintf((char *)&buffer[1], APRS_LATITUDE_LENGTH, "%02d%02d.%02d%c", lat_deg_whole, lat_minutes_whole, lat_minutes_frac, lat_direction);


    //Right now we have the null-terminating character in the buffer "\0". Replace this with our latitude and longitude seperating symbol "1".
    buffer[APRS_LATITUDE_LENGTH] = APRS_SYM_TABLE_CHAR;

    //Now, repeat the process for longitude.
    bool is_east = true;

    //If its less than 0, remember it as West, and then take the magnitude
    if (lon < 0){
        is_east = false;
        lon *= -1;
    }

    //Find whole number degrees
    uint8_t lon_deg_whole = (uint8_t) lon;

    //Find remainder (fractional degrees), convert to minutes
    float lon_minutes = (lon - lon_deg_whole) * 60;

    //Find whole number and fractional minutes. Take two decimal places for the fractional minutes, just like before
    uint8_t lon_minutes_whole = (uint8_t) lon_minutes;
    uint8_t lon_minutes_fractional = (lon_minutes - lon_minutes_whole) * 100;

    //Find direction character
    char lon_direction = (is_east) ? 'E' : 'W';

    //Store this in the buffer, in the format dddmm.hh(E/W)
    snprintf((char *)&buffer[APRS_LATITUDE_LENGTH + 1], APRS_LONGITUDE_LENGTH, "%03d%02d.%02d%c", lon_deg_whole, lon_minutes_whole, lon_minutes_fractional, lon_direction);

    //Appending payload character indicating the APRS symbol (using boat symbol). Replace the null-terminating character with it.
    buffer[APRS_LATITUDE_LENGTH + APRS_LONGITUDE_LENGTH] = APRS_SYM_CODE_CHAR;
}

static void append_compressed_gps_data(uint8_t *buffer, float lat, float lon){
    //indicate start of real-time transmission
    uint32_t temp_lat = (uint32_t)(380926.0 * (90.0 - lat));
    uint32_t temp_lon = (uint32_t)(190463.0 * (180.0 + lon));
    
    buffer[0] = '/';

    for(int i = 3; i >= 0; i--){
        buffer[1 + i] = '!' + temp_lat % 91;
        temp_lat /= 91;
    }

    for(int i = 3; i >= 0; i--){
        buffer[5 + i] = '!' + temp_lon % 91;
        temp_lon /= 91;
    }

    buffer[9] = APRS_SYM_CODE_CHAR;
    buffer[10] = ' '; //c: ' ' means no course-speed/range/altitude
    buffer[11] = 's';
    buffer[12] = 'T';
}

static void append_timestamp(uint8_t *buffer, const uint16_t timestamp[3]){
    char timestamp_buffer[8];
    sprintf(timestamp_buffer, "%02d%02d%02dh", timestamp[0], timestamp[1], timestamp[2]);
    memcpy(buffer, timestamp_buffer, 7);

}

static void append_comment(uint8_t *buffer, size_t max_len, const char *comment){
    size_t len = strlen(comment);
    len = (len < max_len) ? len : max_len;
    memcpy(buffer, comment, len);
}

__attribute((unused))
static void append_comp_gps_w_timestamp(uint8_t *buffer, float lat, float lon, uint16_t timestamp[3]){
    //indicate start of real-time transmission
    uint32_t temp_lat = (uint32_t)(380926.0 * (90.0 - lat));
    uint32_t temp_lon = (uint32_t)(190463.0 * (180.0 + lon));
    
    buffer[0] = '/';
    //copy timestamp in HMS format;
    sprintf((char *)(buffer + 1), "%02d%02d%02dh", timestamp[0], timestamp[1], timestamp[2]);
    
    buffer[8] = '/';
    for(int i = 3; i >= 0; i--){
        buffer[9 + i] = '!' + temp_lat % 91;
        temp_lat /= 91;
    }

    for(int i = 3; i >= 0; i--){
        buffer[6 + i] = '!' + temp_lon % 91;
        temp_lon /= 91;
    }

    buffer[17] = APRS_SYM_CODE_CHAR;
    buffer[18] = ' '; //c: ' ' means no course-speed/range/altitude
    buffer[19] = 's';
    buffer[20] = 'T';
}

int aprs_set_msg_recipient_callsign(const char *callsign) {
	size_t len = strlen(callsign);

	if (len > 6) // callsign too long
		return - -1;

	memcpy(aprs_config.msg_recipient.callsign, callsign, len + 1);
	return 0;
}

int aprs_set_msg_recipient_ssid(uint8_t ssid) {
    if( ssid > 15) //out of range
        return -1;

    aprs_config.msg_recipient.ssid = ssid;
    return 0;
}

int aprs_set_callsign(const char *callsign){
	size_t len = strlen(callsign);

	if (len > 6) // callsign too long
		return - -1;

	memcpy(aprs_config.src.callsign, callsign, len + 1);
	return 0;
}

int aprs_set_ssid(uint8_t ssid) {
    if( ssid > 15) //out of range
        return -1;

    aprs_config.src.ssid = ssid;
    return 0;
}

void aprs_get_callsign(char callsign[static 7]){
	memcpy(callsign, aprs_config.src.callsign, 6);
    callsign[6] = 0;
}

void aprs_get_ssid(uint8_t *ssid){
    *ssid = aprs_config.src.ssid; 
}

void aprs_set_comment(const char *comment, size_t comment_len) {
    comment_len = (comment_len < APRS_MAX_COMMENT_LEN) ? comment_len : APRS_MAX_COMMENT_LEN;
    memcpy(aprs_config.comment, comment, comment_len);
    aprs_config.comment[comment_len] = '\0';
}
