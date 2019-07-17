#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <settings/settings.h>
#include "bluetooth.h"
#include "battery.h"
#include "common.h"
#include "temperature.h"
#include "water_level.h"
#include "watchdog.h"

#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include <img_mgmt/img_mgmt.h>
#endif

LOG_MODULE_REGISTER(main);

#define UPDATE_PERIOD K_MINUTES(15)

K_TIMER_DEFINE(update_timer, NULL, NULL);

void update_thread(void* p1, void* p2, void* p3) {
    int err = 0;

    k_timer_start(&update_timer, UPDATE_PERIOD, UPDATE_PERIOD);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    for (;;) {

        IF_ERR(temperature_update()) {
            LOG_ERR("Failed to update temperature (err %d)", err);
            bluetooth_set_error(ERROR_TEMPERATURE);
        }

        if (!bluetooth_get_error(ERROR_BROWNOUT)) {
            IF_ERR(water_level_update()) {
                LOG_ERR("Failed to update temperature (err %d)", err);
                bluetooth_set_error(ERROR_WATER_LEVEL);
            }
        } else {
            LOG_WRN("Not updating water level due to brownout");
        }

        IF_ERR(battery_update()) {
            LOG_ERR("Failed to update battery (err %d)", err);
            bluetooth_set_error(ERROR_BATTERY);
        }

        bluetooth_set_status(STATUS_NEW_DATA, true);

        k_timer_status_sync(&update_timer);
    }
#pragma clang diagnostic pop
}


K_THREAD_STACK_DEFINE(update_thread_stack, 1024);
struct k_thread update_thread_data;

void main(void) {
    int err = 0;

    IF_ERR(watchdog_init()) {
        LOG_ERR("Watchdog initialization failed (err %d)", err);
    }

    IF_ERR(settings_subsys_init()) {
        LOG_ERR("Settings initialization failed (err %d)", err);
    }

#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
        img_mgmt_register_group();
#endif

    IF_ERR (battery_init()) {
        LOG_ERR("Battery initialization failed (err %d)", err);
    }
    IF_ERR (temperature_init()) {
        LOG_ERR("Temperature initialization failed (err %d)", err);
    }
    IF_ERR(water_level_init()) {
        LOG_ERR("Water level initialization failed (err %d)", err);
    }
    IF_ERR (bluetooth_init()) {
        LOG_ERR("Bluetooth initiallization failed (err %d)", err);
    }

    k_thread_create(
            &update_thread_data,
            update_thread_stack, K_THREAD_STACK_SIZEOF(update_thread_stack),
            update_thread, NULL, NULL, NULL,
            K_PRIO_PREEMPT(5), 0, K_NO_WAIT
    );
}
