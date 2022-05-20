#include "Arduino.h"
#include <SparkFun_Swarm_Satellite_Arduino_Library.h>
#include "APRS.hh"
#include "Swarm.hh"
#include "TagRecoveryBoardv0_9.h"
#include "LittleFS.h"

enum recovery_board_state_t : uint8_t {in_setup=0, sleeping=2, transmitting=3, recovery=4};
uint8_t dive_count; // dive count is updated by message from main board
recovery_board_state_t state;
SWARM_M138 swarm;
APRS aprs(APRS_CALL, APRS_SSID);
byte* swarmTxPacket;
uint8_t battery_level;
uint16_t detach;
uint32_t prevSwarmQueue = 0;
uint32_t swarmQueueInterval = 300000;
uint32_t swarmTransmitInterval = 7.2e+6;
uint32_t arpsTransmitInterval = 300000;
uint32_t prevAprsTx = 0;
LittleFSConfig cfg;

bool swarmQueuing = false;

void setup() {
    SerialPIO vhfSerial = SerialPIO(dra818vUartRxPin, dra818vUartTxPin);
    APRS::configDra818v(vhfSerial);
    cfg.setAutoFormat(false);
    LittleFS.setConfig(cfg);
}

void loop() {
  if(swarmQueuing){
    if (millis()-prevSwarmQueue()>=swarmQueueInterval){
      queueSwarm();
    }
  }
  if(millis()-LittleFSConfig>= aprsQueueInterval){
    txAprs();
  }
}


void queueSwarm(Swarm_M138_GeospatialData_t *gps, uint8_t tagId, uint8_t mode, uint8_t diveNum, uint8_t batterySoc, uint16_t detachTime){
  char message[22];
  char lat[4];
  char lon[4];
  char speed = (char)gps->speed;
  char course[2];
  char time[4];
  char id = (char)tagId;
  char mode_c = (char)mode;
  char dives = (char)diveNum;
  char bat = (char)batterySoc;
  char detachTime_c[2];
  memcpy(lat, &gps->lat, sizeof(float));
  memcpy(lon, &gps->lon, sizeof(float));
  memcpy(course, &gps->course, sizeof(uint16_t));
  Swarm_M138_DateTimeData_t* swarmTime = new Swarm_M138_DateTimeData_t;
  swarm.getDateTime(swarmTime);
  uint32_t epochTime = Swarm::swarmTimetoEpoch(swarmTime);
  memcpy(time, &epochTime, sizeof(uint32_t));
  memcpy(detachTime_c, &detachTime, sizeof(uint16_t));
}

void txAprs(){
  auto *gpsQuality = new Swarm_M138_GPS_Fix_Quality_t;
  swarm.getGpsFixQuality(gpsQuality);
  if(gpsQuality->fix_type != SWARM_M138_GPS_FIX_TYPE_NF && gpsQuality->fix_type != SWARM_M138_GPS_FIX_TYPE_TT && gpsQuality->fix_type !=SWARM_M138_GPS_FIX_TYPE_INVALID){
    auto *gps = new Swarm_M138_GeospatialData_t;
    swarm.getGeospatialInfo(gps);
    uint8_t dive_count; // get dive count from main board
    // get battery state from
    char message[6];
    message[0] = TAG_SN;
    message[1] = (char)state;
    message[3] = battery_level;
    message[4] = detach & 10;
    message[5] = detach & 01;
    aprs.sendPacket(gps, message);
    delete gps;
  }
  else{

  }
  delete gpsQuality;
}

uint8_t initTag(){
    return 0;
}

uint8_t dive(){
    /*
     * sleep and do nothing until interrupted at surface
     *
     */
    return 0;
}

uint8_t breachTransmit(){

  return 0;
}

uint8_t breachLoop(){
    return 0;
}

uint8_t recoveryLoop(){
    return 0;
}

uint8_t queueSwarmTx(){
  uint32_t currTime;
  byte currMessage[9];
  currMessage[0] = (uint8_t) state;
  currMessage[1] = dive_count;
  currMessage[2] = battery_level;
  currMessage[3] = detach & 10;
  currMessage[4] = detach & 01;
  currMessage[5] = currTime & 0xFF000000;
  currMessage[6] = currTime & 0x00FF0000;
  currMessage[7] = currTime & 0x0000FF00;
  currMessage[8] = currTime & 0x000000FF;
  uint64_t msgId;
  Swarm_M138_Error_e err = swarm.transmitBinaryExpire(currMessage,9,&msgId, currTime+600);
}


uint8_t writeGpsLog(){

}