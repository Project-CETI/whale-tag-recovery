#ifndef _RECOVERY_APRS_SLEEP_H_
#define _RECOVERY_APRS_SLEEP_H_

#include "../gps/ublox-config.h"
#include "../gps/gps.h"
#include <stdint.h>
#include <stdio.h>
#include "../setup_utils.h"

void set_bin_desc();
void recover_from_sleep();
void rtc_sleep(const gps_config_s gps_config, uint32_t sleep_time);

#endif