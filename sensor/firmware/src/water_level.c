#include "water_level.h"

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/settings/settings.h>

#include "bluetooth.h"
#include "common.h"
#include "zephyr/sys/util.h"

LOG_MODULE_REGISTER(water_level);

#define NUM_WATER_SAMPLES 10
#define MAX_WATER_SAMPLE_ATTEMPTS 30

static struct {
    const struct device* const rangefinder;
    atomic_t water_level;     // mm
    atomic_t water_distance;  // mm
    atomic_t tank_depth;      // mm
} state = {
    .rangefinder = DEVICE_DT_GET(DT_NODELABEL(rangefinder)),
    .water_level = ATOMIC_INIT(0),
    .water_distance = ATOMIC_INIT(0),
    .tank_depth = ATOMIC_INIT(2000),
};

static int water_level_settings_set(const char* key, size_t len_rd, settings_read_cb read_cb,
                                    void* cb_arg) {
    int err;
    int len = settings_name_next(key, NULL);
    if (!strncmp(key, "td", len)) {
        uint16_t tank_depth;
        RET_ERR(read_cb(cb_arg, &tank_depth, sizeof(tank_depth)));
        atomic_set(&state.tank_depth, tank_depth);
    } else {
        return -ENOENT;
    }
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(water_level_settings, "wl", NULL, water_level_settings_set, NULL,
                               NULL);

int water_level_init(void) {
    if (!device_is_ready(state.rangefinder)) {
        LOG_ERR_DEVICE_NOT_READY(state.rangefinder);
        return -ENODEV;
    }

    return 0;
}

int compare_uint32(const void* a, const void* b) {
    uint32_t val_a = *(const uint32_t*)a;
    uint32_t val_b = *(const uint32_t*)b;

    if (val_a < val_b) return -1;
    if (val_a > val_b) return 1;
    return 0;
}

int water_level_update(void) {
    int err;
    const uint32_t tank_depth = atomic_get(&state.tank_depth);
    // 6 samples >1.3x tank depth
    const uint32_t max_distance_mm = tank_depth + DIV_ROUND_CLOSEST(tank_depth, 3);

    pm_device_runtime_get(state.rangefinder);

    uint32_t distance_mm_samples[NUM_WATER_SAMPLES];

    size_t samples = 0;
    for (size_t tries = 0; tries < MAX_WATER_SAMPLE_ATTEMPTS && samples < NUM_WATER_SAMPLES;
         ++tries) {
        err = sensor_sample_fetch(state.rangefinder);
        if (!err) {
            struct sensor_value distance;
            sensor_channel_get(state.rangefinder, SENSOR_CHAN_DISTANCE, &distance);

            uint32_t distance_mm = (uint32_t)(distance.val1 * 1000 + distance.val2 / 1000);
            // Only include reasonable distance samples
            if (distance_mm < max_distance_mm) {
                distance_mm_samples[samples] = distance_mm;
                ++samples;
                LOG_DBG("Distance (sample %d): %u mm", samples, distance_mm);
            } else {
                LOG_WRN("Distance out of range: %u mm", distance_mm);
            }
        } else {
            LOG_WRN("Failed to read rangefinder (err %d)", err);
        }
        // Wait long enough between samples to allow echoes to decay
        k_sleep(K_MSEC(50));
    }

    pm_device_runtime_put(state.rangefinder);

    if (0 == samples) {
        // No samples collected, don't update distance
        LOG_ERR("No valid water level samples");
        bluetooth_set_error(ERROR_WATER_LEVEL);
        return 0;
    }

    if (samples != NUM_WATER_SAMPLES) {
        LOG_WRN("Only measured %d water level samples", samples);
        bluetooth_set_error(ERROR_WATER_LEVEL);
    }

    // Median filter
    qsort(distance_mm_samples, samples, sizeof(*distance_mm_samples), compare_uint32);
    uint32_t distance_mm_filtered;
    if (samples % 2 == 0) {
        // Even number of samples, everage middle two
        distance_mm_filtered = DIV_ROUND_CLOSEST(
            distance_mm_samples[samples / 2] + distance_mm_samples[samples / 2 - 1],
            2);
    } else {
        // Odd number of samples, single median value
        distance_mm_filtered = distance_mm_samples[samples / 2];
    }

    LOG_INF("Distance (median): %u mm", distance_mm_filtered);

    atomic_set(&state.water_distance, distance_mm_filtered);
    atomic_set(&state.water_level,
               tank_depth > distance_mm_filtered ? tank_depth - distance_mm_filtered : 0);

    return 0;
}

uint16_t water_level_get(void) { return (uint16_t)atomic_get(&state.water_level); }

uint16_t water_level_get_water_distance(void) {
    return (uint16_t)atomic_get(&state.water_distance);
}

uint16_t water_level_get_tank_depth(void) { return (uint16_t)atomic_get(&state.tank_depth); }

void water_level_set_tank_depth(uint16_t depth) {
    atomic_set(&state.tank_depth, depth);
    settings_save_one("wl/td", &depth, sizeof(depth));
}
