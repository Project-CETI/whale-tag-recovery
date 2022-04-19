//
// Created by Louis Adamian on 3/23/22.
//
#ifndef CETI_TAG_TRACKING_APRS_H
#define CETI_TAG_TRACKING_APRS_H
#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "config.hh"
#define _FLAG 0x7e
//sleep power level, squelch, ptt

class APRS {
public:
    uint16_t crc = 0xffff;
    uint8_t powerLevelPin, SquelchPin, pttPin;
    char callSign[8];
    HardwareSerial *serial;
    bool nada = true;
    APRS(const char callSign[8]);
    void sendPacket(char type);
    static bool configDra818v(HardwareSerial &hardSerial, float txFrequency, float rxFrequency, bool emphasis, bool hpf, bool lpf);
#ifdef APRS_SoftwareSerial_enabled
    bool configDra818v(SoftwareSerial serial, float txFrequency, float rxFrequency, bool emphasis, bool hpf, bool lpf)
#endif
private:
    char bit_stuff = 0;
    void sendPayload(char type);
    void sendCharNrzi(char in_byte, bool enBitStuff, char bit_stuff);
    static inline void setNada2400();
    static inline void setNada1200();
    static inline void setNada(bool nada);
    void sendFlag(unsigned char flagLen);
    void sendCrc();
    void calcCrc(bool in_bit);
    static void writeDac(uint8_t);
    void sendStringLen(const char *inString, int len);
};
#endif //CETI_TAG_TRACKING_APRS_H
