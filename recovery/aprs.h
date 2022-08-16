#ifndef _RECOVERY_APRS_H_
#define _RECOVERY_APRS_H_

#include "stdbool.h"
#include "stdint.h"

typedef struct aprs_config_t {
  char callsign[8];
  int ssid;
  char dest[8];
  char digi[8];
  int dssid;
  char comment[128];
  uint32_t interval;
  bool debug;
  int style;
} aprs_config_s;

typedef struct aprs_data_t aprs_data_s;

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
void sendHeader(const aprs_config_s *);
void sendPacket(const aprs_config_s *, float *latlon, uint16_t *acs);

// Debug TX functions
void printPacket(const aprs_config_s *);
void sendTestPackets(const aprs_config_s *);

// Configuration functions
void initAPRS(void);
void configureAPRS_TX(float txFrequency);
void describeConfig(void);
#endif //_RECOVERY_APRS_H_
