//
// Created by Louis Adamian on 3/23/22.
//
#include <Arduino.h>
#ifndef CETI_TAG_TRACKING_R2RDAC_H
#define CETI_TAG_TRACKING_R2RDAC_H
class R2rDac{
    uint8_t pins[8];
    R2rDac(uint8_t pin0, uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4, uint8_t pin5, uint8_t pin6, uint8_t pin7){
        pins[0] = pin0;
        pins[1] = pin1;
        pins[2] = pin2;
        pins[3] = pin4;
        pins[4] = pin4;
        pins[5] = pin5;
        pins[6] = pin6;
        pins[7] = pin7;
        for (uint8_t i =0; i<8; i++) {
            pinMode(pins[i], OUTPUT);
        }
    }
    uint8_t write(uint8_t value){
        
    };
};

#endif //CETI_TAG_TRACKING_R2RDAC_H


