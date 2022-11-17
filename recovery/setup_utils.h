#ifndef _RECOVERY_SETUP_UTILS_H_
#define _RECOVERY_SETUP_UTILS_H_

#include "constants.h"

static void setLed(uint8_t pin, bool state) { gpio_put(pin, state); }

static void initLed(uint8_t pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
}

#endif