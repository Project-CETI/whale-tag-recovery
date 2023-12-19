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
#define BATTERY_MONITOR_ENABLED 0
#define RTC_ENABLED 0
#define UART_ENABLED 1
#define HEARTBEAT_ENABLED 0

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
}Configuration;



#define DEFAULT_CONFIGURATION (Configuration){\
	.critical_voltage = 6.0,\
	.vhf_power = VHF_POWER_HIGH,\
	.aprs_freq = 144.3900f,\
}

extern volatile Configuration g_config;

#endif /* INC_CONFIG_H_ */
