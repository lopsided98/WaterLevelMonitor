#include <device.h>
#include <sensor.h>
#include <logging/log.h>
#include <settings/settings.h>
#include "temperature.h"
#include "common.h"
#include "bluetooth.h"

LOG_MODULE_REGISTER(temperature);

static struct device* sensor;

int temperature_init(void) {
    sensor = device_get_binding(CONFIG_TEMP_NRF5_NAME);
    if (!sensor) return -ENODEV;

    return 0;
}

int temperature_update(void) {
    int err = 0;

    RET_ERR(sensor_sample_fetch(sensor));

    struct sensor_value temp;

    RET_ERR(sensor_channel_get(sensor, SENSOR_CHAN_DIE_TEMP, &temp));

    // Temperature in millicelsius
    s32_t temp_mc = temp.val1 * 100 + temp.val2 / 10000;

    temp_mc = MAX(INT16_MIN, MIN(INT16_MAX, temp_mc));

    RET_ERR(bluetooth_set_temperature((s16_t) temp_mc));

    return err;
}