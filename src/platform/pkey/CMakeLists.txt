# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(tooldev C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(tooldev main.cpp usb_descriptors.c ../../core/CayEngine.cpp ../../core/CayData.cpp)

pico_set_program_name(tooldev "tooldev")
pico_set_program_version(tooldev "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(tooldev 1)
pico_enable_stdio_usb(tooldev 0)

# Add the standard library to the build
add_subdirectory(pico-pio-usb)

target_sources(tooldev PRIVATE
    ${PICO_SDK_PATH}/lib/tinyusb/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

target_link_libraries(tooldev
        pico_stdlib
        tinyusb_device
        tinyusb_host
        tinyusb_board
        pico_pio_usb)

# Add the standard include files to the build
target_include_directories(tooldev PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/../../core
        ${CMAKE_CURRENT_LIST_DIR}/pico-pio-usb/src
)

pico_add_extra_outputs(tooldev)

