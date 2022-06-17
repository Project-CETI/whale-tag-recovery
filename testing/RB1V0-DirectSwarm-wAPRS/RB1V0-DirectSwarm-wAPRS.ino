#include <stdio.h>
#include <math.h>
#include <time.h>
#include "TagRecoveryBoardv1_0_0.h"

// APRS DEFS [START] ----------------------------------------------------
#define _FLAG 0x7e
#define _CTRL_ID 0x03
#define _PID 0xf0
#define _DT_EXP ','
#define _DT_STATUS '>'
//#define _DT_POS '!'
#define _DT_POS '>'
#define tagSerial 2
#define _GPRMC 1
#define _FIXPOS 2
#define _FIXPOS_STATUS 3
#define _STATUS 4
#define _BEACON 5

// VHF control params
#define vhfTimeout 500
#define vhfEnableDelay 1000

#define bitPeriod 832
#define delay1200 19  // 23
#define delay2200 9   // 11
#define numSinValues 32
#define boardSN 2;
// APRS DEFS [END] ------------------------------------------------------

// SWARM DEFS [START] ---------------------------------------------------
#define MAX_SWARM_MSG_LEN 500
// SWARM DEFS [END] -----------------------------------------------------

// APRS VARS [START] ----------------------------------------------------
// APRS communication config
char mycall[8] = "KC1QXQ";
char myssid = 5;
char dest[8] = "APRS";
char dest_beacon[8] = "BEACON";
char digi[8] = "WIDE2";
char digissid = 1;
char comment[128] = "Ceti v0.9 5";
char mystatus[128] = "Status";

char testsequence[20] = "Hello World";

// APRS protocol config
bool nada = 0;
const char sym_ovl = '/'; // Symbol Table ID, either \ or /
const char sym_tab = '>'; // Symbol Code
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
char cogSpeed[7];

//intervals
uint32_t aprsInterval = 10000;

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

// SWARM VARS [START] ---------------------------------------------------
// swarm-communication processes
char modem_wr_buf[MAX_SWARM_MSG_LEN];
int modem_wr_buf_pos = 0;
char modem_rd_buf[MAX_SWARM_MSG_LEN];
int modem_rd_buf_pos = 0;
int buf_len = 0;
char endNmeaString = '*';
uint8_t nmeaCS;

// Parse SWARM
bool bootConfirmed = false;
bool initDTAck = false;
bool initPosAck = false;
// GPS data hacks
float latlon[2] = {0,0};
int acs[3] = {0,0,0};
int gpsReadPos = 4;
int gpsCopyPos = 0;
char gpsParseBuf[MAX_SWARM_MSG_LEN];
int gpsInsertPos = 0;
int gpsMult = 1;
bool gpsFloatQ = false;
// SWARM VARS [END] -----------------------------------------------------

// APRS FUNCTIONS [START] -----------------------------------------------
void initializeOutput() {
  pinMode(dacPin0, OUTPUT);
  pinMode(dacPin1, OUTPUT);
  pinMode(dacPin2, OUTPUT);
  pinMode(dacPin3, OUTPUT);
  pinMode(dacPin4, OUTPUT);
  pinMode(dacPin5, OUTPUT);
  pinMode(dacPin6, OUTPUT);
  pinMode(dacPin7, OUTPUT);

}

