#include "water_level.h"

#include <device.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <settings/settings.h>

#include "bluetooth.h"
#include "common.h"

LOG_MODULE_REGISTER(water_level);

#define NUM_WATER_SAMPLES 10
#define MAX_WATER_SAMPLE_ATTEMPTS 20

static const struct device* rangefinder;

static uint16_t water_level = 0;     // mm
static uint16_t water_distance = 0;  // mm
static uint16_t tank_depth = 2000;   // mm

static int water_level_settings_set(const char* key, size_t len_rd, settings_read_cb read_cb,
                                    void* cb_arg) {
    int err = 0;
    int len = settings_name_next(key, NULL);
    if (!strncmp(key, "td", len)) {
        RET_ERR(read_cb(cb_arg, &tank_depth, sizeof(tank_depth)));
    } else {
        return -ENOENT;
    }
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(water_level_settings, "wl", NULL, water_level_settings_set, NULL,
                               NULL);

int water_level_init(void) {
    rangefinder = device_get_binding(DT_LABEL(DT_NODELABEL(rangefinder)));
    if (!rangefinder) return -ENODEV;

    return 0;
}

int water_level_update(void) {
    int err = 0;
    if (!rangefinder) return -ENODEV;

    pm_device_state_set(rangefinder, PM_DEVICE_STATE_ACTIVE, NULL, NULL);

    uint32_t distance_mm_avg = 0;

    int samples = 0;
    for (int tries = 0; tries < MAX_WATER_SAMPLE_ATTEMPTS && samples < NUM_WATER_SAMPLES; ++tries) {
        err = sensor_sample_fetch(rangefinder);
        if (!err) {
            struct sensor_value distance;
            sensor_channel_get(rangefinder, SENSOR_CHAN_DISTANCE, &distance);

            ++samples;
            uint32_t distance_mm = (uint32_t)(distance.val1 * 1000 + distance.val2 / 1000);
            distance_mm_avg += distance_mm;
        } else {
            LOG_WRN("Failed to read rangefinder (err %d)", err);
        }
        k_sleep(K_MSEC(50));
    }

    pm_device_state_set(rangefinder, PM_DEVICE_STATE_OFF, NULL, NULL);

    if (samples != NUM_WATER_SAMPLES) {
        LOG_ERR("Only measured %d water level samples", samples);
        bluetooth_set_error(ERROR_WATER_LEVEL);
    }

    distance_mm_avg /= samples;
    LOG_INF("Distance (avg): %d mm", distance_mm_avg);

    water_distance = distance_mm_avg;
    water_level = tank_depth > distance_mm_avg ? tank_depth - distance_mm_avg : 0;

    return 0;
}

uint16_t water_level_get(void) { return water_level; }

uint16_t water_level_get_water_distance(void) { return water_distance; }

uint16_t water_level_get_tank_depth(void) { return tank_depth; }

void water_level_set_tank_depth(uint16_t depth) {
    tank_depth = depth;
    settings_save_one("wl/td", &tank_depth, sizeof(tank_depth));
}
