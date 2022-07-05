#include <stdio.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "uart_tx.pio.h"
#include "ceti_aprs.h"

// VHF VARS [START] -----------------------------------------------------
// VHF control pins
const uint vhfPowerLevelPin = 15;
const uint vhfPttPin = 16;
const uint vhfSleepPin = 14;
const uint vhfTxPin = 13;
const uint vhfRxPin = 12;
// VHF control params
const uint vhfTimeout = 500;
const uint vhfEnableDelay = 1000;

// VHF Output Pins
const uint out0Pin = 18;
const uint out1Pin = 19;
const uint out2Pin = 20;
const uint out3Pin = 21;
const uint out4Pin = 22;
const uint out5Pin = 23;
const uint out6Pin = 24;
const uint out7Pin = 25;

const uint32_t vhfPinMask = 0x3FC0000; // bits 18-25 are 1, everything else 0
const uint vhfPinShift = 18; // left shift values 18 bits to align properly
// VHF VARS [END] -------------------------------------------------------

// APRS VARS [START] ----------------------------------------------------
// APRS protocol config (don't change)
const uint16_t _FLAG = 0x7e;
const uint16_t _CTRL_ID = 0x03;
const uint16_t _PID = 0xf0;
const char _DT_POS = '!';
bool nada = 0;
const char sym_ovl = '\\'; // Symbol Table ID, either \ or /
const char sym_tab = '-'; // Symbol Code
unsigned int str_len = 400;
char bitStuff = 0;
unsigned short crc = 0xffff;
uint8_t currOutput = 0;
char rmc[100];
char rmc_stat;

// APRS payload arrays
char lati[9];
char lon[10];
char cogSpeed[8];

// APRS signal TX config
uint64_t endTimeAPRS = 0;
const uint64_t bitPeriod = 832;
const uint64_t delay1200 = 19;
const uint64_t delay2200 = 9;
const uint8_t sinValues[32] = {
  152, 176, 198, 217, 233, 245, 252, 255, 252, 245, 233,
  217, 198, 176, 152, 127, 103, 79,  57,  38,  22,  10,
  3,   1,   3,   10,  22,  38,  57,  79,  103, 128
};
const uint8_t numSinValues = 32;
// APRS VARS [END] ------------------------------------------------------


// Low-level TX functions
void setNextSin(void) {
  currOutput++;
  if (currOutput == numSinValues) currOutput = 0;
  setOutput(sinValues[currOutput]);
}

void setNada1200(void) {
  endTimeAPRS = to_us_since_boot(get_absolute_time()) + bitPeriod;
  while (to_us_since_boot(get_absolute_time()) < endTimeAPRS) {
    setNextSin();
    sleep_us(delay1200);
  }
}

void setNada2400(void) {
  endTimeAPRS = to_us_since_boot(get_absolute_time()) + bitPeriod;
  while (to_us_since_boot(get_absolute_time()) < endTimeAPRS) {
    setNextSin();
    sleep_us(delay2200);
  }
}

void set_nada(bool nada) {
  if (nada)
    setNada1200();
  else
    setNada2400();
}

// Mid-level TX functions
// CRC-16 CCITT, using 0x1021, init at 0xffff
void calc_crc(bool inBit) {
  unsigned short xorIn;

  xorIn = crc ^ inBit;
  crc >>= 1;

  if (xorIn & 0x01) crc ^= 0x8408;
}

void sendCrc(void) {
  uint8_t crcLo = crc ^ 0xff;
  uint8_t crcHi = (crc >> 8) ^ 0xff;

  sendCharNRZI(crcLo, true);
  sendCharNRZI(crcHi, true);
}
/*
   This function will send one byte input and convert it
   into AFSK signal one bit at a time LSB first.

   The encode which used is NRZI (Non Return to Zero, Inverted)
   bit 1 : transmitted as no change in tone
   bit 0 : transmitted as change in tone
*/
void sendCharNRZI(unsigned char in_byte, bool enBitStuff) {
  bool bits;

  for (int i = 0; i < 8; i++) {
    bits = in_byte & 0x01;

    calc_crc(bits);

    if (bits) {
      set_nada(nada);
      bitStuff++;

      if ((enBitStuff) && (bitStuff == 5)) {
        nada ^= 1;
        set_nada(nada);

        bitStuff = 0;
      }
    } else {
      nada ^= 1;
      set_nada(nada);

      bitStuff = 0;
    }

    in_byte >>= 1;
  }
}

void sendStrLen(const char *in_string, int len) {
  for (int j = 0; j < len; j++) sendCharNRZI(in_string[j], true);
}

void sendFlag(int flag_len) {
  for (int j = 0; j < flag_len; j++) sendCharNRZI(_FLAG, false);
}

