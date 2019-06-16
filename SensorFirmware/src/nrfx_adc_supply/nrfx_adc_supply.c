#include <gpio.h>
#include <sensor.h>
#include <logging/log.h>
#include <adc.h>
#include <soc.h>

#include "../common.h"
#include "nrfx_adc_supply.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(nrfx_adc_supply);

#define NRFX_ADC_VBG_VOLTAGE 1200

int nrfx_adc_supply_sample_fetch(struct device* dev, enum sensor_channel chan) {
    int err = 0;

    struct nrfx_adc_supply_data* data = dev->driver_data;

    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE)
        return -ENOTSUP;

    s16_t sample = 0;
    const u8_t resolution = 10;

    const struct adc_sequence sequence = {
            .channels    = BIT(0),
            .buffer      = &sample,
            .buffer_size = sizeof(sample),
            .resolution  = resolution
    };

    RET_ERR(adc_read(data->adc, &sequence));

    u32_t millivolts = (((u32_t) sample) * NRFX_ADC_VBG_VOLTAGE * 3) >> resolution;

    data->value.val1 = millivolts / 1000;
    data->value.val2 = (millivolts % 1000) * 1000;

    return err;
}

static int nrfx_adc_supply_channel_get(struct device* dev,
                                      enum sensor_channel chan,
                                      struct sensor_value* val) {
    struct nrfx_adc_supply_data* data = dev->driver_data;

    if (chan != SENSOR_CHAN_VOLTAGE)
        return -ENOTSUP;

    memcpy(val, &data->value, sizeof(data->value));

    return 0;
}

static int nrfx_adc_supply_init(struct device* dev) {
    int err = 0;

    struct nrfx_adc_supply_data* data = dev->driver_data;

    struct adc_channel_cfg adc_cfg = {
            // FIXME: should this be configurable?
            .channel_id = 0,
            .gain = ADC_GAIN_SUPPLY_1_3,
            .reference = ADC_REF_INTERNAL,
            .input_positive = ADC_CONFIG_PSEL_Disabled
    };

    data->adc = device_get_binding(DT_INST_0_NORDIC_NRF_ADC_LABEL);
    if (!data->adc) {
        LOG_ERR("Could not find ADC: %s", DT_INST_0_NORDIC_NRF_ADC_LABEL);
        return -EINVAL;
    }

    RET_ERR(adc_channel_setup(data->adc, &adc_cfg));

    return 0;
}

const struct sensor_driver_api nrfx_adc_supply_api = {
        .sample_fetch = &nrfx_adc_supply_sample_fetch,
        .channel_get = &nrfx_adc_supply_channel_get,
};

static struct nrfx_adc_supply_data nrfx_adc_supply_data;

DEVICE_AND_API_INIT(nrfx_adc_supply_dev, DT_INST_0_NORDIC_ADC_SUPPLY_LABEL, &nrfx_adc_supply_init,
                    &nrfx_adc_supply_data, NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &nrfx_adc_supply_api);