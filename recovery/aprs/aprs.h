#ifndef _RECOVERY_APRS_H_
#define _RECOVERY_APRS_H_

#include "stdbool.h"
#include "stdint.h"
#include "../../generated/generated.h"

// typedef struct aprs_config_t {
//     char callsign[8];
//     uint8_t ssid;
//     char symbol[3];
//     char dest[8];
//     char digi[16];
//     int dssid;
//     char comment[128];
//     bool compressed;
//     uint32_t interval;
//     uint16_t variance;
// } aprs_config_s;

// typedef struct aprs_data_t aprs_data_s;

// Low-level TX functions
static void setNextSin(void);
static void setNada1200(void);
static void setNada2400(void);
static void set_nada(bool nada);

// Mid-level TX functions
static void calcCrc(bool in_bit);
static void sendCrc(void);
static void sendCharNRZI(unsigned char in_byte, bool enBitStuff);
static void sendStrLen(const char *in_string, int len);
static void sendFlag(int flag_len);

// High-level TX functions
static void setPayload(float *latlon, uint16_t *acs);
static void sendHeader(const aprs_config_s *);
void sendPacket(const aprs_config_s *, float *latlon, uint16_t *acs);

// Debug TX functions
void printPacket(const aprs_config_s *);
void sendTestPackets(const aprs_config_s *);
void printRawPacket(char *buffer);

// Configuration functions
void initializeAPRS(void);
void configureAPRS_TX(const char *txFrequency);
void describeConfig(void);
void constant1200Wave(void);
#endif  //_RECOVERY_APRS_H_
