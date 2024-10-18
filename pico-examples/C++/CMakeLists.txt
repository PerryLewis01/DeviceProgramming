cmake_minimum_required(VERSION 3.12)

include ($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

pico_sdk_init()

project(pico-projects CXX C)

file (GLOB SUBFOLDERS *)
foreach(SUBFOLDER ${SUBFOLDERS})
    message(STATUS "${SUBFOLDER}")
    if ((IS_DIRECTORY ${SUBFOLDER}) AND ( NOT ${SUBFOLDER} MATCHES "build"))
        add_subdirectory(${SUBFOLDER})
    endif()
endforeach()

