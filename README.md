# Strange Returns - CMake, JUCE 6.1.6, ElkPi
This is a fork of [JackWithOneEye's Strange Return](https://github.com/JackWithOneEye/StrangeReturns), a JUCE-based VST3 Delay/Echo,
featuring a variable delay time with modulation, feedback, optional tape simulation, bit crushing, and a low-pass filter.

I forked this repo to adapt it to the ElkPi platform, using CMake and a patched JUCE 6.1.6.
My project is to build a dub sound system control tower multi effect module, 
with a Raspberry Pi 4 running Elk Audio OS and a hifiberry DAC+ ADC Pro, which will use this plugin among others.

I might or might not add features to this plugin, depending on my needs and time.
I would like to implement a more flexible time control, with a tap tempo button,
a tempo pot and a beat multiply pot, like the benidub delay has.

Please feel free to contribute by opening a PR or an issue.

# Building the plugin

I left the original .jucer file in the repo, but I will not maintain it, as I am using CMake to build the plugin.
The CMakeLists.txt is based on the AudioPluginExample from JUCE 6.1.6, with some modifications to make it work with the
StrangeReturns plugin.

## Prerequisites
- CMake 3.15 or newer
- OPTIONAL - A VST3 host to test the plugin (like Ableton, FL Studio, Pro Tools, Carla, Ardour, Reaper, etc.)

## Building steps (using CLion)
1. Clone the repository
2. Open the repository folder in CLion
3. Load the CMake project if not done automatically
4. Select the StrangeReturns_VST3 target
5. Build the project
6. The plugin will be in the `cmake-build-debug/StrangeReturns_artefacts/Debug/VST3` folder
7. Copy the `.vst3` file to your VST3 plugins folder
8. Open your VST3 host and check if the plugin is available
9. Enjoy!

## Building steps (Windows)
1. Clone the repository
2. Open a terminal and navigate to the repository folder
3. Create a build directory: `mkdir build && cd build`
4. Run CMake: `cmake .. -G "Visual Studio 16 2019" -A x64`
5. Open the generated solution file in Visual Studio
6. Build the StrangeReturns_VST3 project in Release mode
7. The plugin will be in the `build\StrangeReturns_artefacts\Release\VST3` folder
8. Copy the `.vst3` file to your VST3 plugins folder
9. Open your VST3 host and check if the plugin is available
10. Enjoy!

## Building steps (Linux, macOS)
1. Clone the repository
2. Open a terminal and navigate to the repository folder
3. Create a build directory: `mkdir build && cd build`
4. Run CMake: `cmake ..`
5. Build the project: `make -j $(nproc)` or `cmake --build . --target StrangeReturns_VST3 --config Release`
6. The plugin will be in the `build/StrangeReturns_artefacts/Debug/VST3` folder
7. Copy the `.vst3` file to your VST3 plugins folder
8. Open your VST3 host and check if the plugin is available
9. Enjoy!


# CrossBuilding for ElkPi

This will only work with JUCE 6, and the juceaide CMakeLists.txt needs to get rid of all the cross-compile unset(xxx)
because sourcing the SDK will set all the needed stuff and the unset(xxx) calls will override the SDK thus making the
build fail

[from the original ELK docs](https://elk-audio.github.io/elk-docs/html/documents/building_plugins_for_elk.html#plugins-using-juce-version-6)

I personally build on a WSL2 Ubuntu 22.04.4, but it should work on any linux machine.

## install SDK on a linux machine  (first time only)

Get the Elk cross-build SDK from there and run it to install (ideally keep the default destinations) :
  https://github.com/elk-audio/elkpi-sdk/releases

## Build

With the Elk crossbuild SDK installed, cd to the repo folder, then run these commands in the following order :

- ```mkdir x-build && cd x-build```

- ```unset LD_LIBRARY_PATH && source /opt/elk/1.0.0/environment-setup-cortexa72-elk-linux```

- ```export CXXFLAGS="-O3 -pipe -ffast-math -feliminate-unused-debug-types -funroll-loops"```

- ```cmake ../ -DCMAKE_BUILD_TYPE=Release```

- ```AR=aarch64-elk-linux-ar make -j `nproc` CONFIG=Release CFLAGS="-Wno-psabi" TARGET_ARCH="-mcpu=cortex-a72 -mtune=cortex-a72"```

The resulting x-build/StrangeReturns_artefacts/Debug/VST3/Audio Plugin Example.vst3 is ready to use on your Elk-Pi device.
