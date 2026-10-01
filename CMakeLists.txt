cmake_minimum_required(VERSION 3.20)
project(PrankApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------------------
# Embed every .wav under assets/sounds/ into a generated C++ header.
# Paths are always resolved relative to the project root (CMAKE_SOURCE_DIR),
# never as absolute filesystem-root paths.
# ---------------------------------------------------------------------------
file(GLOB SOUND_WAV_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/assets/sounds/*.wav")

if(NOT SOUND_WAV_FILES)
    message(WARNING "No .wav files found in ${CMAKE_SOURCE_DIR}/assets/sounds/ - "
                     "the app will build but has no sound to play. Add at least one .wav there.")
endif()

set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set(GENERATED_HEADER "${GENERATED_DIR}/sound_data.h")

# Join the file list with '|' since ';' would be re-split by CMake's -D argument
# parsing before it ever reaches the script.
string(REPLACE ";" "|" SOUND_WAV_FILES_JOINED "${SOUND_WAV_FILES}")

add_custom_command(
    OUTPUT "${GENERATED_HEADER}"
    COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILES=${SOUND_WAV_FILES_JOINED}
            -DOUTPUT_FILE=${GENERATED_HEADER}
            -P "${CMAKE_SOURCE_DIR}/cmake/bin2h.cmake"
    DEPENDS ${SOUND_WAV_FILES} "${CMAKE_SOURCE_DIR}/cmake/bin2h.cmake"
    COMMENT "Embedding assets/sounds/*.wav -> generated/sound_data.h"
    VERBATIM)

add_custom_target(GenerateSoundHeader DEPENDS "${GENERATED_HEADER}")

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(COMMON_SOURCES
    src/main.cpp
    src/scheduler.cpp
    src/audio.cpp
)

if(WIN32)
    list(APPEND COMMON_SOURCES
        src/volume/volume_windows.cpp
        src/autostart/autostart_windows.cpp
        src/platform/self_delete_windows.cpp
        src/instance/single_instance_windows.cpp)
elseif(APPLE)
    list(APPEND COMMON_SOURCES
        src/volume/volume_macos.cpp
        src/autostart/autostart_macos.cpp
        src/platform/self_delete_posix.cpp
        src/instance/single_instance_posix.cpp)
else()
    list(APPEND COMMON_SOURCES
        src/volume/volume_linux.cpp
        src/autostart/autostart_linux.cpp
        src/platform/self_delete_posix.cpp
        src/instance/single_instance_posix.cpp)
endif()

add_executable(PrankApp WIN32 ${COMMON_SOURCES})

add_dependencies(PrankApp GenerateSoundHeader)

target_include_directories(PrankApp PRIVATE
    ${GENERATED_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/src
)

if(WIN32)
    target_link_libraries(PrankApp PRIVATE ole32 shell32 uuid)
    if(MSVC)
        target_link_options(PrankApp PRIVATE /ENTRY:mainCRTStartup)
    endif()
elseif(APPLE)
    target_link_libraries(PrankApp PRIVATE
        "-framework CoreFoundation"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework AudioUnit")
else()
    find_package(Threads REQUIRED)
    target_link_libraries(PrankApp PRIVATE Threads::Threads dl m)
endif()