// High-level TX functions
void setPayload(float *latlon, uint16_t *acs) {
  float latFloat = latlon[0];
  bool south = false;
  if (latFloat < 0) {
    south = true;
    latFloat = fabs(latFloat);
  }
  int latDeg = (int) latFloat;
  float latMin = latFloat - latDeg; // get decimal degress from float
  latMin = latMin * 60; // convert decimal degrees to minutes
  uint8_t latMinInt = (int)latMin;
  uint8_t latMinDec = round((latMin - latMinInt) * 100);

  if (south) {
    snprintf(lati, 9, "%02d%02d.%02dS", latDeg, latMinInt, latMinDec);
  } else {
    snprintf(lati, 9, "%02d%02d.%02dN", latDeg, latMinInt, latMinDec);
  }
  float lonFloat = latlon[1];
  bool west = false;
  if (lonFloat < 0) {
    west = true;
    lonFloat = fabs(lonFloat);
  }
  int lonDeg = (int) lonFloat;
  float lonMin = lonFloat - lonDeg; // get decimal degress from float
  lonMin = lonMin * 60;
  uint8_t lonMinInt = (int)lonMin;
  uint8_t lonMinDec = round((lonMin - lonMinInt) * 100);
  if (west) {
    snprintf(lon, 10, "%03d%02d.%02dW", lonDeg, lonMinInt, lonMinDec);
  } else {
    snprintf(lon, 10, "%03d%02d.%02dE", lonDeg, lonMinInt, lonMinDec);
  }

  double speed = acs[2] / 1.852;  // convert speed from km/h to knots
  uint16_t course = (uint16_t)acs[1];
  if (course ==  0) {
    // APRS wants course in 1-360 and swarm provides it as 0-359
    course = 360;
  }
  int sog = (int) acs[2];
  snprintf(cogSpeed, 8, "%03d/%03d", course, sog);
}

