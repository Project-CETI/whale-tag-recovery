/*

*/
#include <math.h>
#include <stdio.h>

#define _1200   1
#define _2400   0

#define _FLAG       0x7e
#define _CTRL_ID    0x03
#define _PID        0xf0
#define _DT_EXP     ','
#define _DT_STATUS  '>'
#define _DT_POS     '!'

#define _GPRMC          1
#define _FIXPOS         2
#define _FIXPOS_STATUS  3
#define _STATUS         4
#define _BEACON         5

// LED Pin
const uint8_t ledPin = 29;

// VHF control pins
const uint8_t vhfPowerLevelPin = 15;
const uint8_t vhfPttPin = 16;
const uint8_t vhfSleepPin = 14;
const uint8_t vhfTxPin = 13;
const uint8_t vhfRxPin = 12;

// VHF control params
const uint16_t vhfTimeout = 500;
const uint16_t vhfEnableDelay = 1000;

// VHF Output pins
const uint8_t out0Pin = 18;
const uint8_t out1Pin = 19;
const uint8_t out2Pin = 20;
const uint8_t out3Pin = 21;
const uint8_t out4Pin = 22;
const uint8_t out5Pin = 23;
const uint8_t out6Pin = 24;
const uint8_t out7Pin = 25;

bool nada = _2400;

char mycall[8] = "KC1QXQ";
char myssid = 11;

char dest[8] = "APLIGA";
char dest_beacon[8] = "BEACON";

char digi[8] = "WIDE2";
char digissid = 2;

char comment[128] = "v0.9";
char mystatus[128] = "Status";

char lati[9] = "4221.78N";
char lon[10] = "07107.52W";

int coord_va9id;
const char sym_ovl = 'T';
const char sym_tab = 'a';

unsigned int tx_delay = 5000;
unsigned int str_len = 400;

char bit_stuff = 0;
unsigned short crc = 0xffff;

char rmc[100];
char rmc_stat;

void set_nada_1200(void);
void set_nada_2400(void);
void set_nada(bool nada);

void send_char_NRZI(unsigned char in_byte, bool enBitStuff);
void send_string_len(const char *in_string, int len);

void calc_crc(bool in_bit);
void send_crc(void);

void send_packet(char packet_type);
void send_flag(unsigned char flag_len);
void send_header(char mssinValuesg_type);
void send_payload(char type);

void print_debug(char type);

const uint16_t bitPeriod = 832;
const uint16_t delay1200 = 19;//23
const uint16_t delay2200 = 9;//11
const uint8_t numSinValues = 32;

const uint8_t sinValues[numSinValues] = {
        152,176,198,217,233,245,252,255,252,245,233,217,198,176,152,127,103,79,57,38,22,10,3,1,3,10,22,38,57,79,103,128
};

uint8_t currOutput = 0;

void initializeOutput() {
    pinMode(out0Pin, OUTPUT);
    pinMode(out1Pin, OUTPUT);
    pinMode(out2Pin, OUTPUT);
    pinMode(out3Pin, OUTPUT);
    pinMode(out4Pin, OUTPUT);
    pinMode(out5Pin, OUTPUT);
    pinMode(out6Pin, OUTPUT);
    pinMode(out7Pin, OUTPUT);

}

void setOutput(uint8_t state) {
    digitalWrite(out0Pin, state & 0b00000001);
    digitalWrite(out1Pin, state & 0b00000010);
    digitalWrite(out2Pin, state & 0b00000100);
    digitalWrite(out3Pin, state & 0b00001000);
    digitalWrite(out4Pin, state & 0b00010000);
    digitalWrite(out5Pin, state & 0b00100000);
    digitalWrite(out6Pin, state & 0b01000000);
    digitalWrite(out7Pin, state & 0b10000000);
}

void setNextSin() {
    currOutput++;
    if (currOutput == numSinValues)
        currOutput = 0;
    setOutput(sinValues[currOutput]);
}

void set_nada_1200(void)
{
    uint32_t endTime = micros() + bitPeriod;
    while (micros() < endTime) {
        setNextSin();
        delayMicroseconds(delay1200);
    }
}

void set_nada_2400(void)
{
    uint32_t endTime = micros() + bitPeriod;
    while (micros() < endTime) {
        setNextSin();
        delayMicroseconds(delay2200);
    }
}

void set_nada(bool nada)
{
    if (nada)
        set_nada_1200();
    else
        set_nada_2400();
}

/*
   This function will calculate CRC-16 CCITT for the FCS (Frame Check Sequence)
   as required for the HDLC frame validity check.

   Using 0x1021 as polynomial generator. The CRC registers are initialized with
   0xFFFF
*/
void calc_crc(bool in_bit)
{
    unsigned short xor_in;

    xor_in = crc ^ in_bit;
    crc >>= 1;

    if (xor_in & 0x01)
        crc ^= 0x8408;
}

