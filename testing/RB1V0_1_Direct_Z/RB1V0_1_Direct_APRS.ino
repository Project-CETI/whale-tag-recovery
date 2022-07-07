#include "pico/stdlib.h"
#include <stdio.h>
#include <math.h>
#include <time.h>

// APRS DEFS [START] ----------------------------------------------------
#define _FLAG 0x7e
#define _CTRL_ID 0x03
#define _PID 0xf0
#define _DT_EXP ','
#define _DT_STATUS '>'
#define _DT_POS '!'
#define tagSerial 2
#define _GPRMC 1
#define _FIXPOS 2
#define _FIXPOS_STATUS 3
#define _STATUS 4
#define _BEACON 5

// LED Pin
#define ledPin 29

// VHF control pins
#define vhfPowerLevelPin 15
#define vhfPttPin 16
#define vhfSleepPin 14
#define vhfTxPin 13
#define vhfRxPin 12
// VHF control params
#define vhfTimeout 500
#define vhfEnableDelay 1000

// VHF Output pins
#define out0Pin 18
#define out1Pin 19
#define out2Pin 20
#define out3Pin 21
#define out4Pin 22
#define out5Pin 23
#define out6Pin 24
#define out7Pin 25
#define bitPeriod 832
#define delay1200 19  // 23
#define delay2200 9   // 11
#define numSinValues 32
#define boardSN 2;
// APRS DEFS [END] ------------------------------------------------------

// APRS VARS [START] ----------------------------------------------------
// APRS communication config
char mycall[8] = "KC1QXQ";
char myssid = 5;
char dest[8] = "APLIGA";
char dest_beacon[8] = "BEACON";
char digi[8] = "WIDE2";
char digissid = 1;
char comment[128] = "Ceti v0.9 5";
char mystatus[128] = "Status";

char testsequence[20] = "Hello World";

// APRS protocol config
bool nada = 0;
const char sym_ovl = '/'; // Symbol Table ID, either \ or /
const char sym_tab = '-'; // Symbol Code
unsigned int str_len = 400;
char bitStuff = 0;
unsigned short crc = 0xffff;
uint64_t prevAprsTx = 0;
uint8_t currOutput = 0;
char rmc[100];
char rmc_stat;

// APRS payload arrays
char lati[9];
char lon[10];
char cogSpeed[8];

//intervals
uint32_t aprsInterval = 30000;

void setNada1200(void);
void setNada2400(void);
void set_nada(bool nada);

void sendCharNRZI(unsigned char in_byte, bool enBitStuff);
void sendStrLen(const char *in_string, int len);
void setPayload();

void calcCrc(bool in_bit);
void sendCrc(void);

void sendFlag(unsigned char flag_len);
void sendHeader();

const uint8_t sinValues[numSinValues] = {
  152, 176, 198, 217, 233, 245, 252, 255, 252, 245, 233,
  217, 198, 176, 152, 127, 103, 79,  57,  38,  22,  10,
  3,   1,   3,   10,  22,  38,  57,  79,  103, 128
};
// APRS VARS [END] ------------------------------------------------------

// APRS FUNCTIONS [START] -----------------------------------------------
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
  if (currOutput == numSinValues) currOutput = 0;
  setOutput(sinValues[currOutput]);
}

void setNada1200(void) {
  uint32_t endTime = micros() + bitPeriod;
  while (micros() < endTime) {
    setNextSin();
    delayMicroseconds(delay1200);
  }
}

void setNada2400(void) {
  uint32_t endTime = micros() + bitPeriod;
  while (micros() < endTime) {
    setNextSin();
    delayMicroseconds(delay2200);
  }
}

void set_nada(bool nada) {
  if (nada)
    setNada1200();
  else
    setNada2400();
}

/*
   This function will calculate CRC-16 CCITT for the FCS (Frame Check Sequence)
   as required for the HDLC frame validity check.

   Using 0x1021 as polynomial generator. The CRC registers are initialized with
   0xFFFF
*/
void calc_crc(bool inBit) {
  unsigned short xorIn;

  xorIn = crc ^ inBit;
  crc >>= 1;

  if (xorIn & 0x01) crc ^= 0x8408;
}

void sendCrc(void) {
  unsigned char crcLo = crc ^ 0xff;
  unsigned char crcHi = (crc >> 8) ^ 0xff;

  sendCharNRZI(crcLo, true);
  sendCharNRZI(crcHi, true);
}

