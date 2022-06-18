#!/bin/bash
rtl_fm -f 144.39M -s 22050 - | multimon-ng -t raw -s AFSK1200 -A -
