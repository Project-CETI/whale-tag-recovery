#!/bin/bash
set -e

trap 'last_command=$current_command; current_command=$BASH_COMMAND' DEBUG
trap 'echo "Exiting on \"${last_command}\""' EXIT

ug="Usage: ./build_tags.sh [TAGS]\n\nREQUIRED TAGS:\n\n"

un="\t-n -> Bin name:\n\t--------\n\tString holding name of the binary being compiled for multiple tags\n\n"

u="$ug$un\n"

usage() { printf "$u" 1>&2; exit 1; }

while getopts ":n:" o; do
		case "${o}" in
				n)
						n=${OPTARG}
						;;
				*)
					  usage
						;;
		esac
done
shift $((OPTIND-1))

if [ -z "${n}" ]; then
		usage
		exit 1
else
		uf2name=$n
fi

cd "$(dirname "$0")"
rm -rf build
mkdir -p build
cmake -S src/ -B build/
cd build

tagIDs=(1 2 3)

sed -i "9s/NAME .*/NAME ${uf2name})/" ../src/CMakeLists.txt

for i in ${tagIDs[@]}; do
		echo "************** TAG ${i} **************"
		sed -i "16s/= .*/= ${i};/" ../src/main.c
		sed -i "20s/b1.0 .*/b1.0 Tag${i}\";/" ../src/main.c
		sed -i "44s/for .*/for Tag ${i}\"));/" ../src/main.c

		make -j4
		rm -f "../bin/tag${i}/${uf2name}.uf2"
		cp "${uf2name}.uf2" "../bin/tag${i}"
done

echo "************** RESET TO STANDALONE BOARD **************"
sed -i "16s/= .*/= 10;/" ../src/main.c
sed -i "20s/b1.0 .*/b1.0 2-#\";/" ../src/main.c
sed -i "44s/for .*/for standalone recovery board 2-#\"));/" ../src/main.c

make -j4