void send_crc(void)
{
    unsigned char crc_lo = crc ^ 0xff;
    unsigned char crc_hi = (crc >> 8) ^ 0xff;

    send_char_NRZI(crc_lo, true);
    send_char_NRZI(crc_hi, true);
}

void send_header(char msg_type)
{
    char temp;

    /*
       APRS AX.25 Header
       ........................................................
       |   DEST   |  SOURCE  |   DIGI   | CTRL FLD |    PID   |
       --------------------------------------------------------
       |  7 bytes |  7 bytes |  7 bytes |   0x03   |   0xf0   |
       --------------------------------------------------------

       DEST   : 6 byte "callsign" + 1 byte ssid
       SOURCE : 6 byte your callsign + 1 byte ssid
       DIGI   : 6 byte "digi callsign" + 1 byte ssid

       ALL DEST, SOURCE, & DIGI are left shifted 1 bit, ASCII format.
       DIGI ssid is left shifted 1 bit + 1

       CTRL FLD is 0x03 and not shifted.
       PID is 0xf0 and not shifted.
    */

    /********* DEST ***********/
    if (msg_type == _BEACON)
    {
        temp = strlen(dest_beacon);
        for (int j = 0; j < temp; j++)
            send_char_NRZI(dest_beacon[j] << 1, true);
    }
    else
    {
        temp = strlen(dest);
        for (int j = 0; j < temp; j++)
            send_char_NRZI(dest[j] << 1, true);
    }
    if (temp < 6)
    {
        for (int j = 0; j < (6 - temp); j++)
            send_char_NRZI(' ' << 1, true);
    }
    send_char_NRZI('0' << 1, true);



    /********* SOURCE *********/
    temp = strlen(mycall);
    for (int j = 0; j < temp; j++)
        send_char_NRZI(mycall[j] << 1, true);
    if (temp < 6)
    {
        for (int j = 0; j < (6 - temp); j++)
            send_char_NRZI(' ' << 1, true);
    }
    send_char_NRZI((myssid + '0') << 1, true);


    /********* DIGI ***********/
    temp = strlen(digi);
    for (int j = 0; j < temp; j++)
        send_char_NRZI(digi[j] << 1, true);
    if (temp < 6)
    {
        for (int j = 0; j < (6 - temp); j++)
            send_char_NRZI(' ' << 1, true);
    }
    send_char_NRZI(((digissid + '0') << 1) + 1, true);

    /***** CTRL FLD & PID *****/
    send_char_NRZI(_CTRL_ID, true);
    send_char_NRZI(_PID, true);
}

