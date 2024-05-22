/*
 * RTC.h
 *
 *  Created on: Sep 15, 2023
 *      Author: kevinma
 */

#ifndef INC_SENSOR_INC_RTC_H_
#define INC_SENSOR_INC_RTC_H_

#include "Lib Inc/timing.h"
#include "tx_api.h"
#include <stdint.h>

#define RTC_SHUTDOWN_LIMIT_SEC 20
#define RTC_SHUTDOWN_LIMIT_HOUR 4

#define RTC_SLEEP_TIME_SEC 5
#define RTC_SLEEP_TIME_TICKS tx_s_to_ticks(RTC_SLEEP_TIME_SEC)

void RTC_thread_entry(ULONG thread_input);


#endif /* INC_SENSOR_INC_RTC_H_ */
