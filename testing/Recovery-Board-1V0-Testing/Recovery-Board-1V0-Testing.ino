#include <SparkFun_Swarm_Satellite_Arduino_Library.h>
#include <math.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include <time.h>
#include "LittleFS.h"

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

//intervals
uint32_t aprsInterval = 60000;
uint32_t swarmInterval = 300000; // swarm Tx update interval in ms
long logInterval = 60000;

// stall on gps?
bool aprsGpsStall = false; // Stall for GPS signal before sending APRS (keep false unless really necessary)
bool swarmGpsStall = true; // Stall for GPS signal before sending Swarm (keep true unless really necessary)

// log extra(s)
long prevLogTime = 0;

SWARM_M138 swarm;
LittleFSConfig cfg;
uint16_t qCount;

void setLed(bool state) {
  digitalWrite(ledPin, state);
}
// APRS communication config
char mycall[8] = "KC1QXQ";
char myssid = 5; // Shashank's test board gets meaningless fun priority
char dest[8] = "APLIGA";
char dest_beacon[8] = "BEACON";
char digi[8] = "WIDE2";
char digissid = 1;
char comment[128] = "Ceti v1.0b3";
char mystatus[128] = "Status";

// APRS protocol config
bool nada = 0;
const char sym_ovl = 'T';
const char sym_sti = '/'; // Symbol Table Identifier
const char sym_house = '-'; // 'house' Symbol Code
const char sym_tab = 'a';
unsigned int str_len = 400;
char bitStuff = 0;
unsigned short crc = 0xffff;
uint64_t prevAprsTx = 0;
uint64_t prevSwarmQueue = 0;
bool ledState = true;
uint64_t prevLedTime = 0;
uint32_t ledOn = 0;
uint8_t currOutput = 0;
char rmc[100];
char rmc_stat;

void setNada1200(void);
void setNada2400(void);
void set_nada(bool nada);

void sendCharNRZI(unsigned char in_byte, bool enBitStuff);
void sendStrLen(const char *in_string, int len);
void sendPayload(Swarm_M138_GeospatialData_t *locationInfo, char *status);

void calcCrc(bool in_bit);
void sendCrc(void);

void sendFlag(unsigned char flag_len);
void sendHeader();

//void types(String a) {
//  Serial.println("it's a String");
//}
//void types(int a) {
//  Serial.println("it's an int");
//}
//void types(char *a) {
//  Serial.println("it's a char*");
//}
//void types(float a) {
//  Serial.println("it's a float");
//}
//void types(bool a) {
//  Serial.println("it's a bool");
//}

