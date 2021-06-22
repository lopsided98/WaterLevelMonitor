#include "temperature.h"

#include <device.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <settings/settings.h>

#include "bluetooth.h"
#include "common.h"

LOG_MODULE_REGISTER(temperature);

static const struct device* sensor;

static int16_t temperature = 0;

int temperature_init(void) {
    sensor = device_get_binding(DT_LABEL(DT_NODELABEL(temp)));
    if (!sensor) return -ENODEV;

    return 0;
}

int temperature_update(void) {
    int err = 0;
    if (!sensor) return -ENODEV;

    RET_ERR(sensor_sample_fetch(sensor));

    struct sensor_value temperature_value;

    RET_ERR(sensor_channel_get(sensor, SENSOR_CHAN_DIE_TEMP, &temperature_value));
    // Convert to millicelsius as 32-bit
    int32_t temperature_32 = temperature_value.val1 * 100 + temperature_value.val2 / 10000;
    // Bound to 16-bit (more than enough)
    temperature = (int16_t)MAX(INT16_MIN, MIN(INT16_MAX, temperature_32));

    return err;
}

uint16_t temperature_get(void) {
    return temperature;
}