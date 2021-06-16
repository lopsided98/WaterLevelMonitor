#pragma once

int water_level_init(void);

int water_level_update(void);

uint16_t water_level_get(void);

uint16_t water_level_get_water_distance(void);

uint16_t water_level_get_tank_depth(void);

/**
 * Set the depth of the water tank.
 *
 * @param depth depth in millimeters
 */
void water_level_set_tank_depth(uint16_t depth);