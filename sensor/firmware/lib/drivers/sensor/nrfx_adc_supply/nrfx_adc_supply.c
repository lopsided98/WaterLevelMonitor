#define DT_DRV_COMPAT nordic_adc_supply

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrfx_adc_supply, CONFIG_SENSOR_LOG_LEVEL);

struct nrfx_adc_supply_data {
    struct sensor_value value;
};

struct nrfx_adc_supply_config {
    const struct device* adc;
};

#define NRFX_ADC_VBG_VOLTAGE 1200

int nrfx_adc_supply_sample_fetch(const struct device* dev, enum sensor_channel chan) {
    const struct nrfx_adc_supply_config* config = dev->config;
    struct nrfx_adc_supply_data* data = dev->data;

    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE) return -ENOTSUP;

    int16_t sample = 0;
    const uint8_t resolution = 10;

    const struct adc_sequence sequence = {
        .channels = BIT(0),
        .buffer = &sample,
        .buffer_size = sizeof(sample),
        .resolution = resolution,
    };

    int err = adc_read(config->adc, &sequence);
    if (err < 0) return err;

    uint32_t millivolts = (((uint32_t)sample) * NRFX_ADC_VBG_VOLTAGE * 3) >> resolution;

    data->value.val1 = millivolts / 1000;
    data->value.val2 = (millivolts % 1000) * 1000;

    return err;
}

static int nrfx_adc_supply_channel_get(const struct device* dev, enum sensor_channel chan,
                                       struct sensor_value* val) {
    struct nrfx_adc_supply_data* data = dev->data;

    if (chan != SENSOR_CHAN_VOLTAGE) return -ENOTSUP;

    memcpy(val, &data->value, sizeof(data->value));

    return 0;
}

static int nrfx_adc_supply_init(const struct device* dev) {
    const struct nrfx_adc_supply_config* config = dev->config;

    if (!device_is_ready(config->adc)) {
        LOG_ERR_DEVICE_NOT_READY(config->adc);
        return -ENODEV;
    }

    struct adc_channel_cfg adc_cfg = {
        .channel_id = 0,  // FIXME: should this be configurable?
        .gain = ADC_GAIN_SUPPLY_1_3,
        .reference = ADC_REF_INTERNAL,
        .input_positive = ADC_CONFIG_PSEL_Disabled,
    };

    return adc_channel_setup(config->adc, &adc_cfg);
}

const struct sensor_driver_api nrfx_adc_supply_api = {
    .sample_fetch = &nrfx_adc_supply_sample_fetch,
    .channel_get = &nrfx_adc_supply_channel_get,
};

static struct nrfx_adc_supply_data nrfx_adc_supply_data;

static const struct nrfx_adc_supply_config nrfx_adc_supply_config = {
    .adc = DEVICE_DT_GET(DT_NODELABEL(adc)),
};

DEVICE_DT_INST_DEFINE(0, &nrfx_adc_supply_init, NULL, &nrfx_adc_supply_data,
                      &nrfx_adc_supply_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
                      &nrfx_adc_supply_api);
