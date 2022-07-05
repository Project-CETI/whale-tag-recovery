#include "stdint.h"
// VHF HEADERS [START] --------------------------------------------------
void pinDescribe(void);

void initializeOutput(void);
void setOutput(uint8_t state);

void initializeDra818v(bool highPower);
bool configureDra818v(float txFrequency, float rxFrequency, bool emphasis, bool hpf, bool lpf);

void setPttState(bool state);
void setVhfState(bool state);
void configureVHF(void);
// VHF HEADERS [END] ----------------------------------------------------


// APRS HEADERS [START] -------------------------------------------------
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
void sendHeader(char *mycall, int myssid, char *dest, char *digi, int digissid);
void sendPacket(float *latlon, uint16_t *acs, char *mycall, int myssid, char *dest, char *digi, int digissid, char *comment);

// Debug TX functions
void printPacket(char *mycall, int myssid, char *dest, char *digi, int digissid, char *comment);
void sendTestPackets(char *mycall, int myssid, char *dest, char *digi, int digissid, char *comment, int style);
// APRS HEADERS [END] ---------------------------------------------------
