#include "battery.h"

#include <drivers/adc.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "bluetooth.h"
#include "common.h"

LOG_MODULE_REGISTER(battery);

static const uint16_t BATTERY_FULL_MILLIVOLTS = 3200;
static const uint16_t BATTERY_EMPTY_MILLIVOLTS = 1800;

static const struct device* battery;

static uint8_t level = 0;   // %
static uint16_t voltage = 0;  // mV

int battery_init(void) {
    battery = device_get_binding(DT_LABEL(DT_NODELABEL(battery)));
    if (!battery) return -ENODEV;

    return 0;
}

int battery_update(void) {
    int err = 0;
    if (!battery) return -ENODEV;

    RET_ERR(sensor_sample_fetch(battery));

    struct sensor_value voltage_value;
    RET_ERR(sensor_channel_get(battery, SENSOR_CHAN_VOLTAGE, &voltage_value));

    voltage = (uint16_t)(voltage_value.val1 * 1000 + voltage_value.val2 / 1000);

    level = (voltage - BATTERY_EMPTY_MILLIVOLTS) * 100 /
            (BATTERY_FULL_MILLIVOLTS - BATTERY_EMPTY_MILLIVOLTS);
    level = MIN(level, 100);

    LOG_DBG("Battery state: %d%%, %d.%d V\n", level, voltage_value.val1, voltage_value.val2);

    bluetooth_set_battery_level(level);

    return 0;
}

uint16_t battery_get_voltage(void) {
    return voltage;
}