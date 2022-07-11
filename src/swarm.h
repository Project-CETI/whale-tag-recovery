#ifndef SWARM_H
#define SWARM_H
#include "stdint.h"
#include "pico/stdlib.h"
// RX functions from SWARM
int parseModemOutput(void);
void readFromModem(void);

// TX functions to SWARM
uint8_t nmeaChecksum(const char *sz, size_t len);
void writeToModem(char *sz);
void serialCopyToModem(void);

// Unsolicited response callbacks
void storeGpsData(void);
void storeDTData(void);
void txSwarm(void);

// Init SWARM functions
void swarmInit(int txPin, int rxPin, int baudrate, uart_inst_t *uartNum, bool running, bool wAcks, bool interact, bool debug);
int checkSwarmBooted(void);
int checkInitAck(void);
void swarmBootSequence(bool running, bool wAcks);
void swarmResponseInit(bool debug);

// Queue management functions
void getQCount(void);

// Modem management functions
void getRxTestOutput(bool onQ);

// Parameter setting functions
void swarmSetInteractive(bool interact);

// Data retrieval functions
void getPos(float *dstPosBuf);
void getACS(uint16_t *dstACSBuf);
void getLastPDtBufs(char *dstGpsBuf, char *dstDtBuf);
#endif