const uint8_t sinValues[numSinValues] = {
  152, 176, 198, 217, 233, 245, 252, 255, 252, 245, 233,
  217, 198, 176, 152, 127, 103, 79,  57,  38,  22,  10,
  3,   1,   3,   10,  22,  38,  57,  79,  103, 128
};

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
    for (int j = 0; j < (6 - temp); j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI('0' << 1, true);

  /********* SOURCE *********/
  temp = strlen(mycall);
  for (int j = 0; j < temp; j++) sendCharNRZI(mycall[j] << 1, true);
  if (temp < 6) {
    for (int j = 0; j < (6 - temp); j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI((myssid + '0') << 1, true);

  /********* DIGI ***********/
  temp = strlen(digi);
  for (int j = 0; j < temp; j++) sendCharNRZI(digi[j] << 1, true);
  if (temp < 6) {
    for (int j = 0; j < (6 - temp); j++) sendCharNRZI(' ' << 1, true);
  }
  sendCharNRZI(((digissid + '0') << 1) + 1, true);

  /***** CTRL FLD & PID *****/
  sendCharNRZI(_CTRL_ID, true);
  sendCharNRZI(_PID, true);
}

void sendPayload(Swarm_M138_GeospatialData_t *locationInfo, char *status_) {
  float latFloat = locationInfo->lat;
  if (locationInfo->lat == 0) {
    return;
  }
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

  char lati[10];
  if (south) {
    snprintf(lati, 9, "%02d%02d.%02dS", latDeg, latMinInt, latMinDec);
  } else {
    snprintf(lati, 9, "%02d%02d.%02dN", latDeg, latMinInt, latMinDec);
  }
  float lonFloat = locationInfo->lon;
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
  char lon[10];
  if (west) {
    snprintf(lon, 10, "%03d%02d.%02dW", lonDeg, lonMinInt, lonMinDec);
  } else {
    snprintf(lon, 10, "%03d%02d.%02dE", lonDeg, lonMinInt, lonMinDec);
  }

  double speed = locationInfo->speed / 1.852;  // convert speed from km/h to knots
  uint16_t course = (uint16_t)locationInfo->course;
  if (course ==  0) {
    // APRS wants course in 1-360 and swarm provides it as 0-359
    course = 360;
  }
  char cogSpeed[7];
  int sog = (int) locationInfo->speed;
  snprintf(cogSpeed, 7, "%03d/%03d", course, sog);
  Serial.begin(115200);
  /****** MYCALL ********/
  Serial.print(mycall);
  Serial.print('-');
  Serial.print(myssid, DEC);
  Serial.print('>');
  /******** DEST ********/
  Serial.print(dest);
  Serial.print(',');
  /******** DIGI ********/
  Serial.print(digi);
  Serial.print('-');
  Serial.print(digissid, DEC);
  Serial.print(':');
  /******* PAYLOAD ******/
  Serial.print(_DT_POS);
  Serial.print(lati);
  Serial.print(sym_ovl);
  Serial.print(lon);
  Serial.print(sym_tab);
  Serial.println(' ');
  Serial.flush();
  sendCharNRZI(_DT_POS, true);
  sendStrLen(lati, strlen(lati));
//  sendCharNRZI(sym_sti, true);
  sendCharNRZI(sym_ovl, true);
  sendStrLen(lon, strlen(lon));
//  sendCharNRZI(sym_house, true);
  sendStrLen(cogSpeed, strlen(cogSpeed));
  sendCharNRZI(sym_tab, true);
  sendStrLen(comment, strlen(comment));
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

/*
   In this preliminary test, a packet is consists of FLAG(s) and PAYLOAD(s).
   Standard APRS FLAG is 0x7e character sent over and over again as a packet
   delimiter. In this example, 100 flags is used the preamble and 3 flags as
   the postamble.
*/
void sendPacket(Swarm_M138_GeospatialData_t *locationInfo) {
  setLed(true);
  setPttState(true);

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
  if (locationInfo->lat != 0) {
    sendPayload(locationInfo, "");
  }
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

void initLed() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, false);
}

void stallForGps(Swarm_M138_GPS_Fix_Quality_t *gpsQuality, bool stall) {
  swarm.getGpsFixQuality(gpsQuality);
  Serial.print("fix quality = ");
  Serial.print(gpsQuality->fix_type);
  if (stall) {
    while (gpsQuality->fix_type == SWARM_M138_GPS_FIX_TYPE_TT || gpsQuality->fix_type == SWARM_M138_GPS_FIX_TYPE_INVALID || gpsQuality->fix_type == SWARM_M138_GPS_FIX_TYPE_NF) {
      delay(1000);
      swarm.getGpsFixQuality(gpsQuality);
      Serial.print(" ... ");
      Serial.print(gpsQuality->fix_type);
    }
  }
  else if (!stall && (gpsQuality->fix_type == SWARM_M138_GPS_FIX_TYPE_TT || gpsQuality->fix_type == SWARM_M138_GPS_FIX_TYPE_INVALID || gpsQuality->fix_type == SWARM_M138_GPS_FIX_TYPE_NF)) {
    Serial.print(" ... No GPS.");
  }
  Serial.println();
}

void printSwarmError(Swarm_M138_Error_e err) {
  Serial.print(F("Swarm communication error: "));
  Serial.print((int)err);
  Serial.print(F(" : "));
  Serial.print(swarm.modemErrorString(err)); // Convert the error into printable text
  if (err == SWARM_M138_ERROR_ERR) // If we received a command error (ERR), print it
  {
    Serial.print(F(" : "));
    Serial.print(swarm.commandError);
    Serial.print(F(" : "));
    Serial.println(swarm.commandErrorString((const char *)swarm.commandError));
  }
  else
    Serial.println();
}

void printSwarmQueueStatus() {
  swarm.getUnsentMessageCount(&qCount); // Get the number of untransmitted messages
  Serial.print(F("There are "));
  Serial.print(qCount);
  Serial.println(F(" unsent messages in the SWARM TX queue"));
  Serial.println();
  logSwarm();
  if (qCount >= 1024) {
    Serial.println("Clearing SWARM TX queue ...");
    swarm.deleteAllTxMessages();
    Serial.println("Restart queue from current ...");
    txSwarm();
  }
}

void printSwarmTxStatus(const char *msg, Swarm_M138_Error_e err) {
  printSwarmError(err);
  Serial.print("TX Text");
  Serial.print(msg);
  Serial.println();
  // Housekeeping on queue count
  printSwarmQueueStatus();
}

void txSwarm() {
  Swarm_M138_GPS_Fix_Quality_t *gpsQuality = new Swarm_M138_GPS_Fix_Quality_t;
  // START OF STATUS PRINTING
  stallForGps(gpsQuality, swarmGpsStall);
  // END OF STATUS PRINTING
  prevSwarmQueue = millis();
  Swarm_M138_GeospatialData_t *info = new Swarm_M138_GeospatialData_t;
  swarm.getGeospatialInfo(info);
  char lat[7]; dtostrf(info->lat,4,3,lat);
  char lon[7]; dtostrf(info->lon,4,3,lon);
  Swarm_M138_DateTimeData_t *dateTime = new Swarm_M138_DateTimeData_t;
  swarm.getDateTime(dateTime);
  char message[200];
  sprintf(message, "SN%d=%s,%s@%d/%d/%d/%d/%d/%d:%d\0", tagSerial, lat, lon, dateTime->YYYY, dateTime->MM, dateTime->DD, dateTime->hh, dateTime->mm, dateTime->ss, qCount);
  uint64_t *id;
  Swarm_M138_Error_e err = swarm.transmitText(message, id);
  if (err == SWARM_M138_SUCCESS) {
    digitalWrite(ledPin, true);
    prevLedTime = millis();
    ledOn = 50;
    ledState = true;
  }
  // START OF STATUS PRINTING
  printSwarmTxStatus(message, err);
  // END OF STATUS PRINTING
  delete dateTime;
  delete info;
  delete id;
  delete gpsQuality;
}

void printRxTest(const Swarm_M138_Receive_Test_t *rxTest)
{
  Serial.print(F("New receive test message received:"));
  if (rxTest->background) // Check if rxTest contains only the background RSSI
  {
    Serial.print(F("  rssi_background: "));
    Serial.print(rxTest->rssi_background);
    if (rxTest->rssi_background <= -105)
      Serial.println(F(" (Great)"));
    else if (rxTest->rssi_background <= -100)
      Serial.println(F(" (Good)"));
    else if (rxTest->rssi_background <= -97)
      Serial.println(F(" (OK)"));
    else if (rxTest->rssi_background <= -93)
      Serial.println(F(" (Marginal)"));
    else
      Serial.println(F(" (Bad)"));
  }
  else
  {
    Serial.print(F("  rssi_background: "));
    Serial.print(rxTest->rssi_background);
    Serial.print(F("  rssi_sat: "));
    Serial.print(rxTest->rssi_sat);
    Serial.print(F("  snr: "));
    Serial.print(rxTest->snr);
    Serial.print(F("  fdev: "));
    Serial.print(rxTest->fdev);
    Serial.print(F("  "));
    Serial.print(rxTest->time.YYYY);
    Serial.print(F("/"));
    if (rxTest->time.MM < 10) Serial.print(F("0")); Serial.print(rxTest->time.MM); // Print the month. Add a leading zero if required
    Serial.print(F("/"));
    if (rxTest->time.DD < 10) Serial.print(F("0")); Serial.print(rxTest->time.DD); // Print the day of month. Add a leading zero if required
    Serial.print(F(" "));
    if (rxTest->time.hh < 10) Serial.print(F("0")); Serial.print(rxTest->time.hh); // Print the hour. Add a leading zero if required
    Serial.print(F(":"));
    if (rxTest->time.mm < 10) Serial.print(F("0")); Serial.print(rxTest->time.mm); // Print the minute. Add a leading zero if required
    Serial.print(F(":"));
    if (rxTest->time.ss < 10) Serial.print(F("0")); Serial.print(rxTest->time.ss); // Print the second. Add a leading zero if required
    Serial.print(F("  sat_id: 0x"));
    Serial.println(rxTest->sat_id, HEX);
  }
}

void logSwarm() {
//  File f = LittleFS.open("/swarmlog.csv", "a");
//  Swarm_M138_DateTimeData_t *dateTime = new Swarm_M138_DateTimeData_t;
//  swarm.getDateTime(dateTime);
  Swarm_M138_Receive_Test_t *rxTest = new Swarm_M138_Receive_Test_t;
  Swarm_M138_Error_e err = swarm.getReceiveTest(rxTest);
  printSwarmError(err);
  printRxTest(rxTest);
//  delete dateTime;
  delete rxTest;
}

void txAprs() {
  prevAprsTx = millis();
  Swarm_M138_GPS_Fix_Quality_t *gpsQuality = new Swarm_M138_GPS_Fix_Quality_t;
  stallForGps(gpsQuality, aprsGpsStall);
  //  if(gpsQuality->fix_type != SWARM_M138_GPS_FIX_TYPE_TT && gpsQuality->fix_type != SWARM_M138_GPS_FIX_TYPE_INVALID && gpsQuality->fix_type != SWARM_M138_GPS_FIX_TYPE_NF) {
  Swarm_M138_GeospatialData_t *info = new Swarm_M138_GeospatialData_t;
  swarm.getGeospatialInfo(info);
  uint32_t startPacket = millis();
  sendPacket(info);
  uint32_t packetDuration = millis() - startPacket;
  Serial.println("Packet sent in: " + String(packetDuration) + " ms");
  delete info;
  //  }
  delete gpsQuality;
}

void setup() {
  initLed();
  setLed(true);
  Serial.begin(115200);
  delay(5000);

  // Initialize DRA818V
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

  // Initialize Swarm
  Serial.println("Configuring swarm...");
  Serial1.setTX(0);
  Serial1.setRX(1);
  swarm.begin(Serial1);
  while (!swarm.begin(Serial1)) {
    Serial.println(
      F("Could not communicate with the modem. Please check the serial "
        "connections. Freezing..."));
    delay(1200);
  }
  
//  swarm.setDateTimeRate(0);
//  swarm.setGpsJammingIndicationRate(0);
//  swarm.setGeospatialInfoRate(0);
//  swarm.setGpsFixQualityRate(0);
//  swarm.setPowerStatusRate(0);
//  swarm.setReceiveTestRate(0);
//  swarm.setMessageNotifications(false);
  swarmGpsStall = false; // uncomment ONLY if expecting 0 GPS signal

  // Initialize littleFS
  LittleFS.setConfig(cfg);
  LittleFS.begin();
  // Turn off LED, we're done setting up
  setLed(false);
  Serial.println("Setup complete");

  // Debug statements from Rohan@SWARM
//  Serial1.println("$RT 1*17");
//  Serial1.println("$MT C=U*12");
//  Serial1.println("$TD \"Test 1\"*17");
//  Serial1.println("$TD \"Test 2\"*14");
//  Serial1.println("$TD \"Test 3\"*15");

  // Start off by queueing Swarm and transmitting APRS at once
  txSwarm();
  txAprs();
}

void loop() {
  // Update LED state
  if (ledState) {
    if (millis() - prevLedTime >= ledOn) {
      digitalWrite(ledPin, false);
      ledState = false;
//      logSwarm();
    }
  }

  if (millis() - prevLogTime >= logInterval) {
    prevLogTime = millis();
//    logSwarm();
  }

  // Transmit APRS
  if (millis() + random(50) - prevAprsTx >= aprsInterval) {
    txAprs();
  }

  // Queue Swarm
//  if (millis() - prevSwarmQueue >= swarmInterval) {
//    txSwarm();
//  }

}
