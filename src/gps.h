#ifndef SWARM_H
#define SWARM_H
#include "stdint.h"
#include "pico/stdlib.h"
// RX functions from SWARM
// int parseGpsOutput(void);
void readFromGps(void);
void echoGpsOutput(void);

// Unsolicited response callbacks
void storeGpsData(void);
void storeDTData(void);

// Init SWARM functions
void gpsInit(int txPin, int rxPin, int baudrate, uart_inst_t *uartNum, bool interact);

// Data retrieval functions
void getPos(float *dstPosBuf);
void getACS(uint16_t *dstACSBuf);
void getLastPDtBufs(char *dstGpsBuf, char *dstDtBuf);
#endif
