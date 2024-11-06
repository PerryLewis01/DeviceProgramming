cmake_minimum_required(VERSION 3.12)

add_executable(i2c_arcade_demo_2
    i2c_arcade_demo_2.cpp
)

target_link_libraries(i2c_arcade_demo_2 pico_stdlib)

pico_enable_stdio_usb(i2c_arcade_demo_2 1)
pico_enable_stdio_uart(i2c_arcade_demo_2 0)

pico_add_extra_outputs(i2c_arcade_demo_2)