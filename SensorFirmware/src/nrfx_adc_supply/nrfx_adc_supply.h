#pragma once

#include <device.h>
#include <sensor.h>


struct nrfx_adc_supply_data {
    struct device* adc;
    struct sensor_value value;
};
