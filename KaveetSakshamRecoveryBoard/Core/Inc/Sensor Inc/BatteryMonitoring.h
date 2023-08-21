/*
 * BatteryMonitoring.h
 *
 *  Created on: Aug 21, 2023
 *      Author: Kaveet
 *
 *
 * This thread will monitor the voltage of the battery through one of the ADC channel inputs. The nominal voltage on the battery is ~7.5V.
 *
 * This is too large for the STM, so we use a resistor divider to scale it from some value from 0-3.3V. See the schematic on EasyEDA for more details.
 *
 * Once we have the scaled value from 0-3.3V, we can use the resistor divide formula to scale it back up to 0 - 7.5V.
 * When the voltage dips below some value, we will enter recovery mode.
 */
#ifndef INC_SENSOR_INC_BATTERYMONITORING_H_
#define INC_SENSOR_INC_BATTERYMONITORING_H_

#include "Lib Inc/timing.h"
#include "tx_api.h"
#include <stdint.h>

//The R1 and R2 values (in ohms) for our resistor divider (R1 is the one connected to VSYS, and R2 is connected to ground).
#define VSYS_R1 (10000)
#define VSYS_R2 (7150)

#define BATT_MON_SLEEP_TIME_SEC 15

#define BATT_MON_SLEEP_TIME_TICKS tx_s_to_ticks(BATT_MON_SLEEP_TIME_SEC)

#define BATT_MON_ADC_TIMEOUT_TICKS tx_ms_to_ticks(500)

//Function to call to get an ADC conversion and return the raw digital value output
uint32_t battery_monitor_get_raw_adc_data();

//Function to call to get the true (fully scaled) battery voltage, form 0-7.5V.
float battery_monitor_get_true_voltage();

//Main thread entry for battery monitoring function
void battery_monitor_thread_entry(ULONG thread_input);

#endif /* INC_SENSOR_INC_BATTERYMONITORING_H_ */
