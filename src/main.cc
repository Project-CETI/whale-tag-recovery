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

LittleFSConfig cfg;


void setup() {
    SerialPIO vhfSerial = SerialPIO(dra818vUartRxPin, dra818vUartTxPin);
    APRS::configDra818v(vhfSerial);
    cfg.setAutoFormat(false);
    LittleFS.setConfig(cfg);
}

void loop() {

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
    /*
     *
     */
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