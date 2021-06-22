#pragma once

#include <stdint.h>

int battery_init(void);

int battery_update(void);

/**
 * Get the battery voltage in millivolts.
 *
 * @return voltage in millivolts
 */
uint16_t battery_get_voltage(void);