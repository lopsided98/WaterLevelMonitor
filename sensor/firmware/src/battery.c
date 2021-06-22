#include "battery.h"

#include <drivers/adc.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <sys/atomic.h>

#include "bluetooth.h"
#include "common.h"

LOG_MODULE_REGISTER(battery);

static const uint16_t BATTERY_FULL_MILLIVOLTS = 3200;
static const uint16_t BATTERY_EMPTY_MILLIVOLTS = 1800;

static struct {
    const struct device* battery;
    atomic_t level;    // %
    atomic_t voltage;  // mV
} state = {.battery = NULL, .level = ATOMIC_INIT(0), .voltage = ATOMIC_INIT(0)};

int battery_init(void) {
    state.battery = device_get_binding(DT_LABEL(DT_NODELABEL(battery)));
    if (!state.battery) return -ENODEV;

    return 0;
}

int battery_update(void) {
    int err = 0;
    if (!state.battery) return -ENODEV;

    RET_ERR(sensor_sample_fetch(state.battery));

    struct sensor_value voltage_value;
    RET_ERR(sensor_channel_get(state.battery, SENSOR_CHAN_VOLTAGE, &voltage_value));

    uint32_t voltage = voltage_value.val1 * 1000 + voltage_value.val2 / 1000;
    voltage = MIN(voltage, UINT16_MAX);
    atomic_set(&state.voltage, voltage);

    uint32_t level = (voltage - BATTERY_EMPTY_MILLIVOLTS) * 100 /
                     (BATTERY_FULL_MILLIVOLTS - BATTERY_EMPTY_MILLIVOLTS);
    level = MIN(level, 100);
    atomic_set(&state.level, level);

    LOG_DBG("Battery state: %d%%, %d.%d V\n", level, voltage_value.val1, voltage_value.val2);

    return 0;
}

uint8_t battery_get_level(void) { return (uint8_t)atomic_get(&state.level); }

uint16_t battery_get_voltage(void) { return (uint16_t)atomic_get(&state.voltage); }