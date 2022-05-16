//
// Created by Louis Adamian on 4/5/22.
//
#include "Swarm.hh"
#include <ctime>
Swarm::Swarm(HardwareSerial *serial) {

}

Swarm_M138_GeospatialData_t Swarm::getGpsData() {

}

uint32_t Swarm::swarmTimetoEpoch(Swarm_M138_DateTimeData_t *swarmTime) {
  struct tm t = {0};
  t.tm_year = swarmTime->YYYY - 1900;
  t.tm_mon = swarmTime->MM - 1;
  t.tm_mday = swarmTime->DD;
  t.tm_hour = swarmTime->hh;
  t.tm_min = swarmTime->mm;
  t.tm_sec = swarmTime->mm;
  return (uint32_t)mktime(&t);
}
