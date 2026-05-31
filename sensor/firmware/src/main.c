#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include "battery.h"
#include "bluetooth.h"
#include "common.h"
#include "temperature.h"
#include "watchdog.h"
#include "water_level.h"
#include "zephyr/kernel.h"

LOG_MODULE_REGISTER(main);

#define UPDATE_PERIOD K_MINUTES(15)

K_TIMER_DEFINE(update_timer, NULL, NULL);

static void run(void) {
    int err;

    k_timer_start(&update_timer, UPDATE_PERIOD, UPDATE_PERIOD);

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
}

void main(void) {
    int err;

    IF_ERR(watchdog_init()) { LOG_ERR("Watchdog initialization failed (err %d)", err); }

    IF_ERR(settings_subsys_init()) { LOG_ERR("Settings initialization failed (err %d)", err); }

    IF_ERR(battery_init()) { LOG_ERR("Battery initialization failed (err %d)", err); }
    IF_ERR(temperature_init()) { LOG_ERR("Temperature initialization failed (err %d)", err); }
    IF_ERR(water_level_init()) { LOG_ERR("Water level initialization failed (err %d)", err); }
    IF_ERR(bluetooth_init()) { LOG_ERR("Bluetooth initiallization failed (err %d)", err); }

    run();
}