void sendHeader(char *mycall, int myssid, char *dest, char *digi, int digissid) {
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

  temp = strlen(dest);
  for (int j = 0; j < temp; j++) sendCharNRZI(dest[j] << 1, true);
  if (temp < 6) {
    for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI('0' << 1, true);

  /********* SOURCE *********/
  temp = strlen(mycall);
  for (int j = 0; j < temp; j++) sendCharNRZI(mycall[j] << 1, true);
  if (temp < 6) {
    for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI((myssid + '0') << 1, true);

//  /********* DIGI ***********/
  temp = strlen(digi);
  for (int j = 0; j < temp; j++) sendCharNRZI(digi[j] << 1, true);
  if (temp < 6) {
    for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI(((digissid + '0') << 1) + 1, true);

  /***** CTRL FLD & PID *****/
  sendCharNRZI(_CTRL_ID, true);
  sendCharNRZI(_PID, true);
}

void sendPacket(float *latlon, uint16_t *acs, char *mycall, int myssid, char *dest, char *digi, int digissid, char *comment) {
  setPttState(true);
  setPayload(latlon, acs);

  // TODO TEST IF THIS IMPROVES RECEPTION
  // Send initialize sequence for receiver
  //sendCharNRZI(0x99, true);
  //sendCharNRZI(0x99, true);
  //sendCharNRZI(0x99, true);


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

  sendFlag(150);
  crc = 0xffff;
  sendHeader(mycall, myssid, dest, digi, digissid);
  // send payload
  sendCharNRZI(_DT_POS, true);
  sendStrLen(lati, strlen(lati));
  sendCharNRZI(sym_ovl, true);
  sendStrLen(lon, strlen(lon));
  sendCharNRZI(sym_tab, true);
  sendStrLen(cogSpeed, strlen(cogSpeed));
  sendStrLen(comment, strlen(comment));

  sendCrc();
  sendFlag(3);
  setPttState(false);
}

// Debug TX functions
void printPacket(char *mycall, int myssid, char *dest, char *digi, int digissid, char *comment) {
  printf("%s0%s%d%s%d%c%s%c%s%c%s%s\n", dest, mycall, myssid, digi, digissid, _DT_POS, lati, sym_ovl, lon, sym_tab, cogSpeed, comment);
}
void sendTestPackets(char *mycall, int myssid, char *dest, char *digi, int digissid, char *comment, int style) {
  float latlon[2] = {0,0};
  uint16_t acs[3] = {0,0,0};
  switch(style) {
    case 1:
      sendPacket(latlon, acs, mycall, myssid, dest, digi, digissid, comment);
      break;
    case 2:
      latlon[0] = 42.3648;
      latlon[1] = -71.1247;
      sendPacket(latlon, acs, mycall, myssid, dest, digi, digissid, comment);
      break;
    case 3:
      latlon[0] = 0.0;
      latlon[1] = 0.0;
      sendPacket(latlon, acs, mycall, myssid, dest, digi, digissid, comment);
      sleep_ms(1000);
      latlon[0] = 42.3648;
      latlon[1] = -71.1247;
      sendPacket(latlon, acs, mycall, myssid, dest, digi, digissid, comment);
      break;
    case 4:
      crc = 0xffff;
      sendStrLen("123456789",9);
      sendCrc();
      crc = 0xffff;
      sendStrLen("A",1);
      sendCrc();
      break;
    default:
      sendPacket(latlon, acs, mycall, myssid, dest, digi, digissid, comment);
      break;
  }
}
// APRS HEADERS [END] ---------------------------------------------------

// VHF HEADERS [START] --------------------------------------------------
void initializeOutput(void) {
  gpio_init_mask(vhfPinMask);
  gpio_set_dir_masked(vhfPinMask,vhfPinMask);
}

void setOutput(uint8_t state) {
  gpio_put_masked(vhfPinMask, (state << vhfPinShift) & vhfPinMask);
}

void initializeDra818v(bool highPower) {
  gpio_init(vhfPttPin); gpio_set_dir(vhfPttPin, GPIO_OUT);
  gpio_put(vhfPttPin, false);
  gpio_init(vhfSleepPin); gpio_set_dir(vhfSleepPin, GPIO_OUT);
  gpio_put(vhfSleepPin, true);

  gpio_init(vhfPowerLevelPin);
  if (highPower) {
    gpio_set_dir(vhfPowerLevelPin, GPIO_IN);
  }
  else {
    gpio_set_dir(vhfPowerLevelPin, GPIO_OUT);
    gpio_put(vhfPowerLevelPin, 0);
  }

  sleep_ms(vhfEnableDelay);
}
bool configureDra818v(float txFrequency, float rxFrequency, bool emphasis, bool hpf, bool lpf) {
  // char temp[100];
  PIO pio = pio0;
  uint sm = pio_claim_unused_sm(pio, true);
  uint offset = pio_add_program(pio, &uart_tx_program);
  uart_tx_program_init(pio, sm, offset, vhfTxPin, 9600);
  uart_tx_program_puts(pio, sm, "AT+DMOCONNECT");
  sleep_ms(vhfEnableDelay);
  uart_tx_program_puts(pio, sm, "AT+DMOSETGROUP=0,144.39,144.39,0000,0,0000\n");
  sleep_ms(vhfEnableDelay);
  // sprintf(temp, "AT+SETFILTER=%d,%d,%d\n",emphasis,hpf,lpf);
  uart_tx_program_puts(pio, sm, "AT+SETFILTER=0,0,0\n");
  sleep_ms(vhfEnableDelay);
  return true;
}

void setPttState(bool state) {gpio_put(vhfPttPin, state);}
void setVhfState(bool state) {gpio_put(vhfSleepPin, state);}

void configureVHF(void) {
    sleep_ms(10000);
    printf("Configuring DRA818V...\n");
    initializeDra818v(true);
    configureDra818v(144.39,144.39,false,false,false);
    setPttState(false);
    setVhfState(true);
    printf("DRA818V configured.\n");

    printf("Configuring DAC...\n");
    initializeOutput();
    printf("DAC configured.\n");
}
// VHF HEADERS [END] ----------------------------------------------------

void pinDescribe(void) {
  bi_decl(bi_1pin_with_name(vhfPowerLevelPin, "VHF Module Power Level Pin"));
  bi_decl(bi_1pin_with_name(vhfPttPin, "VHF Module PTT Pin"));
  bi_decl(bi_1pin_with_name(vhfSleepPin, "VHF Module Sleep Pin"));
  bi_decl(bi_1pin_with_name(vhfTxPin, "VHF Module TX Pin for sPIO"));
  bi_decl(bi_1pin_with_name(vhfRxPin, "VHF Module RX Pin for sPIO"));
  bi_decl(bi_1pin_with_name(out0Pin, "DAC Pin 0"));
  bi_decl(bi_1pin_with_name(out1Pin, "DAC Pin 1"));
  bi_decl(bi_1pin_with_name(out2Pin, "DAC Pin 2"));
  bi_decl(bi_1pin_with_name(out3Pin, "DAC Pin 3"));
  bi_decl(bi_1pin_with_name(out4Pin, "DAC Pin 4"));
  bi_decl(bi_1pin_with_name(out5Pin, "DAC Pin 5"));
  bi_decl(bi_1pin_with_name(out6Pin, "DAC Pin 6"));
  bi_decl(bi_1pin_with_name(out7Pin, "DAC Pin 7"));
}
