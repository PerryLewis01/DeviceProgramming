cmake_minimum_required(VERSION 3.12)

# include(pico_sdk_import.cmake)

# project(pico-projects)

# pico_sdk_init()

add_executable(blink-led
    blink-led.c
)

target_link_libraries(blink-led pico_stdlib)

pico_enable_stdio_usb(blink-led 1)
pico_enable_stdio_uart(blink-led 0)

pico_add_extra_outputs(blink-led)