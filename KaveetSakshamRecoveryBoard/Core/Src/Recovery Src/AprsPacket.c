/*
 * AprsPacket.c
 *
 *  Created on: Jul 5, 2023
 *      Author: Kaveet
 *              Michael Salino-Hugg (msalinohugg@seas.harvard.edu)
 */

#include "Recovery Inc/AprsPacket.h"
#include "Recovery Inc/Aprs.h"
#include "main.h"
#include "timing.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

static struct {
	char 	src_callsign[7];
	uint8_t src_ssid;
} aprs_config = {
	.src_callsign 	= APRS_SOURCE_CALLSIGN,
	.src_ssid 		= APRS_SOURCE_SSID,
};


static void append_flag(uint8_t * buffer, uint16_t numFlags);
static void append_callsign(uint8_t * buffer, char * callsign, uint8_t ssid);
static void append_gps_data(uint8_t * buffer, float lat, float lon);
static void append_compressed_gps_data(uint8_t *buffer, float lat, float lon, char * comment);
static void append_other_data(uint8_t * buffer, uint16_t course, uint16_t speed, char * comment);
static void append_frame_check(uint8_t * buffer, uint8_t buffer_length);

void aprs_generate_packet(uint8_t * buffer, float lat, float lon){
    uint_fast8_t offset = 0;
    memset(buffer, APRS_FLAG, AX25_FLAG_COUNT);
    offset += AX25_FLAG_COUNT;

    append_callsign(&buffer[offset], APRS_DESTINATION_CALLSIGN, APRS_DESTINATION_SSID);
    offset += 7;

    append_callsign(&buffer[offset], aprs_config.src_callsign, aprs_config.src_ssid);
    offset += 7;

    //We can also treat the digipeter as a callsign since it has the same format
    append_callsign(&buffer[offset], APRS_DIGI_PATH, APRS_DIGI_SSID);
    offset += 6;
    buffer[offset++] += 1;

    //Add the control ID and protocol ID
    buffer[offset++] = APRS_CONTROL_FIELD;
    buffer[offset++] = APRS_PROTOCOL_ID;

    //Attach the payload (including other control characters)
    append_compressed_gps_data(&buffer[AX25_FLAG_COUNT + 23], lat, lon, APRS_COMMENT); //42.3636 -71.1259
    offset += 14 + strlen(APRS_COMMENT) + 4;

    append_frame_check(buffer, offset);
    offset += 2;

    append_flag(&buffer[offset], 3);
    offset = 3;
}

void aprs_generate_message_packet(uint8_t *buffer, const char* message, size_t message_len){
    uint_fast8_t offset = 0;
    memset(buffer, APRS_FLAG, AX25_FLAG_COUNT);
    offset += AX25_FLAG_COUNT;

    append_callsign(&buffer[offset], APRS_DESTINATION_CALLSIGN, APRS_DESTINATION_SSID);
    offset += 7;

    append_callsign(&buffer[offset], APRS_SOURCE_CALLSIGN, aprs_config.src_ssid);
    offset += 7;

    //We can also treat the digipeter as a callsign since it has the same format
    append_callsign(&buffer[offset], APRS_DIGI_PATH, APRS_DIGI_SSID);
    offset += 6;
    buffer[offset++] += 1;

    //Add the control ID and protocol ID
    buffer[offset++] = APRS_CONTROL_FIELD;
    buffer[offset++] = APRS_PROTOCOL_ID;

    /*** Message Format ***/
    //addressee
    buffer[offset++] = ':';
    memcpy(buffer + offset, "KC1TUJ   ", 9);    
    offset += 9;
    buffer[offset++] = ':';

    //message
    memcpy(buffer + offset, message, message_len);
    offset += message_len;

    append_frame_check(buffer, offset);
    offset += 2;

    append_flag(&buffer[offset], 3);
    offset = 3;
}

//Appends the flag character (0x7E) to the buffer 'numFlags' times.
static void append_flag(uint8_t * buffer, uint16_t numFlags){

    //Add numFlags flag characters to the buffer
    for (uint_fast16_t index = 0; index < numFlags; index++){
        buffer[index] = APRS_FLAG;
    }
}

