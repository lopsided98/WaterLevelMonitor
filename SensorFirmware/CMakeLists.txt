cmake_minimum_required(VERSION 3.8.2)

set(ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb)
set(GNUARMEMB_TOOLCHAIN_PATH /usr/bin)

set(ZEPHYR_BASE ${CMAKE_CURRENT_SOURCE_DIR}/../zephyr)
set(ENV{ZEPHYR_BASE} ${ZEPHYR_BASE})

set(BOARD nrf51_ble400)
set(CONF_FILE prf.conf)

include(${ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(SensorFirmware)

#add_subdirectory(src)

target_sources(app PRIVATE
        src/main.c
        src/bluetooth.c
        src/battery.c
        src/jsn_sr04t/jsn_sr04t.c)

#target_link_libraries_ifdef(CONFIG_JSN_SR04T app PRIVATE jsn_sr04t)