void setOutput(uint8_t state) {
  digitalWrite(dacPin0, state & 0b00000001);
  digitalWrite(dacPin1, state & 0b00000010);
  digitalWrite(dacPin2, state & 0b00000100);
  digitalWrite(dacPin3, state & 0b00001000);
  digitalWrite(dacPin4, state & 0b00010000);
  digitalWrite(dacPin5, state & 0b00100000);
  digitalWrite(dacPin6, state & 0b01000000);
  digitalWrite(dacPin7, state & 0b10000000);
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
  sendCharNRZI('0' << 1, true);

  /********* SOURCE *********/
  temp = strlen(mycall);
  for (int j = 0; j < temp; j++) sendCharNRZI(mycall[j] << 1, true);
  if (temp < 6) {
    for (int j = temp; j < 6; j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI((myssid + '0') << 1, true);

  /********* DIGI ***********/
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
  bool west = true;
  if (lonFloat < 0) {
    west = true;
    lonFloat = fabs(lonFloat);
  }
  int lonDeg = (int) lonFloat;
  float lonMin = lonFloat - lonDeg; // get decimal degress from float
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
  snprintf(cogSpeed, 7, "%03d/%03d\0", course, sog);
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
  Serial.print(_FLAG, DEC);
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
  Serial.print(crc);
  Serial.println(_FLAG, DEC);
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
  sendCharNRZI('>', true);
  sendStrLen(lati, strlen(lati));
  sendCharNRZI(sym_ovl, true);
  sendStrLen(lon, strlen(lon));
  sendCharNRZI(sym_tab, true);
  sendStrLen(cogSpeed, strlen(cogSpeed));
//  sendStrLen(comment, strlen(comment));

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

// SWARM FUNCTIONS [START] ----------------------------------------------
uint8_t nmeaChecksum(const char *sz, size_t len) {
  size_t i = 0;
  uint8_t cs;
  if (sz[0] == '$')
    i++;
  for (cs = 0; (i < len) && sz [i]; i++)
    cs ^= ((uint8_t) sz[i]);
  return cs;
}

void storeGpsData() {
  while(modem_rd_buf[gpsReadPos] != '*') {
    while (modem_rd_buf[gpsReadPos] != ',' && modem_rd_buf[gpsReadPos] != '*') {
      if (modem_rd_buf[gpsReadPos] == '-') {
        gpsMult = -1;
      }
      else {
        if (modem_rd_buf[gpsReadPos] == '.') {
          gpsFloatQ = true;
        }
        gpsParseBuf[gpsCopyPos] = modem_rd_buf[gpsReadPos];
        gpsCopyPos++;
      }
      gpsReadPos++;
    }
    gpsParseBuf[gpsCopyPos] = '\0';
    if (gpsFloatQ) {
      latlon[gpsInsertPos] = ((float)gpsMult) * atof(gpsParseBuf);
    }
    else {
      acs[gpsInsertPos] = gpsMult * atoi(gpsParseBuf);
    }
    if (modem_rd_buf[gpsReadPos] == '*') {
      gpsReadPos = 4;
      gpsCopyPos = 0;
      gpsInsertPos = 0;
      gpsMult = 1;
      break;
    }
    gpsReadPos++;
    gpsCopyPos = 0;
    (gpsFloatQ && gpsInsertPos==1) ? gpsInsertPos-- : gpsInsertPos++;
    gpsFloatQ = false;
    gpsMult = 1;
  }
}

int parseModemOutput() {
  if (modem_rd_buf[2] == 'N' && modem_rd_buf[4] != 'E' && modem_rd_buf[4] != 'O') {
    storeGpsData();
  }
  else if (modem_rd_buf[1] == 'T' && modem_rd_buf[4] == 'S') {
    txSwarm();
  }
  return 0;
}

void readFromModem() {
  if (Serial1.available()) {
    modem_rd_buf[modem_rd_buf_pos] = Serial1.read();
    
    if (modem_rd_buf[modem_rd_buf_pos] != '\n') {
      modem_rd_buf_pos++;
    }
    else {
      modem_rd_buf[modem_rd_buf_pos] = '\0';
      buf_len = modem_rd_buf_pos;
      modem_rd_buf_pos = 0;
      Serial.println(modem_rd_buf);
      parseModemOutput();
    }
  }
}

int checkSwarmBooted() {
  if (buf_len != 21) {
    return 0; // Can't be BOOT,RUNNING message
  }
  if (modem_rd_buf[6] != 'B') {
    return 0; // Not BOOT sequence
  }
  if (modem_rd_buf[buf_len-1] != 'a') {
    return 0; // Not the right checksum
  }
  bootConfirmed = true;
  return 0;
}

int checkInitAck() {
  if (modem_rd_buf[16] == '6') {
    initDTAck = true;
    return 0;
  }
  else if (modem_rd_buf[16] == 'e') {
    initPosAck = true;
  }
  return 0;
}

void swarmBootSequence(bool waitForAcks) {
  while(!bootConfirmed) {
    readFromModem();
    checkSwarmBooted();
  }
  while((!initDTAck || !initPosAck) && waitForAcks) {
    serialCopyToModem();
    readFromModem();
    checkInitAck();
  }
}

void serialCopyToModem() {
  if (Serial.available()) {
    modem_wr_buf[modem_wr_buf_pos] = Serial.read();
    
    if (modem_wr_buf[modem_wr_buf_pos] != '\n') {
      modem_wr_buf_pos++;
    }
    else {
      modem_wr_buf[modem_wr_buf_pos] = '\0';
      modem_wr_buf_pos = 0;
      if (modem_wr_buf[0] == '$') {
        writeToModem(modem_wr_buf);
      }
    }
  }
}

void writeToModem(char *sz) {
  nmeaCS = nmeaChecksum(sz, strlen(sz));
  Serial.printf("[SENT] %s%c%02X\n",sz,endNmeaString,nmeaCS);
  Serial1.printf("%s%c%02X\n",sz,endNmeaString,nmeaCS);
}

void getRxTestOutput(bool onQ) {
  onQ ? writeToModem("$RT 1") : writeToModem("$RT 0");
}

void getQCount() {
  writeToModem("$MT C=U");
}

void swarmInit(bool swarmDebug) {
  getRxTestOutput(false);
  getQCount();
  if (swarmDebug) {
    writeToModem("$TD \"Test 1\"");
    writeToModem("$TD \"Test 2\"");
    writeToModem("$TD \"Test 3\"");
  }
  writeToModem("$GN 5");
}
// SWARM FUNCTIONS [END] ------------------------------------------------

bool ledState = false;
bool ledQ = false;
uint64_t prevLedTime = 0;
uint32_t ledOn = 0;
void setLed(bool state) {digitalWrite(ledPin, state);}
void initLed() {pinMode(ledPin, OUTPUT);}

bool swarmRunning = false;
bool waitForAcks = false;
bool aprsRunning = false;

void txSwarm() {
  if (initDTAck) {
    writeToModem("$TD \"Transmit Queue\"");
  }
}

void txAprs() {
  prevAprsTx = millis();
  // What are we sending, hopefully?
//  Serial.print("GPS Data|| lat: ");
//  Serial.print(latlon[0],4);
//  Serial.print(" | lon: ");
//  Serial.print(latlon[1],4);
//  Serial.printf(" | A: %d | C: %d | S: %d\n", acs[0], acs[1], acs[2]);
  // End Debug
  uint32_t startPacket = millis();
  sendPacket();
  uint32_t packetDuration = millis() - startPacket;
  Serial.println("Packet sent in: " + String(packetDuration) + " ms");
  printPacket();
}

void setup() {
//  swarmRunning = true;
  aprsRunning = true;
//  waitForAcks = true;
  initLed();
  setLed(true);

  // Set SWARM first, we need to grab those acks
  Serial.begin(115200);
  Serial1.setTX(0);
  Serial1.setRX(1);
  Serial1.begin(115200);
  swarmBootSequence(waitForAcks && swarmRunning);
  Serial.println("Swarm booted.");

  // DRA818V initialization
  Serial.println("Configuring DRA818V...");
  // Initialize DRA818V
  initializeDra818v();
  configureDra818v();
  setPttState(false);
  setVhfState(true);
  Serial.println("DRA818V configured");

  // Initialize DAC
  Serial.println("Configuring DAC...");
  initializeOutput();
  Serial.println("DAC configured");

  if(swarmRunning) swarmInit(swarmRunning && waitForAcks);
}

void loop() {
  serialCopyToModem();
  readFromModem();
  // Update LED state
  ledQ = (millis() - prevLedTime >= ledOn);
  setLed(!ledQ);
  ledState = !ledQ;
  // Transmit APRS
  if ((millis() - prevAprsTx >= aprsInterval) && aprsRunning) {
    txAprs();
  }
}
