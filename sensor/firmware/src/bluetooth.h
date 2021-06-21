#pragma once

#include <stdbool.h>
#include <sys/util.h>

enum system_error {
    ERROR_TEMPERATURE = BIT(0),
    ERROR_BATTERY = BIT(1),
    ERROR_WATER_LEVEL = BIT(2),
    ERROR_BROWNOUT = BIT(3),
};

enum system_status {
    STATUS_NEW_DATA = BIT(0),
};

int bluetooth_init(void);

int bluetooth_set_battery_level(uint8_t level);

/**
 * Set the temperature characteristic value.
 *
 * @param temp temperature in milli-celsius
 * @return errno
 */
int bluetooth_set_temperature(int16_t temp);

/**
 * Set a latched error bit.
 *
 * @param e error bit to set
 */
void bluetooth_set_error(enum system_error e);

/**
 * Check whether an error bit is set.
 *
 * @param e error bit to get
 */
bool bluetooth_get_error(enum system_error e);

/**
 * Set the value of a status bit.
 * @param s status bit to set
 * @param value value of the bit
 */
void bluetooth_set_status(enum system_status s, bool value);
