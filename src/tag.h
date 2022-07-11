#ifndef TAG_H
#define TAG_H
#include "hardware/uart.h"
// TAG FUNCTIONS [START] ------------------------------------------------
// Request functions
void writeGpsToTag(char *lastGps, char *lastDt);
void detachTag(void);
void reqTagState(void);

// Init functions
void initTagComm(int txPin, int rxPin, int baudrate, uart_inst_t *uartNum);

// TAG FUNCTIONS [END] --------------------------------------------------
#endif
