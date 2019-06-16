cmake_minimum_required(VERSION 3.13.1)

set(ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb)

set(BOARD nrf51_ble400)
set(CONF_FILE prj.conf)
set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/sensor.overlay)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(SensorFirmware)

add_subdirectory(src)

# target_link_libraries_ifdef(CONFIG_JSN_SR04T app PUBLIC jsn_sr04t)
