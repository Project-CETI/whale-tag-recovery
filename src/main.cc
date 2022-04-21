#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <SparkFun_Swarm_Satellite_Arduino_Library.h>
#include "APRS.hh"
#include "Swarm.hh"
#include "TagRecoveryBoardv0_9.h"

enum recovery_board_state_t : uint8_t {in_setup=0, pre_application=1, sleep=2, transmitting=3, recovery=4};
SWARM_M138 swarm;

APRS aprs("KC1QXQ");
char comment[] = " v0.9 https://projectceti.org";

struct swarmTxPacket{

};

void setup() {
    SerialPIO vhfSerial = SerialPIO(dra818vUartRxPin, dra818vUartTxPin);
    APRS::configDra818v();

}

void loop() {

}


void txAprs(){

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

uint8_t txSwarm(){
    return 0;
}


