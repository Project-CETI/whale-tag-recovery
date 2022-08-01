#ifndef SWARM_H
#define SWARM_H
#include "stdint.h"
#include "pico/stdlib.h"
// RX functions from NEO-M8N
void parseGpsOutput(char *line);
void readFromGps(void);
void echoGpsOutput(void);

// Init NEO-M8N functions
void gpsInit(int txPin, int rxPin, int baudrate, uart_inst_t *uartNum, bool interact);

// Data retrieval functions
void getPos(float *dstPosBuf);
void getACS(uint16_t *dstACSBuf);
void getLastPDtBufs(char *dstGpsBuf, char *dstDtBuf);
#endif
