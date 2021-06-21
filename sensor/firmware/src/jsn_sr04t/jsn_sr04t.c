#define DT_DRV_COMPAT jsn_sr04t

#include "jsn_sr04t.h"

#include <drivers/gpio.h>
#include <logging/log.h>

#include "../common.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(jsn_sr04t);

static void sr04t_echo_interrupt(const struct device* port, struct gpio_callback* cb,
                                 uint32_t pins) {
    uint32_t cycles = k_cycle_get_32();

    struct sr04t_data* data = CONTAINER_OF(cb, struct sr04t_data, echo_cb);

    switch (data->state) {
        case STATE_WAIT_ECHO_START:
            data->echo_start_cycles = cycles;
            data->state = STATE_WAIT_ECHO_END;
            break;
        case STATE_WAIT_ECHO_END:
            data->echo_end_cycles = cycles;
            data->state = STATE_READY;
            k_sem_give(&data->echo_end_sem);
            break;
        default:
            break;
    }
}

int sr04t_sample_fetch(const struct device* dev, enum sensor_channel chan) {
    int err = 0;

    const struct sr04t_config* config = dev->config;
    struct sr04t_data* data = dev->data;

    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DISTANCE) return -ENOTSUP;

    device_busy_set(dev);

    IF_ERR(gpio_pin_set(config->trig_gpio.port, config->trig_gpio.pin, 1)) goto cleanup;
    k_busy_wait(50);
    IF_ERR(gpio_pin_set(config->trig_gpio.port, config->trig_gpio.pin, 0)) goto cleanup;

    data->state = STATE_WAIT_ECHO_START;

    k_sem_reset(&data->echo_end_sem);

    IF_ERR(gpio_pin_interrupt_configure(config->echo_gpio.port,
                                        config->echo_gpio.pin,
                                        GPIO_INT_EDGE_BOTH))
    goto cleanup;

    // Wait for interrupt processing to finish
    // FIXME: causes BLE stack to crash under load
    err = k_sem_take(&data->echo_end_sem, K_MSEC(CONFIG_JSN_SR04T_ECHO_TIMEOUT));
    //     k_sleep(CONFIG_JSN_SR04T_ECHO_TIMEOUT);
    __ASSERT_NO_MSG(err != -EBUSY);
    if (err == -EAGAIN) {
        // Timeout
        err = -EIO;
        goto cleanup;
    }

    __ASSERT_NO_MSG(data->state == STATE_READY);

    IF_ERR(gpio_pin_interrupt_configure(config->echo_gpio.port,
                                        config->echo_gpio.pin,
                                        GPIO_INT_DISABLE))
    goto cleanup;

    uint32_t elapsed_cycles = data->echo_end_cycles - data->echo_start_cycles;
    uint32_t elapsed_ns = k_cyc_to_ns_floor32(elapsed_cycles);

    uint16_t dist_mm = elapsed_ns / CONFIG_JSN_SR04T_NS_PER_MM;

    data->value.val1 = dist_mm / 1000;
    data->value.val2 = (dist_mm % 1000) * 1000;

cleanup:

    gpio_pin_interrupt_configure(config->echo_gpio.port, config->echo_gpio.pin, GPIO_INT_DISABLE);
    data->state = STATE_READY;

    device_busy_clear(dev);

    return err;
}

static int sr04t_channel_get(const struct device* dev, enum sensor_channel chan,
                             struct sensor_value* val) {
    struct sr04t_data* data = dev->data;

    if (chan != SENSOR_CHAN_DISTANCE) return -ENOTSUP;

    memcpy(val, &data->value, sizeof(data->value));

    return 0;
}

