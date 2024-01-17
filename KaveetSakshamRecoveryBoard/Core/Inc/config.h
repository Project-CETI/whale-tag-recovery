/*
 * config.h
 *
 *  Created on: Sep 25, 2023
 *      Author: Michael Salino-Hugg (msalinohugg@seas.harvard.edu)
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

#include "Recovery Inc/VHF.h"

/* COMPILE-TIME CONFIGURATION */
#define USB_BOOTLOADER_ENABLED 1
#define BATTERY_MONITOR_ENABLED 1
#define RTC_ENABLED 0
#define UART_ENABLED 1
#define HEARTBEAT_ENABLED 0

#define IN_DOMINICA 1

#if IN_DOMINICA
#define TX_FREQ_MHZ 145.050
#else
#define TX_FREQ_MHZ 144.390
#endif

/* RUNTIME CONFIGURATION */

typedef struct global_position_t{
	float latitude;
	float longitude;
}GlobalPosition;

typedef struct geofence_region_t{
	GlobalPosition min;
	GlobalPosition max;
}GeofenceRegion;

typedef struct config_t{
	float 			critical_voltage;
	VHFPowerLevel 	vhf_power;
	uint32_t 		vhf_tx_interval;
	GeofenceRegion 	geofence_area;
	float			aprs_freq;
	char 			pi_hostname[16];
}Configuration;



#define DEFAULT_CONFIGURATION (Configuration){\
	.critical_voltage = 6.0,\
	.vhf_power = VHF_POWER_HIGH,\
	.aprs_freq = TX_FREQ_MHZ,\
	.pi_hostname = "",\
}

extern Configuration g_config;

#endif /* INC_CONFIG_H_ */
