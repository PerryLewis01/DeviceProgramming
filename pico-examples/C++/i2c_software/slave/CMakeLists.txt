cmake_minimum_required(VERSION 3.12)

add_executable(i2c_software_slave
    i2c_software_slave.cpp
    i2c_software_slave_lib.h
    i2c_software_slave_lib.cpp
)

target_link_libraries(i2c_software_slave pico_stdlib)

pico_enable_stdio_usb(i2c_software_slave 1)
pico_enable_stdio_uart(i2c_software_slave 0)

pico_add_extra_outputs(i2c_software_slave)