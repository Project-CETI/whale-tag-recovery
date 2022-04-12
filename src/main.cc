#include <Arduino.h>
#include <SparkFun_Swarm_Satellite_Arduino_Library.h>
#include "APRS.hh"
#include "Swarm.hh"
#define swarmSerial serial1
#define aprsSerial Serial3
#define tagSerial Serial2

enum state_t : uint8_t {in_setup=0, pre_application=1, sleep=2, transmitting=3, recovery=4};
SWARM_M138 swarm;
APRS aprs;

// current GPS position and signal validity
// valid: false if there are less than 4 GPS satellites available
// lat: current latitude in decimal degrees where west is positive and east is negative
// lon: current longitude in decimal degrees where north is positive and south is negative
// cog: course over ground in decimal degrees
// sog: speed over ground in kilometers per hour
struct GpsInfo{
    bool valid = False;
    float lat;
    float lon;
    float cog;
    float: sog;
};

struct swarmTxPacket{

};

void setup() {

}

void loop() {

}

GpsInfo getGPSinfo(){
    GpsInfo info;
    Swarm_M138_GPS_Fix_Quality_t *quality = new Swarm_M138_GPS_Fix_Quality_t;
    swarm.getGpsFixQuality(quality);
        if (quality->gnss_sats <= 4) {
            info.lat = 0;
            info.lon = 0;
            info.valid = false;
        } else {
        Swarm_M138_GeospatialData_t *gsData = new Swarm_M138_GeospatialData_t;
        swarm.getGeospatialInfo(gsData);
        info.lat = gsData.lat;
        info.lon = gsData.lon;
        info.lat = gsData.
        info.valid = true;
        }
    }

void txAprs(){

}

uint8_t init(){

}

uint8_t dive(){
/*
 * Does nothing until interrupted at surface
 *
 */
}

uint8_t breach_transmit(){
    /*
     *
     */

}

uint8_t breachLoop(){


}

uint8_t recovery(){


}

uint8_t tx_swarm(){

}


