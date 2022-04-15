//
// Created by Louis Adamian on 3/23/22.
//
#include "APRS.hh"
#include "conifig.hh"
// configures the DRA818V VHF module over UART returns true if config us successful and false if unsuccessful.
//
APRS::APRS(uint8_t powerLevelPin, uint8_t SquelchPin, uint8_t pttPin, const char callSign[8]) {
        this->powerLevelPin = powerLevelPin;
        this->SquelchPin = SquelchPin;
        this->pttPin = pttPin;
        for(uint8_t i =0; i< 8;i++){
            this->callSign[i] = callSign[i];
        }
    }
bool APRS::configDra818v(HardwareSerial &hardSerial, float txFrequency, float rxFrequency, bool emphasis, bool hpf, bool lpf) {
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

#ifdef r2rDac5
const uint16_t sin1200Values[32] = {
        19, 22, 24, 27, 29, 30, 31, 31, 31, 30, 29, 27, 24, 22, 19, 16, 12,
        9,  7,  4,  2,  1,  0,  0,  0,  1,  2,  4,  7,  9, 12, 15
};
const uint8_t sin2400Values[16] = {22, 27, 30, 31, 30, 27, 22, 16,  9,  4,  1,  0,  1,  4,  9, 15};
#endif

void APRS::setNada1200() {
    for(uint8_t sinNum = 0; sinNum < 2; sinNum++){
        for(uint8_t currSinVal = 0; currSinVal < sizeof(sin2400Values)/2; currSinVal++){
            setVoltageFast(sin2400Values[currSinVal]);
        }
    }
}

void APRS::setNada2400() {
    for(uint8_t currSinVal = 0; currSinVal < sizeof(sin1200Values)/2; currSinVal++){
        setVoltageFast(sin1200Values[currSinVal]);
    }
}

void APRS::sendPayload(char type) {
    /*
       APRS AX.25 Payloads

       TYPE : POSITION
       ........................................................
       |DATA TYPE |    LAT   |SYMB. OVL.|    LON   |SYMB. TBL.|
       --------------------------------------------------------
       |  1 byte  |  8 bytes |  1 byte  |  9 bytes |  1 byte  |
       --------------------------------------------------------

       DATA TYPE  : !
       LAT        : ddmm.ssN or ddmm.ssS
       LON        : dddmm.ssE or dddmm.ssW


       TYPE : STATUS
       ..................................
       |DATA TYPE |    STATUS TEXT      |
       ----------------------------------
       |  1 byte  |       N bytes       |
       ----------------------------------

       DATA TYPE  : >
       STATUS TEXT: Free form text


       TYPE : POSITION & STATUS
       ..............................................................................
       |DATA TYPE |    LAT   |SYMB. OVL.|    LON   |SYMB. TBL.|    STATUS TEXT      |
       ------------------------------------------------------------------------------
       |  1 byte  |  8 bytes |  1 byte  |  9 bytes |  1 byte  |       N bytes       |
       ------------------------------------------------------------------------------

       DATA TYPE  : !
       LAT        : ddmm.ssN or ddmm.ssS
       LON        : dddmm.ssE or dddmm.ssW
       STATUS TEXT: Free form text


       All of the data are sent in the form of ASCII Text, not shifted.

    */
//    if (type == _GPRMC)
//    {
//        sendCharNrzi('$', true);
//        send_string_len(rmc, strlen(rmc) - 1);
//    }
//    else if (type == _FIXPOS)
//    {
//        sendCharNrzi(_DT_POS, true);
//        send_string_len(lati, strlen(lati));
//        sendCharNrzi(sym_ovl, true);
//        send_string_len(lon, strlen(lon));
//        sendCharNrzi(sym_tab, true);
//    }
//    else if (type == _STATUS)
//    {
//        sendCharNrzi(_DT_STATUS, true);
//        send_string_len(mystatus, strlen(mystatus));
//    }
//    else if (type == _FIXPOS_STATUS)
//    {
//        sendCharNrzi(_DT_POS, true);
//        send_string_len(lati, strlen(lati));
//        sendCharNrzi(sym_ovl, true);
//        send_string_len(lon, strlen(lon));
//        sendCharNrzi(sym_tab, true);
//
//        send_string_len(comment, strlen(comment));
//    }
//    else
//    {
//        send_string_len(mystatus, strlen(mystatus));
//    }
}

void APRS::sendCharNrzi(char in_byte, bool enBitStuff) {
    bool bits;

    for (int i = 0; i < 8; i++)
    {
        bits = in_byte & 0x01;
        calcCrc(bits);

        if (bits)
        {
            setNada(nada);
            bit_stuff++;

            if ((enBitStuff) && (bit_stuff == 5))
            {
                nada ^= 1;
                setNada(nada);

                bit_stuff = 0;
            }
        }
        else
        {
            nada ^= 1;
            setNada(nada);

            bit_stuff = 0;
        }

        in_byte >>= 1;
    }
}

void APRS::sendStringLen(const char *inString, int len)
{
    for (int j = 0; j < len; j++)
        sendCharNrzi(inString[j], true);
}

void APRS::sendFlag(unsigned char flagLen) {
    for (int j = 0; j < flagLen; j++)
        sendCharNrzi(_FLAG, false);
}

void APRS::setNada(bool nada) {
    if (nada)
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

void APRS::sendPacket(char type) {

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
    sendCharNrzi(crc_lo, true);
    sendCharNrzi(crc_hi, true);
}