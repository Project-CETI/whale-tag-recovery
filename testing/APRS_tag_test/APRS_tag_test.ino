
/*
    Copyright (C) 2018 - Handiko Gesang - www.github.com/handiko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <MCP_DAC.h>
#include <Wire.h>
#include <math.h>
#include <stdio.h>
#define _1200 1
#define _2400 0
#define _FLAG 0x7e
#define _CTRL_ID 0x03
#define _PID 0xf0
#define _DT_EXP ','
#define _DT_STATUS '>'
#define _DT_POS '!'

#define _GPRMC 1
#define _FIXPOS 2
#define _FIXPOS_STATUS 3
#define _STATUS 4
#define _BEACON 5

MCP4801 dac;

// VHF control pins
const uint8_t vhfPowerLevelPin = 11;
const uint8_t vhfPttPin = 10;
const uint8_t vhfSleepPin = 12;

// VHF control params
const uint16_t vhfTimeout = 500;
const uint16_t vhfEnableDelay = 1000;

bool nada = _2400;
char mycall[8] = "KC1QKW";
char myssid = 11;

char dest[8] = "APLIGA";
char dest_beacon[8] = "BEACON";

char digi[8] = "WIDE2";
char digissid = 2;

char comment[128] = "Comment";
char mystatus[128] = "Status";

char lati[9] = "4221.75N";
char lon[10] = "07107.52W";
int coord_va9id;
const char sym_ovl = 'T';
const char sym_tab = 'a';

unsigned int tx_delay = 10000;
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
void send_header(char msg_type);
void send_payload(char type);

char rx_gprmc(void);
char parse_gprmc(void);

void set_io(void);
void print_code_version(void);
void print_debug(char type);

const uint16_t sin2400Values[16] = {2831, 3495, 3939, 4095, 3939, 3495,
                                    2831, 2048, 1264, 600,  156,  1,
                                    156,  600,  1264, 2047};

const uint16_t sin1200Values[32] = {
    2447, 2831, 3185, 3495, 3750, 3939, 4055, 4095, 4055, 3939, 3750,
    3495, 3185, 2831, 2447, 2048, 1648, 1264, 910,  600,  345,  156,
    40,   1,    40,   156,  345,  600,  910,  1264, 1648, 2047};

uint8_t msg[3];

void setSave(bool save) {
  if (save) {
    msg[0] = 0x60;
  } else {
    msg[0] = 0x40;
  }
}

void set_nada_1200(void) {
  for (uint8_t currSinVal = 0; currSinVal < sizeof(sin1200Values) / 2;
       currSinVal++) {
    dac.fastWriteA(sin1200Values[currSinVal]);
  }
}

void set_nada_2400(void) {
  for (uint8_t sinNum = 0; sinNum < 2; sinNum++) {
    for (uint8_t currSinVal = 0; currSinVal < sizeof(sin2400Values) / 2;
         currSinVal++) {
      dac.fastWriteA(sin2400Values[currSinVal]);
    }
  }
}

void set_nada(bool nada) {
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
void calc_crc(bool in_bit) {
  unsigned short xor_in;

  xor_in = crc ^ in_bit;
  crc >>= 1;

  if (xor_in & 0x01) crc ^= 0x8408;
}

void send_crc(void) {
  unsigned char crc_lo = crc ^ 0xff;
  unsigned char crc_hi = (crc >> 8) ^ 0xff;

  send_char_NRZI(crc_lo, true);
  send_char_NRZI(crc_hi, true);
}

void send_header(char msg_type) {
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
  if (msg_type == _BEACON) {
    temp = strlen(dest_beacon);
    for (int j = 0; j < temp; j++) send_char_NRZI(dest_beacon[j] << 1, true);
  } else {
    temp = strlen(dest);
    for (int j = 0; j < temp; j++) send_char_NRZI(dest[j] << 1, true);
  }
  if (temp < 6) {
    for (int j = 0; j < (6 - temp); j++) send_char_NRZI(' ' << 1, true);
  }
  send_char_NRZI('0' << 1, true);

  /********* SOURCE *********/
  temp = strlen(mycall);
  for (int j = 0; j < temp; j++) send_char_NRZI(mycall[j] << 1, true);
  if (temp < 6) {
    for (int j = 0; j < (6 - temp); j++) send_char_NRZI(' ' << 1, true);
  }
  send_char_NRZI((myssid + '0') << 1, true);

  /********* DIGI ***********/
  temp = strlen(digi);
  for (int j = 0; j < temp; j++) send_char_NRZI(digi[j] << 1, true);
  if (temp < 6) {
    for (int j = 0; j < (6 - temp); j++) send_char_NRZI(' ' << 1, true);
  }
  send_char_NRZI(((digissid + '0') << 1) + 1, true);

  /***** CTRL FLD & PID *****/
  send_char_NRZI(_CTRL_ID, true);
  send_char_NRZI(_PID, true);
}

