#!/bin/zsh
set -e
trap 'echo "This script is customized for a RPi formatted for CETI use. Change it to fit the current system so it does not error again."' EXIT

basedir="/home/ceti/CETI/whale-tag-recovery"
builddir="${basedir}/build"
cd ${basedir}
cmake -B build/
cd build

d1200s=(18 19 20 21 22 23 24 25 26 27)
d2200s=(7 8 9 10 11 12 13 14 15)

for d in ${d1200s[@]}; do
    for e in ${d2200s[@]}; do
        sed -i "39s/832.*/832, ${d}, ${e}, 0};/" ../recovery/aprs.c
				sed -i "36s/b1.1 .*/b1.1 $(($e % 10))-$(($d % 10))\",/" ../recovery/main.c
				sed -n -e 39p ../recovery/aprs.c
				sed -n -e 35p ../recovery/main.c

			  make -j4
			  picotool load -xv recovery/recovery.uf2 -f
				sleep 1m 10s
    done
done
