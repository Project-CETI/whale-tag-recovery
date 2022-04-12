//
// Created by Louis Adamian on 3/23/22.
//
#include "APRS.hh"

// configures the DRA818V VHF module over UART returns true if config us successful and false if unsuccessful.
//

bool APRS::configDra818v(int serial, float txFrequency, float rxFrequency, bool emphasis, bool hpf, bool lpf) {
    // Open serial connection
    serial.setTX(8);
    serial.setRX(9);
    serial.begin(9600);
    serial.setTimeout(vhfTimeout);
    // Handshake
    serial.println("AT+DMOCONNECT");
    if(!serial.find("+DMOcvf          ")) return false;
    delay(vhfEnableDelay);
    // Set frequencies and group
    serial.print("AT+DMOSETGROUP=0,");
    serial.print(txFrequency, 4);
    serial.print(',');
    serial.print(rxFrequency, 4);
    serial.println(",0000,0,0000");
    if(!serial.find("+DMOSETGROUP:0")) return false;
    delay(vhfEnableDelay);
    // Set filter settings
    serial.print("AT+SETFILTER=");
    serial.print(emphasis);
    serial.print(',');
    serial.print(hpf);
    serial.print(',');
    serial.println(lpf);
    if(!serial.find("+DMOSETFILTER:0")) return false;
    delay(vhfEnableDelay);
    serial.end();
    return true;
}

void APRS::set_nada_1200() {
    for(uint8_t sinNum = 0; sinNum < 2; sinNum++){
        for(uint8_t currSinVal = 0; currSinVal < sizeof(sin2400Values)/2; currSinVal++){
            setVoltageFast(sin2400Values[currSinVal]);
        }
    }
}

void APRS::set_nada_2400() {
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
    if (type == _GPRMC)
    {
        send_char_NRZI('$', true);
        send_string_len(rmc, strlen(rmc) - 1);
    }
    else if (type == _FIXPOS)
    {
        send_char_NRZI(_DT_POS, true);
        send_string_len(lati, strlen(lati));
        send_char_NRZI(sym_ovl, true);
        send_string_len(lon, strlen(lon));
        send_char_NRZI(sym_tab, true);
    }
    else if (type == _STATUS)
    {
        send_char_NRZI(_DT_STATUS, true);
        send_string_len(mystatus, strlen(mystatus));
    }
    else if (type == _FIXPOS_STATUS)
    {
        send_char_NRZI(_DT_POS, true);
        send_string_len(lati, strlen(lati));
        send_char_NRZI(sym_ovl, true);
        send_string_len(lon, strlen(lon));
        send_char_NRZI(sym_tab, true);

        send_string_len(comment, strlen(comment));
    }
    else
    {
        send_string_len(mystatus, strlen(mystatus));
    }
}

void send_char_NRZI(unsigned char in_byte, bool enBitStuff)
{
    bool bits;

    for (int i = 0; i < 8; i++)
    {
        bits = in_byte & 0x01;

        calc_crc(bits);

        if (bits)
        {
            set_nada(nada);
            bit_stuff++;

            if ((enBitStuff) && (bit_stuff == 5))
            {
                nada ^= 1;
                set_nada(nada);

                bit_stuff = 0;
            }
        }
        else
        {
            nada ^= 1;
            set_nada(nada);

            bit_stuff = 0;
        }

        in_byte >>= 1;
    }
}

void send_string_len(const char *in_string, int len)
{
    for (int j = 0; j < len; j++)
        send_char_NRZI(in_string[j], true);
}

void APRS::send_flag(unsigned char flag_len) {
    for (int j = 0; j < flag_len; j++)
        send_char_NRZI(APRS::_FLAG, LOW);
}

void APRS::set_nada(bool nada) {
    if (nada)
        set_nada_1200();
    else
        set_nada_2400();
}

void APRS::calc_crc(bool in_bit)
{
    unsigned short xor_in;

    xor_in = crc ^ in_bit;
    crc >>= 1;

    if (xor_in & 0x01)
        crc ^= 0x8408;
}
