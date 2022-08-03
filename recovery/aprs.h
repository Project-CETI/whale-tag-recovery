#ifndef APRS_H
#define APRS_H
#include "stdint.h"
// Low-level TX functions
void setNextSin(void);
void setNada1200(void);
void setNada2400(void);
void set_nada(bool nada);

// Mid-level TX functions
void calcCrc(bool in_bit);
void sendCrc(void);
void sendCharNRZI(unsigned char in_byte, bool enBitStuff);
void sendStrLen(const char *in_string, int len);
void sendFlag(int flag_len);

// High-level TX functions
void setPayload(float *latlon, uint16_t *acs);
void sendHeader(void);
void sendPacket(float *latlon, uint16_t *acs);

// Debug TX functions
void printPacket(void);
void sendTestPackets(int style);

// Configuration functions
void initAPRS(char *mcall, int mssid, char *dst, char *dgi, int dgssid, char *cmt);
void configureAPRS_TX(float txFrequency);
void describeConfig(void);
#endif
