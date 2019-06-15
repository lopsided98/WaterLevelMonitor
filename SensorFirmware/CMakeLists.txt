cmake_minimum_required(VERSION 3.8.2)

set(ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb)

set(BOARD nrf51_ble400)
set(CONF_FILE prf.conf)
set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/flash.overlay)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(SensorFirmware)

#add_subdirectory(src)

target_sources(app PRIVATE
        src/main.c
        src/bluetooth.c
        src/battery.c
        src/temperature.c
        src/watchdog.c
        src/jsn_sr04t/jsn_sr04t.c)

#target_link_libraries_ifdef(CONFIG_JSN_SR04T app PRIVATE jsn_sr04t)
