/*
 * GPS.c
 *
 *  Created on: Jun 22, 2023
 *      Author: Kaveet, Michael Salino-Hugg
 *
 * The C file for parsing and storing the GPS outputs for board recovery. See matching header file for more info.
 */
#include "tx_api.h"
#include "Recovery Inc/GPS.h"
#include "Lib Inc/minmea.h"
#include <math.h>
#include <string.h>
#include "Comms Inc/PiComms.h"
#include "stm32u5xx_hal_uart.h"
#include "stm32u5xx_hal_uart_ex.h"
#include "main.h"

#ifdef GPS_COMM_DEBUG
#include "Lib Inc/timing.h"
#endif

//For parsing GPS outputs
static void parse_gps_output(GPS_HandleTypeDef* gps, const char* buffer, uint8_t buffer_length);

extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef handle_GPDMA1_Channel0;


// === PRIVATE DEFINES ===
#define NMEA_START_CHAR '$'
#define NMEA_END_CHAR   '\r'
#define NMEA_MAX_SIZE   (82)

#define GPS_BUFFER_SIZE (NMEA_MAX_SIZE + 1)
#define GPS_BUFFER_COUNT (128)

#define GPS_BUFFER_VALID_START (1 << 0)

// === PRIVATE TYPEDEFS ===
typedef struct {
	size_t length;
	uint8_t sentence[GPS_BUFFER_SIZE];
}NmeaString;

typedef struct {
    uint8_t some;
    size_t value;
} Option_size_t;


// === PRIVATE VARIABLES ===
uint8_t rx_buffer[16*NMEA_MAX_SIZE];
TX_EVENT_FLAGS_GROUP gpsBuffer_event_flags_group;
 NmeaString gps_buffer[GPS_BUFFER_COUNT] = {};
volatile size_t gpsBuffer_write_index = 0;
size_t gpsBuffer_read_index = 0;
volatile Option_size_t gpsBuffer_newest_index = {.some = 0};
volatile uint8_t found_start = 0;

// === PRIVATE METHODS ===

// === PUBLIC METHODS ===
/**
 * @brief UART IT complete callback
 * 
 * @param huart 
 */

void GPS_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
 	//restarts the DMA listening of the UART rx (next 247 bytes)
	// seperate messages
	int nmea_start = -1;
	int new = 0;
	for (int i = 0; i < Size; i++){
		//find sentence start
		switch(rx_buffer[i]) {
			case NMEA_START_CHAR:
				nmea_start = i;
				break;

			case '\n': /* fallthrough */
			case '\r':
				if (nmea_start == -1)
					break; //sentence end with no start if you reach here

				gps_buffer[gpsBuffer_write_index].length = ((i) - nmea_start);
				memcpy(&gps_buffer[gpsBuffer_write_index].sentence[0], &rx_buffer[nmea_start], gps_buffer[gpsBuffer_write_index].length);
				gps_buffer[gpsBuffer_write_index].sentence[gps_buffer[gpsBuffer_write_index].length] = '\0';
				gpsBuffer_write_index = (gpsBuffer_write_index + 1) % GPS_BUFFER_COUNT;
				new = 1;
				//flag thread to process sentence
				nmea_start = -1;
				break;

			default:
				break;
		}
	}
	int remaining = 0;
	//(nmea_start == -1) => no start found //flush entire buffer
	//(nmea_start == 0) => sentence too long //flush entire buffer
	if (nmea_start > 0) {
		remaining = sizeof(rx_buffer) - nmea_start;
		memmove(&rx_buffer[0], &rx_buffer[nmea_start], remaining); //shift sentence start to beginning of buffer
	}

	if(new){
		tx_event_flags_set(&gpsBuffer_event_flags_group, GPS_BUFFER_VALID_START, TX_OR);
	}

	HAL_UARTEx_ReceiveToIdle_DMA(&huart3, &rx_buffer[remaining], sizeof(rx_buffer) - remaining); //initiate next transfer
	__HAL_DMA_DISABLE_IT(&handle_GPDMA1_Channel0,DMA_IT_HT);//we don't want the half done transaction interrupt
	return;
}

/**
 * @brief Returns the latest raw gps message pointer if it exists, otherwise returns NULL ptr;
 * 
 * @return const uint8_t* 
 */
const uint8_t * gpsBuffer_pop_latest(void) {
    //check if new messages
    if (!gpsBuffer_newest_index.some)
        return NULL;

    const uint8_t *msg_ptr = gps_buffer[gpsBuffer_newest_index.value].sentence;
    gpsBuffer_newest_index.some = 0; //make newest index as old
    return msg_ptr;
}

