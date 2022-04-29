//
// Created by Louis Adamian on 3/23/22.
//
#ifndef CETI_TAG_TRACKING_APRS_H
#define CETI_TAG_TRACKING_APRS_H
#include "Arduino.h"
#include "config.h"
#include "SparkFun_Swarm_Satellite_Arduino_Library.h"
#define numSinValues 32
#define _FLAG 0x7e
//sleep power level, squelch, ptt

class APRS {
public:
    APRS(const char callSign[8], uint8_t ssid);
    APRS(const char callSign[8], uint8_t ssid, const char dest[8]);
    APRS(const char callSign[8], uint8_t ssid, const char dest[8], const char digi[8], const uint8_t digi_ssid);
    void sendPacket(Swarm_M138_GeospatialData_t *locationInfo, char* status);
   void sendPayload(Swarm_M138_GeospatialData_t *locationInfo, char* status);
#ifdef APRS_SoftwareSerial_enabled
    static bool configDra818v(SoftwareSerial serial, float txFrequency, float rxFrequency, bool emphasis, bool hpf, bool lpf)
#endif
    static bool configDra818v(HardwareSerial &hardSerial, bool emphasis, bool hpf, bool lpf, float txFrequency, float rxFrequency);
    static bool configDra818v(SerialPIO serial, bool emphasis, bool hpf, bool lpf, float txFrequency, float rxFrequency);
private:
    char callSign[8];
    uint8_t ssid;
    char bitStuff = 0;
    uint16_t crc = 0xffff;
    uint8_t currOutput = 0;
    bool nada = true;

#ifdef r2rDac5
    static constexpr uint8_t sinValues[numSinValues] = {
        19, 22, 24, 27, 29, 30, 31, 31, 31, 30, 29, 27, 24, 22, 19, 16, 12,
        9,  7,  4,  2,  1,  0,  0,  0,  1,  2,  4,  7,  9, 12, 15
};
#else
    static constexpr uint8_t sinValues[numSinValues] = {
            152,176,198,217,233,245,252,255,252,245,233,217,198,176,152,127,103,79,57,38,22,10,3,1,3,10,22,38,57,79,103,128
    };
#endif
    char bit_stuff = 0;
    void sendCharNrzi(char in_byte, bool enBitStuff);
    void setNextSin();
    inline void setNada2400();
    inline void setNada1200();
    inline void setNada(bool nada);
    void sendFlag(unsigned char flagLen);
    void sendCrc();
    void calcCrc(bool in_bit);
    static void writeDac(uint8_t);
    void sendStringLen(const char *inString, uint16_t len);
    static void setOutput(uint8_t state);
};
#endif //CETI_TAG_TRACKING_APRS_H
