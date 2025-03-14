# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

# Initialize the Pico SDK
include(pico_sdk_import.cmake)

# Set project name and language
project(piezo_sensor C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable
add_executable(piezo_sensor
    main.c
)

# Create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(piezo_sensor)

# Add the standard library to the build
target_link_libraries(piezo_sensor
    pico_stdlib
    hardware_adc
    hardware_dma
)

# Enable USB output for printf statements
pico_enable_stdio_usb(piezo_sensor 1)
pico_enable_stdio_uart(piezo_sensor 0)