// This thread reads nmea sentence from the gps
/**
 * @brief This thread reads values from
 * 
 * @param thread_input 
 */
void gpsBuffer_thread(ULONG thread_input) {
#ifndef GPS_COMM_DEBUG 
	tx_event_flags_create(&gpsBuffer_event_flags_group, "GPS Buffer Event Flags");

	//initiate UART DMA
	HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rx_buffer, sizeof(rx_buffer));
	__HAL_DMA_DISABLE_IT(&handle_GPDMA1_Channel0,DMA_IT_HT);//we don't want the half done transaction interrupt
	while(1) {
        //wait for gps message start
        ULONG actual_flags = 0;

        tx_event_flags_get(&gpsBuffer_event_flags_group, GPS_BUFFER_VALID_START , TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);

        while(gpsBuffer_read_index != gpsBuffer_write_index){
			NmeaString *read_sentence = &gps_buffer[gpsBuffer_read_index];
			pi_comms_tx_forward_gps(read_sentence->sentence, read_sentence->length);
			gpsBuffer_newest_index.value = gpsBuffer_read_index;
			gpsBuffer_newest_index.some = 1;

            gpsBuffer_read_index = (gpsBuffer_read_index + 1) % GPS_BUFFER_COUNT;
        }
	}
#else
	// buffer a indexed packet every second
	static uint32_t packet_index = 0;
    while(1){
        //create fake message to be buffered and logged
		snprintf((char *)gps_buffer[gpsBuffer_write_index], GPS_BUFFER_SIZE, "%08lxh\r\n", packet_index);
		pi_comms_tx_forward_gps(gps_buffer[gpsBuffer_write_index], 12);
		gpsBuffer_write_index = (gpsBuffer_write_index + 1) % GPS_BUFFER_COUNT;
		packet_index++;
		tx_thread_sleep(tx_s_to_ticks(1));
    }
#endif
}

HAL_StatusTypeDef initialize_gps(UART_HandleTypeDef* huart, GPS_HandleTypeDef* gps){

	gps->huart = huart;

	//TODO: Other initialization like configuring GPS output types or other parameter setting.
	return HAL_OK;
}


bool read_gps_data(GPS_HandleTypeDef* gps){
#if GPS_SIMULATION
		gps->data[GPS_SIM] = (GPS_Data){
			.is_valid_data = true,
			.latitude = GPS_SIM_LAT,
			.longitude = GPS_SIM_LON,
			.is_dominica = is_in_dominica(GPS_SIM_LAT, GPS_SIM_LON),
		};

		gps->is_pos_locked = true;

		return true;
#else
	//check if there is a new packet
	const uint8_t *latest_message = gpsBuffer_pop_latest();
	if (latest_message == NULL) {
		return false;
	}
	size_t msg_len = strlen((const char *)latest_message);

	//parse packet if it exists
	parse_gps_output(gps, (const char *)latest_message, msg_len);

	return true;

#endif
}

