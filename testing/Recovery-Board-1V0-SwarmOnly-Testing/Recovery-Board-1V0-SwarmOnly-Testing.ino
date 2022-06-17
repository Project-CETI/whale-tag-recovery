#include <SparkFun_Swarm_Satellite_Arduino_Library.h>
#include <math.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include <time.h>

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
uint32_t swarmInterval = 300000; // swarm Tx update interval in ms
long logInterval = 60000;

// swarm bools
bool swarmGpsStall = true; // Stall for GPS signal before sending Swarm (keep true, alter in setup)
bool pStatSwarm = false; // Provide additional Serial output beyond the modem's replies

// log extra(s)
long prevLogTime = 0;
uint64_t prevSwarmQueue = 0;
bool ledState = true;
uint64_t prevLedTime = 0;
uint32_t ledOn = 0;

SWARM_M138 swarm;
uint16_t qCount;

void setLed(bool state) {
  digitalWrite(ledPin, state);
}

void initLed() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, false);
}

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

void stallForGps(Swarm_M138_GPS_Fix_Quality_t *gpsQuality, bool stall, bool pStat) {
  swarm.getGpsFixQuality(gpsQuality);
  if (pStat) {
    Serial.print("fix quality = ");
    Serial.print(gpsQuality->fix_type);
  }
  if (stall) {
    while (gpsQuality->fix_type == SWARM_M138_GPS_FIX_TYPE_TT || gpsQuality->fix_type == SWARM_M138_GPS_FIX_TYPE_INVALID || gpsQuality->fix_type == SWARM_M138_GPS_FIX_TYPE_NF) {
      delay(1000);
      swarm.getGpsFixQuality(gpsQuality);
      if (pStat) {
        Serial.print(" ... ");
        Serial.print(gpsQuality->fix_type);
      }
    }
  }
  else if (!stall && pStat && (gpsQuality->fix_type == SWARM_M138_GPS_FIX_TYPE_TT || gpsQuality->fix_type == SWARM_M138_GPS_FIX_TYPE_INVALID || gpsQuality->fix_type == SWARM_M138_GPS_FIX_TYPE_NF)) {
    Serial.print(" ... No GPS.");
  }
  if (pStat) {
    Serial.println();
  }
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
  if (qCount >= 1024) {
    Serial.println("Clearing SWARM TX queue ...");
    swarm.deleteAllTxMessages();
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

void txSwarm(bool pStat) {
  Swarm_M138_GPS_Fix_Quality_t *gpsQuality = new Swarm_M138_GPS_Fix_Quality_t;
  // START OF STATUS PRINTING
  stallForGps(gpsQuality, swarmGpsStall, pStat);
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
  if (pStat) {printSwarmTxStatus(message, err);}
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

void logSwarm(bool pStat) {
//  File f = LittleFS.open("/swarmlog.csv", "a");
//  Swarm_M138_DateTimeData_t *dateTime = new Swarm_M138_DateTimeData_t;
//  swarm.getDateTime(dateTime);
  Swarm_M138_Receive_Test_t *rxTest = new Swarm_M138_Receive_Test_t;
  Swarm_M138_Error_e err = swarm.getReceiveTest(rxTest);
  if (pStat) {
    printSwarmError(err);
    printRxTest(rxTest);
  }
//  delete dateTime;
  delete rxTest;
}

void setup() {
  initLed();
  setLed(true);
  Serial.begin(115200);
  delay(5000);

  // Initialize DAC
  Serial.println("Configuring DAC...");
  initializeOutput();
  Serial.println("DAC configured");

  // Initialize Swarm
  Serial.println("Configuring swarm...");
  swarm.enableDebugging();
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
//  pStatSwarm = true; // uncomment ONLY if want additional serial output (aside from debug msgs)

  // Turn off LED, we're done setting up
  setLed(false);
  Serial.println("Setup complete");

  // Optionally start off by queueing Swarm once
  txSwarm(pStatSwarm);
  // Debug statements from Rohan@SWARM
  Serial1.println("$RT 1*17");
  Serial1.println("$MT C=U*12");
  Serial1.println("$TD \"Test 1\"*17");
  Serial1.println("$TD \"Test 2\"*14");
  Serial1.println("$TD \"Test 3\"*15");
}

void loop() {
  // Update LED state
  if (ledState) {
    if (millis() - prevLedTime >= ledOn) {
      digitalWrite(ledPin, false);
      ledState = false;
    }
  }

  if (millis() - prevLogTime >= logInterval) {
    prevLogTime = millis();
    logSwarm(pStatSwarm);
  }

  // Queue Swarm
  if (millis() - prevSwarmQueue >= swarmInterval) {
    txSwarm(pStatSwarm);
  }
  swarm.checkUnsolicitedMsg();
}
