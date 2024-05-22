/*
 * Aprs.c
 *
 *  Created on: May 26, 2023
 *      Author: Kaveet
 */

#include "Recovery Inc/Aprs.h"
#include "Recovery Inc/VHF.h"
#include "Recovery Inc/GPS.h"
#include "Recovery Inc/AprsPacket.h"
#include "Recovery Inc/AprsTransmit.h"
#include "main.h"
#include "config.h"
#include <stdlib.h>


//Extern variables for HAL UART handlers and message queues
extern VHF_HandleTypdeDef vhf;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart2;
extern TX_QUEUE gps_tx_queue;

TX_MUTEX vhf_mutex;

static bool toggle_freq(bool is_gps_dominica, bool is_currently_dominica);

void aprs_thread_entry(ULONG aprs_thread_input){

    //buffer for packet data
    uint8_t packetBuffer[APRS_PACKET_MAX_LENGTH] = {0};

    //Create the GPS handler and configure it
    GPS_HandleTypeDef gps;
    initialize_gps(&huart3, &gps);

    //Initialize VHF module for transmission. Turn transmission off so we don't hog the frequency
    vhf_sleep(&vhf);
    tx_mutex_create(&vhf_mutex, "VHF mutex", 1);

    //Generate Aprs sine table
    aprs_transmit_init();

    //We arent in dominica by default
    bool is_in_dominica = true;

    //Main task loop
    while(1){

        //GPS data struct
        GPS_Data gps_data;


        //Attempt to get a GPS lock
        // bool is_locked = gps_read(&gps_data);
       bool is_locked = get_gps_lock(&gps, &gps_data);

        //The time we will eventually put this task to sleep for. We assign this assuming the GPS lock has failed (only sleep for a shorter, fixed period of time).
        //If we did get a GPS lock, the sleep_period will correct itself by the end of the task (be appropriately assigned after succesful APRS transmission)
        uint32_t sleep_period = GPS_SLEEP_LENGTH;

        for(int i = 0; i < 15; i++){
			HAL_GPIO_TogglePin(PWR_LED_NEN_GPIO_Port, PWR_LED_NEN_Pin);
			HAL_Delay(33);//flash at ~15 Hz for 1 second
		}
		HAL_GPIO_WritePin(PWR_LED_NEN_GPIO_Port, PWR_LED_NEN_Pin, GPIO_PIN_RESET);//ensure light is off after strobe
        //If we've locked onto a position, we can start creating an APRS packet.
        if (is_locked){
            uint8_t *packet_end;
            size_t packet_length;
            aprs_generate_location_packet(packetBuffer, &packet_end, gps_data.latitude, gps_data.longitude);

            //We first initialized the VHF module with our default frequencies. If we are in Dominica, re-initialize the VHF module to use the dominica frequencies.
            //
            //The function also handles switching back to the default frequency if we leave dominica
//            is_in_dominica = toggle_freq(gps_data.is_dominica, is_in_dominica);

            //Start transmission
            //increment aprs packet #
            tx_mutex_get(&vhf_mutex,TX_WAIT_FOREVER);
            if(vhf_tx(&vhf) == HAL_OK){
                packet_length = packet_end - packetBuffer;
				aprs_transmit_send_data(packetBuffer, packet_length);
            }
            //end transmission
            vhf_sleep(&vhf);
            tx_mutex_put(&vhf_mutex);
            //gps_invalidate();

            //Set the sleep period for a successful APRS transmission
            sleep_period = APRS_BASE_SLEEP_LENGTH - tx_ms_to_ticks(VHF_MAX_WAKE_TIME_MS);

            //Add a random component to it so that we dont transmit at the same interval each time (to prevent bad timing drowning out other transmissions)
            uint8_t random_num = rand() % tx_s_to_ticks(30);

            //Add a random amount of seconds to the sleep, from 0 to 29
            sleep_period += random_num;
        } else if (0 /*!(last tx < retransmit_timer)*/) {
            //retransmit last position with timerstamp
        }
        HAL_GPIO_WritePin(PWR_LED_NEN_GPIO_Port, PWR_LED_NEN_Pin, GPIO_PIN_SET);//ensure light is off after strobe


        //Go to sleep now
        tx_thread_sleep(sleep_period);
    }
}

void aprs_sleep(void){
    vhf_sleep(&vhf);
}

/*Reconfigures the VHF module to change the transmission frequency based on where the GPS is.
 *
 * is_gps_dominica: whether or not the current gps data is in dominica
 * is_currently_dominica: whether or not our VHF module is configured to the dominica frequency
 *
 * Returns: the current state of our configuration (whether or not VHF is configured for dominica
 */
static bool toggle_freq(bool is_gps_dominica, bool is_currently_dominica){

    //If the GPS is in dominica, but we are not configured for it, switch to dominica
    if (is_gps_dominica && !is_currently_dominica){
        //Re-initialize for dominica frequencies
        vhf_set_freq(&vhf, 145.0500f);

        //Now configured for dominica, return to indicate that
        return true;
    }
    //elseif, we are not in dominica, but we are configured for dominica. Switch back to the regular frequencies
    else if (!is_gps_dominica && is_currently_dominica){

        //Re-initialize for default frequencies
        vhf_set_freq(&vhf, 144.3900f);

        //No longer on dominica freq, return to indicate that
        return false;
    }

    //else: do nothing
    return is_currently_dominica;
}

void aprs_tx_message(const char* message, size_t message_len){
    uint8_t packetBuffer[APRS_PACKET_MAX_LENGTH] = {0};
    //buffer for packet data
    uint8_t *packet_end;
    aprs_generate_message_packet(packetBuffer, &packet_end, message, message_len);

    //Start transmission
    tx_mutex_get(&vhf_mutex,TX_WAIT_FOREVER);
    if(vhf_tx(&vhf) == HAL_OK){
        //Now, transmit the signal through the VHF module. Transmit a few times just for safety.
        aprs_transmit_send_data(packetBuffer, packet_end - packetBuffer);
    }
    //end transmission
    vhf_sleep(&vhf);
    tx_mutex_put(&vhf_mutex);
}