#if GPS_SIMULATION
__attribute__((unused))
#endif
static void parse_gps_output(GPS_HandleTypeDef* gps, const char* buffer, uint8_t buffer_length){

	enum minmea_sentence_id sentence_id = minmea_sentence_id((const char *)buffer, false);

	float lat;
	float lon;

	switch (sentence_id) {

	case MINMEA_SENTENCE_RMC: {
		;
		struct minmea_sentence_rmc frame;
		if (minmea_parse_rmc(&frame, (const char *)buffer)){

			lat = minmea_tocoord(&frame.latitude);
			lon = minmea_tocoord(&frame.longitude);

			//Ensure the data is valid or not.
			if (isnan(lat) || isnan(lon)){

				//data invalid, set the default values and indicate invalid data
				gps->data[GPS_RMC].latitude = DEFAULT_LAT;
				gps->data[GPS_RMC].longitude = DEFAULT_LON;
				gps->data[GPS_RMC].is_valid_data = false;

			} else {

				//data is valid, save the latitude and longitude + valid data flags
				gps->data[GPS_RMC].latitude = lat;
				gps->data[GPS_RMC].longitude = lon;
				gps->data[GPS_RMC].is_valid_data = true;
				gps->data[GPS_RMC].is_dominica = is_in_dominica(lat, lon);
				gps->is_pos_locked = true;

				//save the time data into our struct.
				uint16_t time_temp[3] = {frame.time.hours, frame.time.minutes, frame.time.seconds};
				memcpy(gps->data[GPS_RMC].timestamp, time_temp, 6);
			}
		}

		break;
	}
	case MINMEA_SENTENCE_GLL: {
		;
		struct minmea_sentence_gll frame;
		if (minmea_parse_gll(&frame, (const char *)buffer)){

			lat = minmea_tocoord(&frame.latitude);
			lon = minmea_tocoord(&frame.longitude);

			//Ensure the data is valid or not.
			if (isnan(lat) || isnan(lon)){

				//data invalid, set the default values and indicate invalid data
				gps->data[GPS_GLL].latitude = DEFAULT_LAT;
				gps->data[GPS_GLL].longitude = DEFAULT_LON;
				gps->data[GPS_GLL].is_valid_data = false;

			} else {

				//data is valid, save the latitude and longitude + valid data flags
				gps->data[GPS_GLL].latitude = lat;
				gps->data[GPS_GLL].longitude = lon;
				gps->data[GPS_GLL].is_valid_data = true;
				gps->data[GPS_GLL].is_dominica = is_in_dominica(lat, lon);
				gps->is_pos_locked = true;

				//save the time data into our struct.
				uint16_t time_temp[3] = {frame.time.hours, frame.time.minutes, frame.time.seconds};
				memcpy(gps->data[GPS_GLL].timestamp, time_temp, 6);
			}
		}

		break;
	}
	case MINMEA_SENTENCE_GGA: {
		struct minmea_sentence_gga frame;
		if (minmea_parse_gga(&frame, (const char *)buffer)){

			lat = minmea_tocoord(&frame.latitude);
			lon = minmea_tocoord(&frame.longitude);

			//Ensure the data is valid or not.
			if (isnan(lat) || isnan(lon)){

				//data invalid, set the default values and indicate invalid data
				gps->data[GPS_GGA].latitude = DEFAULT_LAT;
				gps->data[GPS_GGA].longitude = DEFAULT_LON;
				gps->data[GPS_GGA].is_valid_data = false;

			} else {

				//data is valid, save the latitude and longitude + valid data flags
				gps->data[GPS_GGA].latitude = lat;
				gps->data[GPS_GGA].longitude = lon;
				gps->data[GPS_GGA].is_valid_data = true;
				gps->data[GPS_GGA].is_dominica = is_in_dominica(lat, lon);
				gps->is_pos_locked = true;

				//save the time data into our struct.
				uint16_t time_temp[3] = {frame.time.hours, frame.time.minutes, frame.time.seconds};
				memcpy(gps->data[GPS_GGA].timestamp, time_temp, 6);
			}
		}

		break;
	}
	default:

		break;
	}
}

bool get_gps_lock(GPS_HandleTypeDef* gps, GPS_Data* gps_data){

	gps->is_pos_locked = false;

	//ensure all valid data flags are false
	for (GPS_MsgTypes msg_type = GPS_SIM; msg_type < GPS_NUM_MSG_TYPES; msg_type++){
		gps->data[msg_type].is_valid_data = false;
	}

	//time trackers for any possible timeouts
	uint32_t start_time = HAL_GetTick();
	uint32_t current_time = start_time;

	//Keep trying to read the GPS data until we get a lock, or we timeout
	while (!gps->is_pos_locked && ((current_time - start_time) < GPS_TRY_LOCK_TIMEOUT)){
		read_gps_data(gps);
		current_time = HAL_GetTick();
	}

	//Populate the GPS data struct that we are officially returning to the caller. Prioritize message types in the order they appear in the enum definiton
	for (GPS_MsgTypes msg_type = GPS_SIM; msg_type < GPS_NUM_MSG_TYPES; msg_type++){
		if (gps->data[msg_type].is_valid_data){

			//Set the message type
			gps->data[msg_type].msg_type = msg_type;

			//Copy into the struct that returns back to the user
			memcpy(gps_data, &gps->data[msg_type], sizeof(GPS_Data));

			return true;
		}
	}

	return false;
}

bool is_in_dominica(float latitude, float longitude){
	return (latitude < DOMINICA_LAT_BOUNDARY);
}


//External variables defined in other C files (uart handler and queue for comms threads)


//Turn GPS off (through power FET) and starts buffering thread
void gps_sleep(void){
	HAL_GPIO_WritePin(GPS_NEN_GPIO_Port, GPS_NEN_Pin, GPIO_PIN_SET);
}

void gps_wake(void){
	//Enable power to GPS module and starts buffering thread
	HAL_GPIO_WritePin(GPS_NEN_GPIO_Port, GPS_NEN_Pin, GPIO_PIN_RESET);
}
