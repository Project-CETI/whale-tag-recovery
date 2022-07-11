#!/bin/bash
set -e
trap 'echo "This script is customized for a RPi formatted for CETI use. Change it to fit the current system so it does not error again."' EXIT

basedir="/home/ceti/CETI/whale-tag-recovery"
builddir="${basedir}/build"
cd basedir
mkdir -p build

cd $builddir

d1200s=(18 19 20 21 22 23 24 25 26 27)
d2200s=(7 8 9 10 11 12 13 14 15)

for d in ${d1200s[@]}; do
    for e in ${d2200s[@]}; do
        sed -i "33s/=.*/= ${d};/" ../src/aprs.c
				sed -i "34s/=.*/= ${e};/" ../src/aprs.c
				sed -i "18s/0 .*/0 $(($e % 10))-$(($d % 10))\";/" ../src/main.c
				sed -n -e 33,34p ../src/aprs.c
				sed -n -e 18p ../src/main.c

				make -j4
				sudo picotool load -xv recovery.uf2 -f
				sleep 1m 10s
    done
done
