#pragma once

#include <stdint.h>

int temperature_init(void);

int temperature_update(void);

/**
 * Get the temperature in millicelsius.
 *
 * @return temperature in mC
 */
uint16_t temperature_get(void);