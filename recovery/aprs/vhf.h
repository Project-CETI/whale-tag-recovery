#ifndef _RECOVERY_APRS_VHF_H_
#define _RECOVERY_APRS_HF_H_

#include "stdbool.h"
#include "stdint.h"



void pinDescribe(void);

static void initializeOutput(void);
void setOutput(uint8_t state);

// Fish tracker functions
void prepFishTx(const char *txFreq);
bool vhf_pulse_callback(void);

static void initializeDra818v(bool highPower);
void configureDra818v(const char *txFrequency, const char *rxFrequency,
                      uint8_t volume, bool emphasis, bool hpf, bool lpf);
void setPttState(bool state);
static void setVhfState(bool state);
void wakeVHF();
void sleepVHF();
void initializeVHF(void);

#endif  // _RECOVERY_VHF_H_
