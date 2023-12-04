/*
 * VHF.h
 *
 *  Created on: Jun 10, 2023
 *      Author: Kaveet
 *
 *  This file outlines the UART driver for the VHF module (DRA818V).
 *  http://www.dorji.com/docs/data/DRA818V.pdf
 *
 *  It initializes, configures and manages the VHF module used in recovery mode.
 *
 *  The VHF module attached our input signal (DAC) to the carrier wave. The carrier wave has a configurable frequency.
 */

#ifndef INC_RECOVERY_INC_VHF_H_
#define INC_RECOVERY_INC_VHF_H_

#include "main.h"
#include <stdbool.h>

//Defines
extern UART_HandleTypeDef huart4;

#define VHF_UART huart4

//Useful Frequencies
#define TX_FREQ "144.3900"
#define RX_FREQ "144.3900"
#define DOMINICA_TX_FREQ "145.0500"
#define DOMINICA_RX_FREQ "145.0500"

#define VHF_VOLUME_LEVEL 4

//Lengths of messages used to configure the VHF module
#define DUMMY_TRANSMIT_LENGTH 6
#define DUMMY_RESPONSE_LENGTH 15

#define HANDSHAKE_TRANSMIT_LENGTH 15
#define HANDSHAKE_RESPONSE_LENGTH 15

#define SET_PARAMETERS_TRANSMIT_LENGTH 48
#define SET_PARAMETERS_RESPONSE_LENGTH 16

#define SET_VOLUME_TRANSMIT_LENGTH 20
#define SET_VOLUME_RESPONSE_LENGTH 17

#define SET_FILTER_TRANSMIT_LENGTH 20
#define SET_FILTER_RESPONSE_LENGTH 17

#define VHF_MAX_WAKE_TIME_MS 2000
#define VHF_TRANSITION_TIME_MS 20

//Expected responses to transmitted messages
#define VHF_HANDSHAKE_EXPECTED_RESPONSE "+DMOCONNECT:0\r\n"
#define VHF_SET_PARAMETERS_EXPECTED_RESPONSE "+DMOSETGROUP:0\r\n"
#define VHF_SET_VOLUME_EXPECTED_RESPONSE "+DMOSETVOLUME:0\r\n"
#define VHF_SET_FILTER_EXPECTED_RESPONSE "+DMOSETFILTER:0\r\n"

typedef enum vhf_power_level_e {
	VHF_POWER_LOW,
	VHF_POWER_HIGH,
}VHFPowerLevel;

typedef enum vhf_state_e {
	VHF_STATE_SLEEP,
	VHF_STATE_TX,
	VHF_STATE_RX,
}VHFState;

typedef struct vhf_configuration_t{
	float tx_freq_MHz;
	float rx_freq_MHz;
	uint8_t volume;
	bool emphasis;
	bool lpf;
	bool hpf;
}VHFConfig;

typedef struct vhf_handle_t {
	UART_HandleTypeDef *huart;
	struct {
		GPIO_TypeDef *port;
		uint16_t pin;
	}pd;
	struct {
		GPIO_TypeDef *port;
		uint16_t pin;
	}h_l;
	struct {
		GPIO_TypeDef *port;
		uint16_t pin;
	}ptt;
	VHFState state;
	VHFPowerLevel power_level;
	VHFConfig config;
}VHF_HandleTypdeDef;

/*Configure the VHF module over UART
 Parameters:
 	 -huart - UART handler to talk to the module
 	 -isHigh: whether or not to use high power (1W) or low power (0.5W). Passing true means high power.
 	 -emphasis: whether or not to use emphasis on rf signals. True = use emphasis.
 	 -lpf: Apply a low-pass-filter on the signals.                             */
//HAL_StatusTypeDef configure_dra818v(UART_HandleTypeDef huart, bool emphasis, bool lpf, bool hpf, char * tx_freq, char * rx_freq);
HAL_StatusTypeDef configure_dra818v(VHF_HandleTypdeDef *vhf);

//Toggles the power level on the module from high (1W) and low (0.5W).
void vhf_set_power_level(VHF_HandleTypdeDef *vhf, VHFPowerLevel power_level);


/* Set VHF Module TX and RX frequencies
 * Parameters:
 *   -freq_MHz: 134.0000 to 174.0000
 */
HAL_StatusTypeDef vhf_set_freq(VHF_HandleTypdeDef *vhf, float freq_MHz);

// Try putting VHF module into recieve state
HAL_StatusTypeDef vhf_rx(VHF_HandleTypdeDef *vhf);

// Try putting VHF module into transmit state
HAL_StatusTypeDef vhf_tx(VHF_HandleTypdeDef *vhf);

// Puts the VHF module to sleep
void vhf_sleep(VHF_HandleTypdeDef *vhf);

//Wakes up the VHF module
HAL_StatusTypeDef vhf_wake(VHF_HandleTypdeDef *vhf);

#endif /* INC_RECOVERY_INC_VHF_H_ */
