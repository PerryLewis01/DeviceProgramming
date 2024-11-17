cmake_minimum_required(VERSION 3.12)

add_executable(button_master_module
    button_master_module.cpp
)

target_link_libraries(button_master_module pico_stdlib)

pico_enable_stdio_usb(button_master_module 1)
pico_enable_stdio_uart(button_master_module 0)

pico_add_extra_outputs(button_master_module)