static int sr04t_pm_control(const struct device* dev, uint32_t cmd, uint32_t* state,
                            pm_device_cb cb, void* arg) {
    int err = 0;

    const struct sr04t_config* config = dev->config;
    struct sr04t_data* data = dev->data;

    switch (cmd) {
        case PM_DEVICE_STATE_SET:
            switch (*state) {
                case PM_DEVICE_STATE_ACTIVE:
                    if (data->state == STATE_OFF) {
                        IF_ERR(gpio_pin_set(config->supply_gpio.port, config->supply_gpio.pin, 1))
                        goto cleanup;
                        // Wait for device to start
                        k_sleep(K_MSEC(100));
                        data->state = STATE_READY;
                    }
                    break;
                case PM_DEVICE_STATE_OFF:
                    if (data->state != STATE_OFF) {
                        IF_ERR(gpio_pin_set(config->supply_gpio.port, config->supply_gpio.pin, 0))
                        goto cleanup;
                        data->state = STATE_OFF;
                    }
                    break;
                default:
                    err = -EINVAL;
                    goto cleanup;
            }
            break;
        case PM_DEVICE_STATE_GET:
            *state = data->state == STATE_OFF ? PM_DEVICE_STATE_OFF : PM_DEVICE_STATE_ACTIVE;
            break;
        default:
            err = -EINVAL;
            goto cleanup;
    }

cleanup:
    if (cb) {
        cb(dev, err, state, arg);
    }

    return err;
}

static int sr04t_init(const struct device* dev) {
    int err = 0;

    const struct sr04t_config* config = dev->config;
    struct sr04t_data* data = dev->data;

    data->state = STATE_OFF;

    k_sem_init(&data->echo_end_sem, 0, 1);

    // Trig
    if (!device_is_ready(config->trig_gpio.port)) {
        LOG_ERR("Trigger GPIO device is not ready");
        return -ENODEV;
    }
    RET_ERR(gpio_pin_configure(config->trig_gpio.port,
                               config->trig_gpio.pin,
                               GPIO_OUTPUT_INACTIVE | config->trig_gpio.dt_flags));

    // Echo
    if (!device_is_ready(config->echo_gpio.port)) {
        LOG_ERR("Echo GPIO device is not ready");
        return -ENODEV;
    }
    RET_ERR(gpio_pin_configure(config->echo_gpio.port,
                               config->echo_gpio.pin,
                               GPIO_INPUT | config->echo_gpio.dt_flags));
    gpio_init_callback(&data->echo_cb, sr04t_echo_interrupt, BIT(config->echo_gpio.pin));
    RET_ERR(gpio_add_callback(config->echo_gpio.port, &data->echo_cb));

    // Enable
    if (!device_is_ready(config->supply_gpio.port)) {
        LOG_ERR("Enable GPIO device is not ready");
        return -ENODEV;
    }
    RET_ERR(gpio_pin_configure(config->supply_gpio.port,
                               config->supply_gpio.pin,
                               GPIO_OUTPUT_INACTIVE | config->supply_gpio.dt_flags));

    return 0;
}

const struct sensor_driver_api sr04t_api = {
    .sample_fetch = &sr04t_sample_fetch,
    .channel_get = &sr04t_channel_get,
};

#define SR04T_INST(inst)                                           \
    static struct sr04t_data sr04t_data_##inst;                    \
                                                                   \
    static const struct sr04t_config sr04t_config_##inst = {       \
        .trig_gpio = GPIO_DT_SPEC_INST_GET(inst, trig_gpios),      \
        .echo_gpio = GPIO_DT_SPEC_INST_GET(inst, echo_gpios),      \
        .supply_gpio = GPIO_DT_SPEC_INST_GET(inst, supply_gpios)}; \
                                                                   \
    DEVICE_DT_INST_DEFINE(inst,                                    \
                          sr04t_init,                              \
                          sr04t_pm_control,                        \
                          &sr04t_data_##inst,                      \
                          &sr04t_config_##inst,                    \
                          POST_KERNEL,                             \
                          CONFIG_SENSOR_INIT_PRIORITY,             \
                          &sr04t_api);

DT_INST_FOREACH_STATUS_OKAY(SR04T_INST)
