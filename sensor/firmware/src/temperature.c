#include "temperature.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>

#include "common.h"

LOG_MODULE_REGISTER(temperature);

static struct {
    const struct device* const sensor;
    atomic_t temperature;
} state = {
    .sensor = DEVICE_DT_GET(DT_NODELABEL(temp)),
    .temperature = ATOMIC_INIT(0),
};

int temperature_init(void) { return 0; }

int temperature_update(void) {
    int err;

    RET_ERR(sensor_sample_fetch(state.sensor));

    struct sensor_value temperature_value;

    RET_ERR(sensor_channel_get(state.sensor, SENSOR_CHAN_DIE_TEMP, &temperature_value));
    // Convert to millicelsius
    int32_t temperature = temperature_value.val1 * 100 + temperature_value.val2 / 10000;
    // Bound to 16-bit (more than enough)
    temperature = clamp(temperature, INT16_MIN, INT16_MAX);
    atomic_set(&state.temperature, temperature);

    return err;
}

uint16_t temperature_get(void) { return (uint16_t)atomic_get(&state.temperature); }