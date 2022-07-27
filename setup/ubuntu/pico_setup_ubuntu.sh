#!/bin/bash
# install all the dependencies
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential minicom libstdc++-arm-none-eabi-newlib pkgconfig libusb-1.0-0-dev

# clone all the base repos
git clone https://github.com/raspberrypi/pico-sdk.git
git clone https://github.com/raspberrypi/pico-examples.git
git clone https://github.com/raspberrypi/pico-extras.git
git clone https://github.com/raspberrypi/pico-playground.git

dir=$PWD

# Update these paths to match where you've checked out the repos
echo "export PICO_SDK_PATH='${dir}/pico-sdk'" >> ~/.bashrc
echo "export PICO_EXTRAS_PATH='${dir}/pico-extras'" >> ~/.bashrc

# read in the env variables we just set
export PICO_SDK_PATH='${dir}/pico-sdk'
export PICO_EXTRAS_PATH='${dir}/pico-extras'

# pull in the git submodule dependencies in the pico-sdk and pico-extras projects
cd pico-sdk
git submodule update --init
cd ..

cd pico-extras
git submodule update --init
cd ..

# Optional, haven't tried these yet
# git clone https://github.com/raspberrypi/picoprobe.git
git clone -b master https://github.com/raspberrypi/picotool.git
cd picotool/
mkdir build
cd build
cmake ../
make
dir=$PWD
echo "alias picotool='${dir}/picotool'" >> ~/.bashrc
unalias sudo 2>/dev/null
echo "alias sudo='sudo '" >> ~/.bashrc
source ~/.bashrc

# example for how to build from command prompt:
cd ../../
cd pico-examples
mkdir build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Debug

cd blink
make -j4 # you can update this 4 to be how many CPU cores you want to build on
