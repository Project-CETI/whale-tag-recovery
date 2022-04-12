//
// Created by Louis Adamian on 3/23/22.
//
#ifndef CETI_TAG_TRACKING_APRS_H
#define CETI_TAG_TRACKING_APRS_H
#include <Arduino.h>
#include "R2rDac.hh"
#define _FLAG 0x7e
#define crc 0xffff
//sleep power level, squelch, ptt

class APRS {
public:
    R2rDac dac;
    uint8_t powerLevelPin, SquelchPin, pttPin;
    char callSign[8];

    APRS(R2rDac dac,  uint8_t powerLevelPin, uint8_t SquelchPin, uint8_t pttPin){
        this.dac = dac;
        this.powerLevelPin = powerLevelPin;
        this.SquelchPin = SquelchPin
        this.pttPin = pttPin;

    };

    void sendPacket(char type);
    bool configDra818v(SoftwareSerial serial, float txFrequency = 144.39, float rxFrequency = 144.39, bool emphasis = false, bool hpf = false, bool lpf = false);
private:
    void sendPayload(char type);
    void send_char_NZRI(char in_byte, enBitStuff);
    static inline void set_nada_2400();
    static inline void set_nada_1200();
    static void inline set_nada(bool nada);
    void send_flag(unsigned char flag_len);
    void sendCrc();
    void send
};
#endif //CETI_TAG_TRACKING_APRS_H
