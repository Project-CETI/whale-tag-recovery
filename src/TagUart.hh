//
// Created by Louis Adamian on 5/16/22.
//
#include <pico.h>
#ifndef WHALE_TAG_RECOVERY_SRC_TAGUART_HH_
#define WHALE_TAG_RECOVERY_SRC_TAGUART_HH_
#include <SparkFun_Swarm_Satellite_Arduino_Library.h>

class TagUart {
  TagUart(uint8_t txPin, uint8_t rxPin);
  void receiveInterupt();
  uint8_t sendDetach();
  uint8_t sendGpsTime();
  void wake();
  uint8_t parseStatus(char* status);
  int8_t parseReceive(char* buff);
};

#endif //WHALE_TAG_RECOVERY_SRC_TAGUART_HH_
