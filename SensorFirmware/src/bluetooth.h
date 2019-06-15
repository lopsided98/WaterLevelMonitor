#pragma once

#include <zephyr.h>

int bluetooth_init(void);

int bluetooth_set_battery_level(u8_t level);

/**
 * Set the temperature characteristic value.
 *
 * @param temp temperature in milli-celsius
 * @return errno
 */
int bluetooth_set_temperature(s16_t temp);

int bluetooth_set_water_level(u16_t level);