void send_payload(char type) {
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
     |DATA TYPE |    LAT   |SYMB. OVL.|    LON   |SYMB. TBL.|    STATUS TEXT |
     ------------------------------------------------------------------------------
     |  1 byte  |  8 bytes |  1 byte  |  9 bytes |  1 byte  |       N bytes |
     ------------------------------------------------------------------------------

     DATA TYPE  : !
     LAT        : ddmm.ssN or ddmm.ssS
     LON        : dddmm.ssE or dddmm.ssW
     STATUS TEXT: Free form text


     All of the data are sent in the form of ASCII Text, not shifted.

  */
  if (type == _GPRMC) {
    send_char_NRZI('$', true);
    send_string_len(rmc, strlen(rmc) - 1);
  } else if (type == _FIXPOS) {
    send_char_NRZI(_DT_POS, true);
    send_string_len(lati, strlen(lati));
    send_char_NRZI(sym_ovl, true);
    send_string_len(lon, strlen(lon));
    send_char_NRZI(sym_tab, true);
  } else if (type == _STATUS) {
    send_char_NRZI(_DT_STATUS, true);
    send_string_len(mystatus, strlen(mystatus));
  } else if (type == _FIXPOS_STATUS) {
    send_char_NRZI(_DT_POS, true);
    send_string_len(lati, strlen(lati));
    send_char_NRZI(sym_ovl, true);
    send_string_len(lon, strlen(lon));
    send_char_NRZI(sym_tab, true);

    send_string_len(comment, strlen(comment));
  } else {
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
void send_char_NRZI(unsigned char in_byte, bool enBitStuff) {
  bool bits;

  for (int i = 0; i < 8; i++) {
    bits = in_byte & 0x01;

    calc_crc(bits);

    if (bits) {
      set_nada(nada);
      bit_stuff++;

      if ((enBitStuff) && (bit_stuff == 5)) {
        nada ^= 1;
        set_nada(nada);

        bit_stuff = 0;
      }
    } else {
      nada ^= 1;
      set_nada(nada);

      bit_stuff = 0;
    }

    in_byte >>= 1;
  }
}

void send_string_len(const char *in_string, int len) {
  for (int j = 0; j < len; j++) send_char_NRZI(in_string[j], true);
}

void send_flag(unsigned char flag_len) {
  for (int j = 0; j < flag_len; j++) send_char_NRZI(_FLAG, LOW);
}

/*
   In this preliminary test, a packet is consists of FLAG(s) and PAYLOAD(s).
   Standard APRS FLAG is 0x7e character sent over and over again as a packet
   delimiter. In this example, 100 flags is used the preamble and 3 flags as
   the postamble.
*/
void send_packet(char packet_type) {
  print_debug(packet_type);

  digitalWrite(LED_BUILTIN, HIGH);
  setPttState(true);

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
  digitalWrite(LED_BUILTIN, LOW);
}

void print_debug(char type) {
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
  if (type == _GPRMC) {
    Serial.print('$');
    Serial.print(rmc);
  } else if (type == _FIXPOS) {
    Serial.print(_DT_POS);
    Serial.print(lati);
    Serial.print(sym_ovl);
    Serial.print(lon);
    Serial.print(sym_tab);
  } else if (type == _STATUS) {
    Serial.print(_DT_STATUS);
    Serial.print(mystatus);
  } else if (type == _FIXPOS_STATUS) {
    Serial.print(_DT_POS);
    Serial.print(lati);
    Serial.print(sym_ovl);
    Serial.print(lon);
    Serial.print(sym_tab);

    Serial.print(comment);
  } else {
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
  digitalWrite(vhfPttPin, HIGH);

  // Setup sleep pin
  pinMode(vhfSleepPin, OUTPUT);
  digitalWrite(vhfSleepPin, HIGH);

  // Setup power mode
  if (highPower) {
    pinMode(vhfPowerLevelPin, INPUT);
  } else {
    pinMode(vhfPowerLevelPin, OUTPUT);
    pinMode(vhfPowerLevelPin, LOW);
  }

  // Setup VHF UART
  // pinMode(vhfRxPin, INPUT);
  // pinMode(vhfTxPin, OUTPUT);

  // Wait for module to boot
  delay(vhfEnableDelay);
}

// Configures DRA818V settings
bool configureDra818v(float txFrequency = 144.39, float rxFrequency = 144.39,
                      bool emphasis = false, bool hpf = false,
                      bool lpf = false) {
  // Open serial connection
  Serial2.setTX(8);
  Serial2.setRX(9);
  Serial2.begin(9600);
  Serial2.setTimeout(vhfTimeout);

  // Handshake
  Serial2.println("AT+DMOCONNECT");
  if (!Serial2.find("+DMOcvf          ")) return false;
  delay(vhfEnableDelay);

  // Set frequencies and group
  Serial2.print("AT+DMOSETGROUP=0,");
  Serial2.print(txFrequency, 4);
  Serial2.print(',');
  Serial2.print(rxFrequency, 4);
  Serial2.println(",0000,0,0000");
  if (!Serial2.find("+DMOSETGROUP:0")) return false;
  delay(vhfEnableDelay);

  // Set filter settings
  Serial2.print("AT+SETFILTER=");
  Serial2.print(emphasis);
  Serial2.print(',');
  Serial2.print(hpf);
  Serial2.print(',');
  Serial2.println(lpf);
  if (!Serial2.find("+DMOSETFILTER:0")) return false;
  delay(vhfEnableDelay);

  Serial2.end();
  return true;
}

// Sets the push to talk state
void setPttState(bool state) { digitalWrite(vhfPttPin, state); }

// Sets the VHF module state
void setVhfState(bool state) { digitalWrite(vhfSleepPin, state); }

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  delay(500);
  Serial.println("Configuring DRA818V...");
  // Initialize DRA818V
  initializeDra818v();
  while (!configureDra818v()) Serial.println("DRA818V config failed");
  setPttState(false);
  setVhfState(true);
  // TODO Optimize for power, etc
  Serial.println("DRA818V configured");

  Serial.println("Configuring DAC...");
  // Initialize DAC
  Wire.setClock(3400000);
  Wire.begin();
  setSave(false);
  Serial.println("DAC configured");

  Serial.println("Setup complete");
  digitalWrite(LED_BUILTIN, LOW);
  delay(5000);
}

void loop() {
  /*
    parse_gprmc();
    coord_valid = get_coord();

    if(rmc_stat > 10)
    {
    //send_packet(random(1,4), random(1,3));
    if(coord_valid > 0)
  send_packet(_FIXPOS);
  else
    send_packet(_BEACON);
    }*/

  send_packet(_FIXPOS);
  delay(tx_delay);
}
