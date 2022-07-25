#include <stdio.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "aprs.h"
#include "vhf.h"

#define PICO_TIME_DEFAULT_ALARM_POOL_DISABLED   0

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

bool q145 = false; // are we in 144.3MHz territory or 145.05MHz territory?

char callsign[8]="J73MAB";
int ssid = 15;
char destsign[8] = "APLIGA";
char digisign[8] = "WIDE2";
int dssid = 1; // digi ssid
char comment[128] = "Ceti b1.0 2-S"; // Ceti b#.# tag# OR Ceti b#.# #-#

// APRS payload arrays
char lati[9];
char lon[10];
char cogSpeed[8];

// APRS signal TX config
uint64_t endTimeAPRS = 0;
const uint64_t bitPeriod = 832;
const uint64_t delay1200 = 23;
const uint64_t delay2200 = 15;
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
    busy_wait_us(delay1200);
  }
}

void setNada2400(void) {
  endTimeAPRS = to_us_since_boot(get_absolute_time()) + bitPeriod;
  while (to_us_since_boot(get_absolute_time()) < endTimeAPRS) {
    setNextSin();
    busy_wait_us(delay2200);
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

void sendHeader(void) {
  int temp;
  /********* DEST ***********/

  temp = strlen(destsign);
  for (int j = 0; j < temp; j++) sendCharNRZI(destsign[j] << 1, true);
  if (temp < 6) {
    for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI('0' << 1, true);

  /********* SOURCE *********/
  temp = strlen(callsign);
  for (int j = 0; j < temp; j++) sendCharNRZI(callsign[j] << 1, true);
  if (temp < 6) {
    for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI((ssid + '0') << 1, true);

//  /********* DIGI ***********/
  temp = strlen(digisign);
  for (int j = 0; j < temp; j++) sendCharNRZI(digisign[j] << 1, true);
  if (temp < 6) {
    for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI(((dssid + '0') << 1) + 1, true);

  /***** CTRL FLD & PID *****/
  sendCharNRZI(_CTRL_ID, true);
  sendCharNRZI(_PID, true);
}

void sendPacket(float *latlon, uint16_t *acs) {
  if (latlon[0] < 17.71468 && !q145) {
    configureDra818v14505(145.05,145.05,false,false,false);
    q145 = true;
  }
  else if (latlon[0] >= 17.71468 && q145) {
    configureDra818v14439(144.39,144.39,false,false,false);
    q145 = false;
  }
  setVhfState(true);
  setPttState(true);
  setPayload(latlon, acs);

  // TODO TEST IF THIS IMPROVES RECEPTION
  // Send initialize sequence for receiver
  //sendCharNRZI(0x99, true);
  //sendCharNRZI(0x99, true);
  //sendCharNRZI(0x99, true);

  sendFlag(150);
  crc = 0xffff;
  sendHeader();
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
  setVhfState(false);
}

// Debug TX functions
void printPacket(void) {
  printf("%s0%s%d%s%d%x%x%c%s%c%s%c%s%s\n", destsign, callsign, ssid, digisign, dssid, _CTRL_ID, _PID, _DT_POS, lati, sym_ovl, lon, sym_tab, cogSpeed, comment);
}
void sendTestPackets(int style) {
  float latlon[2] = {0,0};
  uint16_t acs[3] = {0,0,0};
  switch(style) {
    case 1:
      // printf("Style 1: My latlon is: %f %f\n",latlon[0],latlon[1]);
      sendPacket(latlon, acs);
      break;
    case 2:
      latlon[0] = 42.3648;
      latlon[1] = -71.1247;
      // printf("Style 2: My latlon is: %f %f\n",latlon[0],latlon[1]);
      sendPacket(latlon, acs);
      break;
    case 3:
      sendHeader();
      sendStrLen(">...",strlen(">..."));
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
      // printf("Style D: My latlon is: %f %f\n",latlon[0],latlon[1]);
      sendPacket(latlon, acs);
      break;
  }
}

// Configuration functions
void configureAPRS(char *mcall, int mssid, char *dst, char *dgi, int dgssid, char *cmt) {
  configureVHF();
  q145 = false;
  memcpy(callsign, mcall, 8 * sizeof(*mcall));
  memcpy(destsign, dst, 8 * sizeof(*dst));
  memcpy(digisign, dgi, 8 * sizeof(*dgi));
  memcpy(comment, cmt, 128 * sizeof(*cmt));
  ssid = mssid;
  dssid = dgssid;
}

void describeConfig(void) {
  pinDescribe();
}
// APRS HEADERS [END] ---------------------------------------------------






