void sendHeader() {
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
  sendCharNRZI('1' << 1, true);

  /********* SOURCE *********/
  temp = strlen(mycall);
  for (int j = 0; j < temp; j++) sendCharNRZI(mycall[j] << 1, true);
  if (temp < 6) {
    for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI((myssid + '0') << 1, true);

//  /********* DIGI ***********/
//  temp = strlen(digi);
//  for (int j = 0; j < temp; j++) sendCharNRZI(digi[j] << 1, true);
//  if (temp < 6) {
//    for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
//  }
//  sendCharNRZI(((digissid + '0') << 1) + 1, true);

  /***** CTRL FLD & PID *****/
  sendCharNRZI(_CTRL_ID, true);
  sendCharNRZI(_PID, true);
}

void setPayload() {
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
    snprintf(lati, 9, "%02d%02d.%02dS\0", latDeg, latMinInt, latMinDec);
  } else {
    snprintf(lati, 9, "%02d%02d.%02dN\0", latDeg, latMinInt, latMinDec);
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
    snprintf(lon, 10, "%03d%02d.%02dW\0", lonDeg, lonMinInt, lonMinDec);
  } else {
    snprintf(lon, 10, "%03d%02d.%02dE\0", lonDeg, lonMinInt, lonMinDec);
  }

  double speed = acs[2] / 1.852;  // convert speed from km/h to knots
  uint16_t course = (uint16_t)acs[1];
  if (course ==  0) {
    // APRS wants course in 1-360 and swarm provides it as 0-359
    course = 360;
  }
  int sog = (int) acs[2];
  Serial.print("speed: ");
  Serial.print(sog);
  snprintf(cogSpeed, 7, "%03d/%03d\0", course, sog);
  Serial.print(" and yet this thinks: ");
  Serial.println(cogSpeed);
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

void sendFlag(unsigned char flag_len) {
  for (int j = 0; j < flag_len; j++) sendCharNRZI(_FLAG, LOW);
}

void printPacket() {
  // initial flag
  Serial.print(_FLAG, HEX);
  /******** DEST ********/
  Serial.print(dest);
  Serial.print('0');
  /****** MYCALL ********/
  Serial.print(mycall);
  Serial.print(myssid, DEC);
  /******** DIGI ********/
  Serial.print(digi);
  Serial.print(digissid, DEC);
  // addn. stuff
  Serial.print(_CTRL_ID,DEC);
  Serial.print(_PID,DEC);
  // payload
  Serial.print(_DT_POS);
  Serial.print(lati);
  Serial.print(sym_ovl);
  Serial.print(lon);
  Serial.print(sym_tab);
  Serial.print(cogSpeed);
  Serial.print(comment);
  Serial.print(crc, HEX);
  Serial.println(_FLAG, HEX);
}

/*
   In this preliminary test, a packet is consists of FLAG(s) and PAYLOAD(s).
   Standard APRS FLAG is 0x7e character sent over and over again as a packet
   delimiter. In this example, 100 flags is used the preamble and 3 flags as
   the postamble.
*/
void sendPacket() {
  setLed(true);
  setPttState(true);
  setPayload();

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
  sendHeader();
//  send payload
  sendCharNRZI(_DT_POS, true);
  sendStrLen(lati, strlen(lati));
  sendCharNRZI(sym_ovl, true);
  sendStrLen(lon, strlen(lon));
  sendCharNRZI(sym_tab, true);
//  sendStrLen(cogSpeed, strlen(cogSpeed));
  sendStrLen(comment, strlen(comment));

//  sendStrLen(testsequence,strlen(testsequence));
  sendCrc();
  sendFlag(3);
  setLed(false);
  setPttState(false);
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
  } else {
    pinMode(vhfPowerLevelPin, OUTPUT);
    pinMode(vhfPowerLevelPin, LOW);
  }

  // Wait for module to boot
  delay(vhfEnableDelay);
}

// Configures DRA818V settings
bool configureDra818v(float txFrequency = 144.39, float rxFrequency = 144.39, bool emphasis = false, bool hpf = false, bool lpf = false) {
  // Open serial connection
  SerialPIO serial = SerialPIO(vhfRxPin, vhfTxPin);
  serial.begin(9600);
  serial.setTimeout(vhfTimeout);

  // Handshake
  serial.println("AT+DMOCONNECT");
  if (!serial.find("+DMOCONNECT:0")) return false;
  delay(vhfEnableDelay);

  // Set frequencies and group
  serial.print("AT+DMOSETGROUP=0,");
  serial.print(txFrequency, 4);
  serial.print(',');
  serial.print(rxFrequency, 4);
  serial.println(",0000,0,0000");
  if (!Serial1.find("+DMOSETGROUP:0")) return false;
  delay(serial);

  // Set filter settings
  serial.print("AT+SETFILTER=");
  serial.print(emphasis);
  serial.print(',');
  serial.print(hpf);
  serial.print(',');
  serial.println(lpf);
  if (!serial.find("+DMOSETFILTER:0")) return false;
  delay(vhfEnableDelay);
  serial.end();
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

// APRS FUNCTIONS [END] -------------------------------------------------
