cmake_minimum_required(VERSION 3.15)

project(StrangeReturns VERSION 0.0.1)

add_subdirectory(JUCE) # Use our patched JUCE 6.1.6, which skips the unset(xxx) calls

juce_add_plugin(StrangeReturns
    COMPANY_NAME Juce-Dev                       # Specify the name of the plugin's author (I put Juce-Dev to have my dev VST grouped together)
    IS_SYNTH FALSE                              # Is this a synth or an effect?
    NEEDS_MIDI_INPUT FALSE                      # Does the plugin need midi input?
    NEEDS_MIDI_OUTPUT FALSE                     # Does the plugin need midi output?
    IS_MIDI_EFFECT FALSE                        # Is this plugin a MIDI effect?
    COPY_PLUGIN_AFTER_BUILD TRUE        # Should the plugin be installed to a default location after building?
    PLUGIN_MANUFACTURER_CODE Juce               # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE Stgr                            # A unique four-character plugin id with exactly one upper-case character
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
    FORMATS VST3 Standalone                     # The formats to build. Other valid formats are: AAX Unity VST AU AUv3
    PRODUCT_NAME "StrangeReturns")              # The name of the final executable, which can differ from the target name

file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h")
target_sources(StrangeReturns INTERFACE ${SourceFiles})

# binary data seems to break the crossbuild because it needs libfreetype6 at build time (lmk if you know how to fix this)
# Uncomment this if you want to use binary data, then just add your assets to <repo>/Resources/,
#add #include "BinaryData.h" in the relevant source files, and run a build.

#file(GLOB_RECURSE AssetFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*")
#juce_add_binary_data(Assets SOURCES ${AssetFiles})

target_compile_definitions(StrangeReturns
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(StrangeReturns
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# -- Assuming a WSL2 setup with ELK toolchain installed and its rootfs mounted to Z:/
# -- Converts Windows paths "Z:/home/..." to linux "/home/..."
set(SRCDIR_WSL "${CMAKE_SOURCE_DIR}")
if (SRCDIR_WSL MATCHES "^Z:/")
    string(REPLACE "Z:/" "/" SRCDIR_WSL "${SRCDIR_WSL}")
endif()

# --- ELK: configure (WSL)
add_custom_target(elk_configure
        COMMAND wsl.exe --cd ${SRCDIR_WSL} bash -lc "set -euo pipefail; \
    mkdir -p x-build && cd x-build; \
    unset LD_LIBRARY_PATH && source /opt/elk/1.0.0/environment-setup-cortexa72-elk-linux; \
    export CXXFLAGS='-O3 -pipe -ffast-math -feliminate-unused-debug-types -funroll-loops'; \
    cmake ../ -DCMAKE_BUILD_TYPE=Release"
        COMMENT "ELK: configure crossbuild (WSL)"
        VERBATIM)

# --- ELK: build (WSL)
add_custom_target(elk_build
        COMMAND wsl.exe --cd ${SRCDIR_WSL}/x-build bash -lc "set -euo pipefail; \
    AR=aarch64-elk-linux-ar make -j \`nproc\` CONFIG=Release CFLAGS='-Wno-psabi' TARGET_ARCH='-mcpu=cortex-a72 -mtune=cortex-a72'"
        DEPENDS elk_configure
        COMMENT "ELK: make -j \`nproc\` (WSL)"
        VERBATIM)

# --- ELK: deploy .so (WSL)
add_custom_target(elk_deploy
        COMMAND wsl.exe bash -lc "set -euo pipefail; \
    ssh mind@192.168.1.136 'mkdir -p /home/mind/plugins/${CMAKE_PROJECT_NAME}.vst3/Contents/aarch64-linux'; \
    echo 'mkdir ok' ;\
    scp '${SRCDIR_WSL}/x-build/${CMAKE_PROJECT_NAME}_artefacts/Release/VST3/${CMAKE_PROJECT_NAME}.vst3/Contents/aarch64-linux/${CMAKE_PROJECT_NAME}.so' \
        mind@192.168.1.136:/home/mind/plugins/${CMAKE_PROJECT_NAME}.vst3/Contents/aarch64-linux/ ;
    echo 'scp ok'"
        COMMENT "ELK: deploy ${CMAKE_PROJECT_NAME}.so"
        VERBATIM)
