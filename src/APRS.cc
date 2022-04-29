//
// Created by Louis Adamian on 3/23/22.
//
#include "APRS.hh"
#include "config.h"
#include "SparkFun_Swarm_Satellite_Arduino_Library.h"
#include <cmath>
#define _DT_POS '!'
#define sym_ovl  'T'
#define sym_tab 'a'
#define delay2400 9
#define numSinValues 32
#define delay1200 19
#define bitPeriod 832
#include "Arduino.h"
#define aprsFrequencyMhz 144.39
// configures the DRA818V VHF module over UART returns true if config us successful and false if unsuccessful.
//
APRS::APRS(const char callSign[8], uint8_t ssid) {
        for(uint8_t i =0; i< 8; i++){
            this->callSign[i] = callSign[i];
        }
        this->ssid = ssid;
    }


bool APRS::configDra818v(HardwareSerial &hardSerial, bool emphasis, bool hpf, bool lpf, float txFrequency=aprsFrequencyMhz, float rxFrequency=aprsFrequencyMhz) {
    // Open serial connection
    HardwareSerial *serial;
    serial = &hardSerial;
    serial->begin(9600);
    serial->setTimeout(dra818vTimeout);
    // Handshake
    serial->println("AT+DMOCONNECT");
    if(!serial->find("+DMOcvf          ")) return false;
    delay(dra818vEnableDelay);
    // Set frequencies and group
    serial->print("AT+DMOSETGROUP=0,");
    serial->print(txFrequency, 4);
    serial->print(',');
    serial->print(rxFrequency, 4);
    serial->println(",0000,0,0000");
    if(!serial->find("+DMOSETGROUP:0")) return false;
    delay(dra818vEnableDelay);
    // Set filter settings
    serial->print("AT+SETFILTER=");
    serial->print(emphasis);
    serial->print(',');
    serial->print(hpf);
    serial->print(',');
    serial->println(lpf);
    if(!serial->find("+DMOSETFILTER:0")) return false;
    delay(dra818vEnableDelay);
    serial->end();
    return true;
}

bool APRS::configDra818v(SerialPIO serial, bool emphasis, bool hpf, bool lpf, float txFrequency=aprsFrequencyMhz, float rxFrequency=aprsFrequencyMhz) {
    // Open serial connection
    serial.begin(9600);
    serial.setTimeout(dra818vTimeout);
    // Handshake
    serial.println("AT+DMOCONNECT");
    if(!serial.find("+DMOcvf          ")) return false;
    delay(dra818vEnableDelay);
    // Set frequencies and group
    serial.print("AT+DMOSETGROUP=0,");
    serial.print(txFrequency, 4);
    serial.print(',');
    serial.print(rxFrequency, 4);
    serial.println(",0000,0,0000");
    if(!serial->find("+DMOSETGROUP:0")) return false;
    delay(dra818vEnableDelay);
    // Set filter settings
    serial.print("AT+SETFILTER=");
    serial.print(emphasis);
    serial.print(',');
    serial.print(hpf);
    serial.print(',');
    serial.println(lpf);
    if(!serial.find("+DMOSETFILTER:0")) return false;
    delay(dra818vEnableDelay);
    serial.end();
    return true;
}

void APRS::setOutput(uint8_t state) {
    digitalWrite(dacPin0, state & 0b00000001);
    digitalWrite(dacPin1, state & 0b00000010);
    digitalWrite(dacPin2, state & 0b00000100);
    digitalWrite(dacPin3, state & 0b00001000);
    digitalWrite(dacPin4, state & 0b00010000);
    digitalWrite(dacPin5, state & 0b00100000);
    digitalWrite(dacPin6, state & 0b01000000);
    digitalWrite(dacPin7, state & 0b10000000);
}
void APRS::setNextSin() {
    APRS::currOutput++;
    if (currOutput == numSinValues)
        currOutput = 0;
    setOutput(APRS::sinValues[currOutput]);
}

void APRS::setNada1200() {
    uint32_t endTime = micros() + bitPeriod;
    while (micros() < endTime) {
        APRS::setNextSin();
        delayMicroseconds(delay1200);
    }
}

