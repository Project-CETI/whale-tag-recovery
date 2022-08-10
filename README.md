# CETI-tag-tracking
APRS and Swarm satellite based GPS tracking for whale tags


## C++ Style and formatting


# Setup
## Docker Dev Environments
### To setup Docker dev environment on Windows
First, configure Github to avoid CRLF conversions on Windows:
```
git config --global core.autocrlf input
```

ONLY then do we clone this repository:
```
git clone -b <branch> https://github.com/Project-CETI/whale-tag-recovery.git
```

Next, install Docker Desktop for Windows. This setup has been tested with the WSL-2 setup, but should hypothetically also work with the Hyper-V method. Enable Virtualization in BIOS as needed (for the Windows Surface Pro, this is already enabled and needs no extra steps).

After that, build the Docker dev environment image:
```
cd /path/to/whale-tag-recovery
cd docker/pico
docker build -t ceti:pico -f Picofile .
```

We name this `ceti:pico` to indicate that this is the dev environment for the RP2040 chip. The Dockerfile has also been renamed `Picofile` for the same reason.

After building the image, make sure you've removed any existing `build` folder, because CMake will be mad if you try building in a folder not made inside Docker.
```
cd /path/to/whale-tag-recovery
rm -r build
```

Now, set up a **persistent** container! This is critical, because in Windows, you can't use `make` commands to do the job easily for you. (You probably can, but I don't want to write it.) If you leave it as persistent, then the convenient GUI of Docker Desktop will let you have a running 'executable' that just compiles the build folder for you. To do this, run:

```
cd /path/to/whale-tag-recovery
docker run -v ${PWD}:/project --name pico-dev ceti:pico
```

The image name (final portion of the second line above) must be the image name you specified while building the image; the instructions default to `ceti:pico`. Likewise, you should name your container something informative, like `pico-dev`, because Docker has almost intentionally terrible naming conventions. Running this command should already produce a `build` sub-directory in your `whale-tag-recovery` repo, complete with built binaries. In the future, you can go to Docker Desktop, and in the Container tab, click the play button to have things recompiled as you wish. Note that in Docker Desktop you need to click the container's name, and then navigate to the `Logs` folder to see the error output from the builds.

And that's it! You now have a fully functional build environment on Windows.
