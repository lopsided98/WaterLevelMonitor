cmake_minimum_required(VERSION 3.13.1)

if(NOT ZEPHYR_TOOLCHAIN_VARIANT)
    set(ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb)
endif()
if(NOT BOARD)
    set(BOARD nrf51_ble400)
endif()
set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/sensor.overlay)

set(CONF_FILE common.conf)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND CONF_FILE debug.conf)
else()
    list(APPEND CONF_FILE release.conf)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(SensorFirmware)

add_subdirectory(src)

#target_link_libraries_ifdef(CONFIG_JSN_SR04T app PUBLIC jsn_sr04t)
