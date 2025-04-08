cmake_minimum_required(VERSION 3.12)

# Set the path to the Pico SDK
set(PICO_SDK_PATH "C:/Users/eogha/oled-test/pico-sdk")

# Initialize the Pico SDK
include(pico_sdk_import.cmake)

project(oled_test C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


pico_sdk_init()

add_executable(oled_test
    oled_test.cpp
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(oled_test 
    pico_stdlib
    hardware_spi
    hardware_gpio
    pico_binary_info
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(oled_test 1)
pico_enable_stdio_uart(oled_test 0)

# Create map/bin/hex/uf2 file in addition to ELF file
pico_add_extra_outputs(oled_test)