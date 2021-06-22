#include "temperature.h"

#include <device.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <settings/settings.h>
#include <sys/atomic.h>

#include "bluetooth.h"
#include "common.h"

LOG_MODULE_REGISTER(temperature);

static struct {
    const struct device* sensor;
    atomic_t temperature;
} state = {.sensor = NULL, .temperature = ATOMIC_INIT(0)};

int temperature_init(void) {
    state.sensor = device_get_binding(DT_LABEL(DT_NODELABEL(temp)));
    if (!state.sensor) return -ENODEV;

    return 0;
}

int temperature_update(void) {
    int err = 0;
    if (!state.sensor) return -ENODEV;

    RET_ERR(sensor_sample_fetch(state.sensor));

    struct sensor_value temperature_value;

    RET_ERR(sensor_channel_get(state.sensor, SENSOR_CHAN_DIE_TEMP, &temperature_value));
    // Convert to millicelsius
    int32_t temperature = temperature_value.val1 * 100 + temperature_value.val2 / 10000;
    // Bound to 16-bit (more than enough)
    temperature = MAX(INT16_MIN, MIN(INT16_MAX, temperature));
    atomic_set(&state.temperature, temperature);

    return err;
}

uint16_t temperature_get(void) { return (uint16_t)atomic_get(&state.temperature); }