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

set(MCUBOOT_BASE $ENV{MCUBOOT_BASE})

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(SensorFirmware)

add_subdirectory(src)

#target_link_libraries_ifdef(CONFIG_JSN_SR04T app PUBLIC jsn_sr04t)
