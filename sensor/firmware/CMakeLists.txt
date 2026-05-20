# Copyright (c) 2026 Ben Wolsieffer
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.13.1)

if(NOT ZEPHYR_TOOLCHAIN_VARIANT)
    set(ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb)
endif()
if(NOT BOARD)
    set(BOARD nrf51_ble400)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(SensorFirmware LANGUAGES C)

add_subdirectory(src)
