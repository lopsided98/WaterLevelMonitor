#include <gpio.h>
#include <sensor.h>
#include <logging/log.h>

#include "../common.h"
#include "jsn_sr04t.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(jsn_sr04t);

static void sr04t_echo_interrupt(struct device* port, struct gpio_callback* cb, u32_t pins) {
    u32_t cycles = k_cycle_get_32();

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

int sr04t_sample_fetch(struct device* dev, enum sensor_channel chan) {
    int err = 0;

    const struct sr04t_config *config = dev->config->config_info;
    struct sr04t_data* data = dev->driver_data;

    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DISTANCE)
        return -ENOTSUP;

    device_busy_set(dev);

    IF_ERR(gpio_pin_write(data->trig_gpio, config->trig_gpio_pin, 1)) goto cleanup;
    k_busy_wait(50);
    IF_ERR(gpio_pin_write(data->trig_gpio, config->trig_gpio_pin, 0)) goto cleanup;

    data->state = STATE_WAIT_ECHO_START;

    k_sem_reset(&data->echo_end_sem);

    IF_ERR(gpio_pin_enable_callback(data->echo_gpio, config->echo_gpio_pin)) goto cleanup;

    // Wait for interrupt processing to finish
    // FIXME: causes BLE stack to crash under load
//    err = k_sem_take(&data->echo_end_sem, K_MSEC(CONFIG_JSN_SR04T_ECHO_TIMEOUT));
    k_sleep(CONFIG_JSN_SR04T_ECHO_TIMEOUT);
    __ASSERT_NO_MSG(err != -EBUSY);
    if (err == -EAGAIN) {
        // Timeout
        err = -EIO;
        goto cleanup;
    }

    __ASSERT_NO_MSG(data->state == STATE_READY);

    IF_ERR(gpio_pin_disable_callback(data->echo_gpio, config->echo_gpio_pin)) goto cleanup;

    u32_t elapsed_cycles = data->echo_end_cycles - data->echo_start_cycles;
    u32_t elapsed_ns = SYS_CLOCK_HW_CYCLES_TO_NS(elapsed_cycles);

    u16_t dist_mm = elapsed_ns / CONFIG_JSN_SR04T_NS_PER_MM;

    data->value.val1 = dist_mm / 1000;
    data->value.val2 = (dist_mm % 1000) * 1000;

    cleanup:

    gpio_pin_disable_callback(data->echo_gpio, config->echo_gpio_pin);
    data->state = STATE_READY;

    device_busy_clear(dev);

    return err;
}

static int sr04t_channel_get(struct device* dev,
                             enum sensor_channel chan,
                             struct sensor_value* val) {
    struct sr04t_data* data = dev->driver_data;

    if (chan != SENSOR_CHAN_DISTANCE)
        return -ENOTSUP;

    memcpy(val, &data->value, sizeof(data->value));

    return 0;
}

static int sr04t_pm_control(struct device* dev, u32_t cmd, void* context, device_pm_cb cb, void* arg) {
    int err = 0;

    const struct sr04t_config *config = dev->config->config_info;
    struct sr04t_data* data = dev->driver_data;

    switch (cmd) {
        case DEVICE_PM_SET_POWER_STATE:
            switch (*(u32_t*) context) {
                case DEVICE_PM_ACTIVE_STATE:
                    if (data->state == STATE_OFF) {
                        IF_ERR(gpio_pin_write(data->en_gpio, config->en_gpio_pin, 1)) goto cleanup;
                        // Wait for device to start
                        k_sleep(K_MSEC(20));
                        data->state = STATE_READY;
                    }
                    break;
                case DEVICE_PM_OFF_STATE:
                    if (data->state != STATE_OFF) {
                        IF_ERR(gpio_pin_write(data->en_gpio, config->en_gpio_pin, 0)) goto cleanup;
                        data->state = STATE_OFF;
                    }
                    break;
                default:
                    err = -EINVAL;
                    goto cleanup;
            }
            break;
        case DEVICE_PM_GET_POWER_STATE:
            *(u32_t*) context = data->state == STATE_OFF ?
                                DEVICE_PM_OFF_STATE : DEVICE_PM_ACTIVE_STATE;
            break;
        default:
            err = -EINVAL;
            goto cleanup;
    }

    cleanup:
    if (cb) {
        cb(dev, err, context, arg);
    }

    return err;
}

static int sr04t_init(struct device* dev) {
    int err = 0;

    const struct sr04t_config *config = dev->config->config_info;
    struct sr04t_data* data = dev->driver_data;

    data->state = STATE_OFF;

    k_sem_init(&data->echo_end_sem, 0, 1);

    // Trig
    data->trig_gpio = device_get_binding(config->trig_gpio_name);
    if (!data->trig_gpio) {
        LOG_ERR("Could not find trigger GPIO device: %s", config->trig_gpio_name);
        return -EINVAL;
    }
    RET_ERR(gpio_pin_configure(data->trig_gpio, config->trig_gpio_pin, config->trig_gpio_flags));
    RET_ERR(gpio_pin_write(data->trig_gpio, config->trig_gpio_pin, 0));

    // Echo
    data->echo_gpio = device_get_binding(config->echo_gpio_name);
    if (!data->echo_gpio) {
        LOG_ERR("Could not find echo GPIO device: %s", config->echo_gpio_name);
        return -EINVAL;
    }
    RET_ERR(gpio_pin_configure(data->echo_gpio, config->echo_gpio_pin, config->echo_gpio_flags));
    gpio_init_callback(&data->echo_cb, sr04t_echo_interrupt,
                       BIT(config->echo_gpio_pin));
    RET_ERR(gpio_add_callback(data->echo_gpio, &data->echo_cb));

    // Enable
    data->en_gpio = device_get_binding(config->en_gpio_name);
    if (!data->en_gpio) {
        LOG_ERR("Could not find enable GPIO device: %s", config->en_gpio_name);
        return -EINVAL;
    }
    RET_ERR(gpio_pin_configure(data->en_gpio, config->en_gpio_pin, config->en_gpio_flags));
    RET_ERR(gpio_pin_write(data->en_gpio, config->en_gpio_pin, 0));

    return 0;
}

const struct sensor_driver_api sr04t_api = {
        .sample_fetch = &sr04t_sample_fetch,
        .channel_get = &sr04t_channel_get,
};

static const struct sr04t_config sr04t_config = {
        .trig_gpio_name = DT_INST_0_JSN_SR04T_TRIG_GPIOS_CONTROLLER,
        .trig_gpio_pin = DT_INST_0_JSN_SR04T_TRIG_GPIOS_PIN,
        .trig_gpio_flags = DT_INST_0_JSN_SR04T_TRIG_GPIOS_FLAGS,
        .echo_gpio_name = DT_INST_0_JSN_SR04T_ECHO_GPIOS_CONTROLLER,
        .echo_gpio_pin = DT_INST_0_JSN_SR04T_ECHO_GPIOS_PIN,
        .echo_gpio_flags = DT_INST_0_JSN_SR04T_ECHO_GPIOS_FLAGS,
        .en_gpio_name = DT_INST_0_JSN_SR04T_EN_GPIOS_CONTROLLER,
        .en_gpio_pin = DT_INST_0_JSN_SR04T_EN_GPIOS_PIN,
        .en_gpio_flags = DT_INST_0_JSN_SR04T_EN_GPIOS_FLAGS
};

static struct sr04t_data sr04t_data;

DEVICE_DEFINE(sr04t_dev, DT_INST_0_JSN_SR04T_LABEL, &sr04t_init, &sr04t_pm_control, &sr04t_data,
              &sr04t_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &sr04t_api);