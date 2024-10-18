cmake_minimum_required(VERSION 3.12)

add_executable(i2c_software_master
    i2c_software_master.cpp
)

target_link_libraries(i2c_software_master pico_stdlib)

pico_enable_stdio_usb(i2c_software_master 1)
pico_enable_stdio_uart(i2c_software_master 0)

pico_add_extra_outputs(i2c_software_master)