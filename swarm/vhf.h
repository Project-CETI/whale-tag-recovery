#ifndef VHF_H
#define VHF_H
#include "stdint.h"
// VHF HEADERS [START] --------------------------------------------------
void pinDescribe(void);

void initializeOutput(void);
void setOutput(uint8_t state);

void initializeDra818v(bool highPower);
void configureDra818v(float txFrequency, float rxFrequency, uint8_t volume, bool emphasis, bool hpf, bool lpf);
void setPttState(bool state);
void setVhfState(bool state);
void configureVHF(void);
// VHF HEADERS [END] ----------------------------------------------------
#endif
