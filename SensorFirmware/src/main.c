/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include <sensor.h>
#include "bluetooth.h"
#include "battery.h"

#define LED_PORT LED0_GPIO_CONTROLLER
#define LED    LED0_GPIO_PIN

#define SLEEP_TIME 100


void main(void) {
    int err = 0;
    int cnt = 0;
    struct device* gpio;
    struct device* rangefinder;

    bluetooth_init();
    battery_init();

    gpio = device_get_binding(LED_PORT);
    __ASSERT(gpio, "Failed to initialize GPIO");

    /* Set LED pin as output */
    gpio_pin_configure(gpio, LED, GPIO_DIR_OUT);

    rangefinder = device_get_binding(CONFIG_JSN_SR04T_NAME);
    __ASSERT(rangefinder, "Failed to initialize rangefinder");

    struct sensor_value distance;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (1) {
        /* Set pin to HIGH/LOW every 1 second */
        gpio_pin_write(gpio, LED, cnt % 2);
        cnt++;

        err = sensor_sample_fetch(rangefinder);
        if (!err) {
            sensor_channel_get(rangefinder, SENSOR_CHAN_DISTANCE, &distance);
            printk("Distance: %d mm\n", distance.val1 * 1000 + distance.val2 / 1000);
        }

        k_sleep(SLEEP_TIME);
    }
#pragma clang diagnostic pop
}