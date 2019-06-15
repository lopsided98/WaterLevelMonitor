#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include <sensor.h>
#include <logging/log.h>
#include <settings/settings.h>
#include "bluetooth.h"
#include "battery.h"
#include "common.h"
#include "temperature.h"

LOG_MODULE_REGISTER(main);

#define LED_PORT LED0_GPIO_CONTROLLER
#define LED LED0_GPIO_PIN

#define SLEEP_TIME 500

#define NUM_SAMPLES 10

void main(void) {
    int err = 0;
    int cnt = 0;
    struct device* gpio;
    struct device* rangefinder;

    IF_ERR (bluetooth_init()) {
        LOG_ERR("bluetooth initialization failed (err %d)", err);
    }
    battery_init();
    IF_ERR (temperature_init()) {
        LOG_ERR("temperature initialization failed (err %d)", err);
    }

    gpio = device_get_binding(LED_PORT);
    if (!gpio) {
        LOG_ERR("GPIO initialization failed");
        return;
    }

    // Set LED pin as output
    gpio_pin_configure(gpio, LED, GPIO_DIR_OUT);

    rangefinder = device_get_binding(CONFIG_JSN_SR04T_NAME);
    if (!rangefinder) {
        LOG_ERR("rangefinder initialization failed");
        return;
    }

    struct sensor_value distance;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    for (;;) {

        temperature_update();

        device_set_power_state(rangefinder, DEVICE_PM_ACTIVE_STATE, NULL, NULL);

        u32_t distance_mm_avg = 0;

        for (int i = 0; i < NUM_SAMPLES;) {
            // Set pin to HIGH/LOW every 1 second
            gpio_pin_write(gpio, LED, cnt % 2);
            ++cnt;

            err = sensor_sample_fetch(rangefinder);
            if (!err) {
                sensor_channel_get(rangefinder, SENSOR_CHAN_DISTANCE, &distance);

                ++i;
                u32_t distance_mm = (u32_t) (distance.val1 * 1000 + distance.val2 / 1000);
                distance_mm_avg += distance_mm;

                printk("Distance: %d mm\n", distance_mm);
            } else {
                LOG_ERR("failed to get rangefinder (err %d)", err);
            }

            k_sleep(50);
        };

        device_set_power_state(rangefinder, DEVICE_PM_OFF_STATE, NULL, NULL);

        distance_mm_avg /= NUM_SAMPLES;
        printk("Distance (avg): %d mm\n", distance_mm_avg);

        bluetooth_set_water_level((u16_t) distance_mm_avg);

        k_sleep(SLEEP_TIME);
    }
#pragma clang diagnostic pop
}