void send_payload(char type)
{
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

/*
   This function will send one byte input and convert it
   into AFSK signal one bit at a time LSB first.

   The encode which used is NRZI (Non Return to Zero, Inverted)
   bit 1 : transmitted as no change in tone
   bit 0 : transmitted as change in tone
*/
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

void send_flag(unsigned char flag_len)
{
    for (int j = 0; j < flag_len; j++)
        send_char_NRZI(_FLAG, LOW);
}

/*
   In this preliminary test, a packet is consists of FLAG(s) and PAYLOAD(s).
   Standard APRS FLAG is 0x7e character sent over and over again as a packet
   delimiter. In this example, 100 flags is used the preamble and 3 flags as
   the postamble.
*/
void send_packet(char packet_type)
{
    print_debug(packet_type);

    setLed(true);
    setPttState(true);

    // TODO TEST IF THIS IMPROVES RECEPTION
    // Send initialize sequence for receiver
    send_char_NRZI(0x99, true);
    send_char_NRZI(0x99, true);
    send_char_NRZI(0x99, true);

    // delay(100);

    /*
       AX25 FRAME

       ........................................................
       |  FLAG(s) |  HEADER  | PAYLOAD  | FCS(CRC) |  FLAG(s) |
       --------------------------------------------------------
       |  N bytes | 22 bytes |  N bytes | 2 bytes  |  N bytes |
       --------------------------------------------------------

       FLAG(s)  : 0x7e
       HEADER   : see header
       PAYLOAD  : 1 byte data type + N byte info
       FCS      : 2 bytes calculated from HEADER + PAYLOAD
    */

    send_flag(150);
    crc = 0xffff;
    send_header(packet_type);
    send_payload(packet_type);
    send_crc();
    send_flag(3);

    setPttState(false);
    setLed(false);
}

void print_debug(char type)
{
    /*
       PROTOCOL DEBUG.

       Will outputs the transmitted data to the serial monitor
       in the form of TNC2 string format.

       MYCALL-N>APRS,DIGIn-N:<PAYLOAD STRING> <CR><LF>
    */
    Serial.begin(115200);

    /****** MYCALL ********/
    Serial.print(mycall);
    Serial.print('-');
    Serial.print(myssid, DEC);
    Serial.print('>');

    /******** DEST ********/
    if (type == _BEACON)
        Serial.print(dest_beacon);
    else
        Serial.print(dest);
    Serial.print(',');

    /******** DIGI ********/
    Serial.print(digi);
    Serial.print('-');
    Serial.print(digissid, DEC);
    Serial.print(':');

    /******* PAYLOAD ******/
    if (type == _GPRMC)
    {
        Serial.print('$');
        Serial.print(rmc);
    }
    else if (type == _FIXPOS)
    {
        Serial.print(_DT_POS);
        Serial.print(lati);
        Serial.print(sym_ovl);
        Serial.print(lon);
        Serial.print(sym_tab);
    }
    else if (type == _STATUS)
    {
        Serial.print(_DT_STATUS);
        Serial.print(mystatus);
    }
    else if (type == _FIXPOS_STATUS)
    {
        Serial.print(_DT_POS);
        Serial.print(lati);
        Serial.print(sym_ovl);
        Serial.print(lon);
        Serial.print(sym_tab);

        Serial.print(comment);
    }
    else
    {
        Serial.print(mystatus);
    }

    Serial.println(' ');

    Serial.flush();
    Serial.end();
}

// Initializes DRA818V pins
void initializeDra818v(bool highPower = true) {
    // Setup PTT pin
    pinMode(vhfPttPin, OUTPUT);
    digitalWrite(vhfPttPin, LOW);

    // Setup sleep pin
    pinMode(vhfSleepPin, OUTPUT);
    digitalWrite(vhfSleepPin, HIGH);

    // Setup power mode
    if (highPower) {
        pinMode(vhfPowerLevelPin, INPUT);
    }
    else {
        pinMode(vhfPowerLevelPin, OUTPUT);
        pinMode(vhfPowerLevelPin, LOW);
    }

    // Wait for module to boot
    delay(vhfEnableDelay);
}

// Configures DRA818V settings
bool configureDra818v(float txFrequency = 144.39, float rxFrequency = 144.39, bool emphasis = false, bool hpf = false, bool lpf = false){
    // Open serial connection
    Serial1.setTX(vhfRxPin);
    Serial1.setRX(vhfTxPin);
    Serial1.begin(9600);
    Serial1.setTimeout(vhfTimeout);

    // Handshake
    Serial1.println("AT+DMOCONNECT");
    if(!Serial1.find("+DMOCONNECT:0")) return false;
    delay(vhfEnableDelay);

    // Set frequencies and group
    Serial1.print("AT+DMOSETGROUP=0,");
    Serial1.print(txFrequency, 4);
    Serial1.print(',');
    Serial1.print(rxFrequency, 4);
    Serial1.println(",0000,0,0000");
    if(!Serial1.find("+DMOSETGROUP:0")) return false;
    delay(Serial1);

    // Set filter settings
    Serial1.print("AT+SETFILTER=");
    Serial1.print(emphasis);
    Serial1.print(',');
    Serial1.print(hpf);
    Serial1.print(',');
    Serial1.println(lpf);
    if(!Serial1.find("+DMOSETFILTER:0")) return false;
    delay(vhfEnableDelay);

    Serial1.end();
    return true;
}

// Sets the push to talk state
void setPttState(bool state) {
    digitalWrite(vhfPttPin, state);
}

// Sets the VHF module state
void setVhfState(bool state) {
    digitalWrite(vhfSleepPin, state);
}

void initLed(){
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, false);
}

void setLed(bool state){
    digitalWrite(ledPin, state);
}

void setup()
{
    initLed();
    setLed(true);

    Serial.begin(115200);
    delay(5000);

    Serial.println("Configuring DRA818V...");
    // Initialize DRA818V
    initializeDra818v();
    configureDra818v();
//    while(!configureDra818v()){
//        Serial.println("Failed to configure, trying again");
//        delay(3000);
//    }
    setPttState(false);
    setVhfState(true);
    Serial.println("DRA818V configured");

    Serial.println("Configuring DAC...");
    initializeOutput();
    Serial.println("DAC configured");

    Serial.println("Setup complete");
    setLed(false);
    delay(1000);
}

void loop()
{
    uint32_t startPacket = millis();
    send_packet(_FIXPOS);
    //send_packet(_BEACON);
    uint32_t packetDuration = millis() - startPacket;
    Serial.println("Packet sent in: " + String(packetDuration) + " ms");
    delay(tx_delay);
    //randomize(tx_delay, 5000, 9000);
}
