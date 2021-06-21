#pragma once

#include <device.h>
#include <drivers/sensor.h>

struct nrfx_adc_supply_data {
    const struct device* adc;
    struct sensor_value value;
};