void APRS::setNada2400() {
    uint32_t endTime = micros() + bitPeriod;
    while (micros() < endTime) {
        APRS::setNextSin();
        delayMicroseconds(delay2400);
    }
}


void APRS::sendPayload(Swarm_M138_GeospatialData_t *locationInfo, char* status){
    float latFloat = locationInfo->lat;
    bool south = false;
    if (latFloat <0){
        south = true;
        latFloat = fabs(latFloat);
    }
    float latInt, latDec;
    latDec = modf(latFloat, &latInt);// split the integer and decimal component fom the latitude
    latDec = latDec *60; // convert decimal degrees to minutes
    char lat[9];
    if (south){
        sprintf(lat, "%02f.%06fS", latInt, latDec);
    } else{
        sprintf(lat, "%02f.%06fN", latInt, latDec);
    }
    float lonFloat = locationInfo->lon;
    bool west = false;
    if(lonFloat<0){
        west = true;
        lonFloat = fabs(lonFloat);
    }
    float lonInt, lonDec;
    lonDec = modf(lonFloat,&lonInt);
    lonDec = lonDec*60;
    char lon[10];
    if(west){
        sprintf(lon, "%03f%6fW", lonInt, lonDec);
    } else{
        sprintf(lon, "%03f%6fE", lonInt, lonDec);
    }
    double speed = locationInfo->speed /1.852; //convert speed from km/h to knots
    float course = locationInfo->course;
    if(course ==0){ //APRS wants course in 1-360 and swarm provides it as 0-359
        course = 360;
    }
    char cogSpeed[7];
    sprintf(cogSpeed, "%03f/%03f", course, speed);
    APRS::sendCharNrzi(_DT_POS, true);
    APRS::sendStringLen(lat, strlen(lat));
    APRS::sendCharNrzi(sym_ovl, true);
    APRS::sendStringLen(lon, strlen(lon));
    APRS::sendCharNrzi(sym_tab, true);
    APRS::sendStringLen(status, strlen(status));
}


void APRS::sendCharNrzi(char in_byte, bool enBitStuff) {
    bool bits;

    for (int i = 0; i < 8; i++)
    {
        bits = in_byte & 0x01;
        APRS::calcCrc(bits);

        if (bits)
        {
            APRS::setNada(nada);
            bit_stuff++;

            if ((enBitStuff) && (bit_stuff == 5))
            {
                nada ^= 1;
                APRS::setNada(nada);

                bit_stuff = 0;
            }
        }
        else
        {
            nada ^= 1;
            APRS::setNada(nada);

            bit_stuff = 0;
        }

        in_byte >>= 1;
    }
}

void APRS::sendStringLen(const char *inString, uint16_t len)
{
    for (uint16_t j = 0; j < len; j++)
        APRS::sendCharNrzi(inString[j], true);
}

void APRS::sendFlag(unsigned char flagLen) {
    for (int j = 0; j < flagLen; j++)
        APRS::sendCharNrzi(_FLAG, false);
}

void APRS::setNada(bool currNada) {
    if (currNada)
        setNada1200();
    else
        setNada2400();
}

void APRS::calcCrc(bool in_bit)
{
    unsigned short xor_in;

    xor_in = crc ^ in_bit;
    crc >>= 1;

    if (xor_in & 0x01)
        crc ^= 0x8408;
}

void APRS::writeDac(uint8_t value) {
#ifdef r2rDac5
    digitalWrite(dacPin0, value & 0b00001);
    digitalWrite(dacPin1, value & 0b00010);
    digitalWrite(dacPin2, value & 0b00100);
    digitalWrite(dacPin3, value & 0b01000);
    digitalWrite(dacPin4, value & 0b10000);
#endif //r2rDac5
}

void APRS::sendCrc() {
    unsigned char crc_lo = crc ^ 0xff;
    unsigned char crc_hi = (crc >> 8) ^ 0xff;
    APRS::sendCharNrzi(crc_lo, true);
    APRS::sendCharNrzi(crc_hi, true);
}