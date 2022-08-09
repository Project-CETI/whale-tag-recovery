#!/bin/bash

export PICO_SDK_PATH="/pico/pico-sdk"
export PICO_EXTRAS_PATH="/pico/pico-extras"
cd /project
exec "$@"
