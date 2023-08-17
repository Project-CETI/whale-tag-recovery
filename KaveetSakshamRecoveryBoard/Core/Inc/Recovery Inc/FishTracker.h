/*
 * FishTracker.h
 *
 *  Created on: Aug 16, 2023
 *      Author: Kaveet
 *
 * An experimental mode that turns makes the Recovery board (DRA + Input Wave) emulate a fishtracker.
 *
 * It was developed to be highly configurable and will likely not be used on a deployed tag.
 *
 * It was mostly designed for experiments when we want to simulate multiple tags (Initially for Ninad's experiments).
 */

#ifndef INC_RECOVERY_INC_FISHTRACKER_H_
#define INC_RECOVERY_INC_FISHTRACKER_H_

#include "tx_api.h"
#include "Lib Inc/timing.h"
#include <stdbool.h>

#define USE_FISHTRACKER false

#define FISHTRACKER_CARRIER_FREQ_MHZ "150.7750"

#define FISHTRACKER_INPUT_FREQ_HZ 1000

#define FISHTRACKER_ON_TIME_MS 80

#define FISHTRACKER_OFF_TIME_MS 1100

#define FISHTRACKER_ON_TIME_TICKS (tx_ms_to_ticks(FISHTRACKER_ON_TIME_MS))

#define FISHTRACKER_OFF_TIME_TICKS (tx_ms_to_ticks(FISHTRACKER_OFF_TIME_MS))

#define FISHTRACKER_NUM_DAC_SAMPLES 100

//IF WE CHANGE THE TIMER2 PARAMETERS, REDEFINE THESE VALUES
#define TIM2_PRESCALER 16
#define TIM2_BASE_FREQ 160000000

#define TIM2_SCALED_FREQ (TIM2_BASE_FREQ / TIM2_PRESCALER)

void fishtracker_thread_entry(ULONG thread_input);

#endif /* INC_RECOVERY_INC_FISHTRACKER_H_ */
