#ifndef TAG_H
#define TAG_H
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
void writeGpsToTag(const tag_config_s * tag_cfg, char *lastGps, char *lastDt);
void detachTag(const tag_config_s * tag_cfg);
void reqTagState(const tag_config_s * tag_cfg);

// Init functions
void initTagComm(const tag_config_s * tag_cfg);

// TAG FUNCTIONS [END] --------------------------------------------------
#endif
