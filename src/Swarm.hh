//
// Created by Louis Adamian on 4/5/22.
//

#ifndef CETI_TAG_TRACKING_SWARM_H
#define CETI_TAG_TRACKING_SWARM_H
#include "Arduino.h"
#include "SparkFun_Swarm_Satellite_Arduino_Library.h"
class Swarm {
private:

public:
    Swarm(HardwareSerial *serial);

    Swarm_M138_GeospatialData_t getGpsData();
};


#endif //CETI_TAG_TRACKING_SWARM_H
