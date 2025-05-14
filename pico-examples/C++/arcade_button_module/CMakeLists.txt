cmake_minimum_required(VERSION 3.12)

add_executable(arcade_button_module
    arcade_button_module.cpp
)

target_link_libraries(arcade_button_module pico_stdlib)

pico_enable_stdio_usb(arcade_button_module 1)
pico_enable_stdio_uart(arcade_button_module 0)

pico_add_extra_outputs(arcade_button_module)