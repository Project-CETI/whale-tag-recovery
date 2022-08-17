#ifndef _RECOVERY_TAG_H_
#define _RECOVERY_TAG_H_

#include "constants.h"
#include "hardware/uart.h"

typedef struct tag_config_t {
    int txPin;
    int rxPin;
    int baudrate;
    uart_inst_t *uart;
    uint32_t interval;
    uint32_t ackWaitT_us;
    uint32_t ackWaitT_ms;
    int numTries;
} tag_config_s;

// TAG FUNCTIONS [START] ------------------------------------------------
// Request functions
void writeGpsToTag(const tag_config_s *tag_cfg, char *lastGps, char *lastDt);
void detachTag(const tag_config_s *tag_cfg);
void reqTagState(const tag_config_s *tag_cfg);

// Init functions
uint32_t initTagComm(const tag_config_s *tag_cfg);

// TAG FUNCTIONS [END] --------------------------------------------------
#endif  //_RECOVERY_TAG_H_
