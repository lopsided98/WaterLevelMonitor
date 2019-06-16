#include <sensor.h>
#include <adc.h>
#include <logging/log.h>
#include "battery.h"
#include "common.h"
#include "bluetooth.h"

LOG_MODULE_REGISTER(battery);

#define BATTERY_FULL_MILLIVOLTS 3000
#define BATTERY_EMPTY_MILLIVOLTS 1800

static struct device* battery;

int battery_init(void) {
    battery = device_get_binding(DT_NORDIC_ADC_SUPPLY_BATTERY_LABEL);
    if (!battery) {
        LOG_ERR("Could not find battery");
        return -EINVAL;
    }

    return 0;
}

int battery_update(void) {
    int err = 0;

    RET_ERR(sensor_sample_fetch(battery));

    struct sensor_value voltage;
    RET_ERR(sensor_channel_get(battery, SENSOR_CHAN_VOLTAGE, &voltage));

    u32_t millivolts = (u32_t) (voltage.val1 * 1000 + voltage.val2 / 1000);

    u8_t level = (millivolts - BATTERY_EMPTY_MILLIVOLTS) * 100
                 / (BATTERY_FULL_MILLIVOLTS - BATTERY_EMPTY_MILLIVOLTS);

    level = MAX(level, 100);

    LOG_DBG("Battery state: %d%%, %d.%d V\n", level, voltage.val1, voltage.val2);

    bluetooth_set_battery_level(level);

    return 0;
}