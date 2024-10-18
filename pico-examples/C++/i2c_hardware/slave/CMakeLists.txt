cmake_minimum_required(VERSION 3.12)

add_executable(i2c_hardware_slave
    i2c_hardware_slave.c
)

target_link_libraries(i2c_hardware_slave pico_i2c_slave pico_stdlib hardware_i2c)

pico_enable_stdio_usb(i2c_hardware_slave 1)
pico_enable_stdio_uart(i2c_hardware_slave 0)

pico_add_extra_outputs(i2c_hardware_slave)