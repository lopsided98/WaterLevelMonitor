#pragma once

#include <stdint.h>

int battery_init(void);

int battery_update(void);

/**
 * Get the battery level as a percentage.
 *
 * @return percent of the battery remaining (0-100)
 */
uint8_t battery_get_level(void);

/**
 * Get the battery voltage in millivolts.
 *
 * @return voltage in millivolts
 */
uint16_t battery_get_voltage(void);