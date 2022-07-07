#!/bin/bash

basedir="/home/ceti/CETI/whale-tag-recovery"
builddir="${basedir}/build"

cd $builddir

d2200s=(7 8 9 10 11 12 13 14 15)
d1200s=(18 19 20 21 22 23 24 25 26 27)

for d in ${d1200s[@]}; do
    for e in ${d2200s[@]}; do
        sed -i "33s/=.*/= ${d};/" ../src/aprs.c
	sed -i "34s/=.*/= ${e};/" ../src/aprs.c
	sed -i "18s/0 .*/0 $(($e % 10))-$(($d % 10))\";/" ../src/sdk_conn.c
	sed -n -e 33,34p ../src/aprs.c
	sed -n -e 18p ../src/sdk_conn.c

	make
	sudo picotool load -xv sdk_conn.uf2 -f
	sleep 1m 10s
    done
done