//Appends a callsign to the buffer with its SSID.
static void append_callsign(uint8_t * buffer, char * callsign, uint8_t ssid){

    //Determine the length of the callsign
    uint8_t length  = strlen(callsign);

    //Append the callsign to the buffer. Note that ASCII characters must be left shifted by 1 bit as per APRS101 standard
    for (uint8_t index = 0; index < length; index++){
        buffer[index] = (callsign[index] << 1);
    }

    //The callsign field must be atleast 6 characters long, so fill any missing spots with blanks
    if (length < APRS_CALLSIGN_LENGTH){
        for (uint8_t index = length; index < APRS_CALLSIGN_LENGTH; index++){
            //We still need to shift left by 1 bit
            buffer[index] = (' ' << 1);
        }
    }

    //Now, we've filled the first 6 bytes with the callsign (index 0-5), so the SSID must be in the 6th index.
    //We can find its ASCII character by adding the integer value to the ascii value of '0'. Still need to shift left by 1 bit.
    buffer[APRS_CALLSIGN_LENGTH] = (ssid + '0') << 1;
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

static void append_compressed_gps_data(uint8_t *buffer, float lat, float lon, char * comment){
	static uint16_t message_index = 0;
	char index_buffer[5];
    //indicate start of real-time transmission
    uint32_t temp_lat = (uint32_t)(380926.0 * (90.0 - lat));
    uint32_t temp_lon = (uint32_t)(190463.0 * (180.0 + lon));
    
    buffer[0] = APRS_DT_POS_CHARACTER;
    buffer[1] = '/';

    for(int i = 3; i >= 0; i--){
        buffer[2 + i] = '!' + temp_lat % 91;
        temp_lat /= 91;
    }

    for(int i = 3; i >= 0; i--){
        buffer[6 + i] = '!' + temp_lon % 91;
        temp_lon /= 91;
    }

    buffer[10] = APRS_SYM_CODE_CHAR;
    buffer[11] = ' '; //c: ' ' means no course-speed/range/altitude
    buffer[12] = 's';
    buffer[13] = 'T';


    sprintf(index_buffer, "%03d:", message_index);
    memcpy(&buffer[14], index_buffer, 4);
    memcpy(&buffer[14 + 4], comment, strlen(comment));
    message_index++;
}



//Appends other extra data (course, speed and the comment)
__attribute((unused))
static void append_other_data(uint8_t * buffer, uint16_t course, uint16_t speed, char * comment){

    //Append the course and speed of the tag (course is the heading 0->360 degrees
    uint8_t length = 8 + strlen(comment);
    snprintf((char *)buffer, length, "%03d/%03d%s", course, speed, comment);
}

//Calculates and appends the CRC frame checker. Follows the CRC-16 CCITT standard.
static void append_frame_check(uint8_t * buffer, uint8_t buffer_length){

    uint16_t crc = 0xFFFF;

    //Loop through each *bit* in the buffer. Only start after the starting flags.
    for (uint8_t index = 150; index < buffer_length; index++){

        uint8_t byte = buffer[index];

        for (uint8_t bit_index = 0; bit_index < 8; bit_index++){

            bool bit = (byte >> bit_index) & 0x01;

            //Bit magic for the CRC
            unsigned short xorIn;
            xorIn = crc ^ bit;

            crc >>= 1;

            if (xorIn & 0x01) crc ^= 0x8408;

        }
    }

    uint8_t crc_lo = (crc & 0xFF) ^ 0xFF;
    uint8_t crc_hi = (crc >> 8) ^ 0xFF;

    buffer[buffer_length] = crc_lo;
    buffer[buffer_length + 1] = crc_hi;
}

int aprs_set_callsign(const char *callsign){
	size_t len = strlen(callsign);

	if (len > 6) // callsign too long
		return - -1;

	memcpy(aprs_config.src_callsign, callsign, len + 1);
	return 0;
}

void aprs_get_callsign(char callsign[static 7]){
	memcpy(callsign, aprs_config.src_callsign, 6);
    callsign[6] = 0;
}

void aprs_get_ssid(uint8_t *ssid){
    *ssid = aprs_config.src_ssid; 
}
