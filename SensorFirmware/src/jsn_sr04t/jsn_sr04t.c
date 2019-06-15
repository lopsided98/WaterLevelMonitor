#include <gpio.h>
#include <sensor.h>
#include <logging/log.h>

#include "../common.h"
#include "jsn_sr04t.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(jsn_sr04t);

static void sr04t_echo_interrupt(struct device* port, struct gpio_callback* cb, u32_t pins) {
    u32_t cycles = k_cycle_get_32();

    struct sr04t_data* drv_data = CONTAINER_OF(cb, struct sr04t_data, echo_cb);

    switch (drv_data->state) {
        case STATE_WAIT_ECHO_START:
            drv_data->echo_start_cycles = cycles;
            drv_data->state = STATE_WAIT_ECHO_END;
            break;
        case STATE_WAIT_ECHO_END:
            drv_data->echo_end_cycles = cycles;
            drv_data->state = STATE_READY;
            k_sem_give(&drv_data->echo_end_sem);
            break;
        default:
            break;
    }
}

int sr04t_sample_fetch(struct device* dev, enum sensor_channel chan) {
    int err = 0;
    struct sr04t_data* drv_data = dev->driver_data;

    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DISTANCE)
        return -ENOTSUP;

    device_busy_set(dev);

    IF_ERR(gpio_pin_write(drv_data->gpio, CONFIG_JSN_SR04T_TRIG_GPIO_PIN_NUM, 1)) goto cleanup;
    k_busy_wait(50);
    IF_ERR(gpio_pin_write(drv_data->gpio, CONFIG_JSN_SR04T_TRIG_GPIO_PIN_NUM, 0)) goto cleanup;

    drv_data->state = STATE_WAIT_ECHO_START;

    k_sem_reset(&drv_data->echo_end_sem);

    IF_ERR(gpio_pin_enable_callback(drv_data->gpio, CONFIG_JSN_SR04T_ECHO_GPIO_PIN_NUM)) goto cleanup;

    // Wait for interrupt processing to finish
    // FIXME: causes BLE stack to crash under load
//    err = k_sem_take(&drv_data->echo_end_sem, K_MSEC(CONFIG_JSN_SR04T_ECHO_TIMEOUT));
    k_sleep(CONFIG_JSN_SR04T_ECHO_TIMEOUT);
    __ASSERT_NO_MSG(err != -EBUSY);
    if (err == -EAGAIN) {
        // Timeout
        err = -EIO;
        goto cleanup;
    }

    __ASSERT_NO_MSG(drv_data->state == STATE_READY);

    IF_ERR(gpio_pin_disable_callback(drv_data->gpio, CONFIG_JSN_SR04T_ECHO_GPIO_PIN_NUM)) goto cleanup;

    u32_t elapsed_cycles = drv_data->echo_end_cycles - drv_data->echo_start_cycles;
    u32_t elapsed_ns = SYS_CLOCK_HW_CYCLES_TO_NS(elapsed_cycles);

    u16_t dist_mm = elapsed_ns / CONFIG_JSN_SR04T_NS_PER_MM;

    drv_data->value.val1 = dist_mm / 1000;
    drv_data->value.val2 = (dist_mm % 1000) * 1000;

    cleanup:

    gpio_pin_disable_callback(drv_data->gpio, CONFIG_JSN_SR04T_ECHO_GPIO_PIN_NUM);
    drv_data->state = STATE_READY;

    device_busy_clear(dev);

    return err;
}

static int sr04t_channel_get(struct device* dev,
                             enum sensor_channel chan,
                             struct sensor_value* val) {
    struct sr04t_data* drv_data = dev->driver_data;

    if (chan != SENSOR_CHAN_DISTANCE)
        return -ENOTSUP;

    memcpy(val, &drv_data->value, sizeof(drv_data->value));

    return 0;
}

static int sr04t_pm_control(struct device* dev, u32_t cmd, void* context, device_pm_cb cb, void *arg) {
    int err = 0;
    struct sr04t_data* drv_data = dev->driver_data;

    switch (cmd) {
        case DEVICE_PM_SET_POWER_STATE:
            switch (*(u32_t*) context) {
                case DEVICE_PM_ACTIVE_STATE:
                    if (drv_data->state == STATE_OFF) {
                        IF_ERR(gpio_pin_write(drv_data->gpio, CONFIG_JSN_SR04T_EN_GPIO_PIN_NUM, 1)) goto cleanup;
                        // Wait for device to start
                        k_sleep(K_MSEC(20));
                        drv_data->state = STATE_READY;
                    }
                    break;
                case DEVICE_PM_OFF_STATE:
                    if (drv_data->state != STATE_OFF) {
                        IF_ERR(gpio_pin_write(drv_data->gpio, CONFIG_JSN_SR04T_EN_GPIO_PIN_NUM, 0)) goto cleanup;
                        drv_data->state = STATE_OFF;
                    }
                    break;
                default:
                    err = -EINVAL;
                    goto cleanup;
            }
            break;
        case DEVICE_PM_GET_POWER_STATE:
            *(u32_t*) context = drv_data->state == STATE_OFF ?
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
    int err;

    struct sr04t_data* drv_data = dev->driver_data;

    drv_data->state = STATE_OFF;

    k_sem_init(&drv_data->echo_end_sem, 0, 1);

    drv_data->gpio = device_get_binding(CONFIG_JSN_SR04T_GPIO_DEV_NAME);
    if (drv_data->gpio == NULL) {
        LOG_ERR("Failed to get GPIO device");
        return -EINVAL;
    }

    // Trig
    RET_ERR(gpio_pin_configure(drv_data->gpio, CONFIG_JSN_SR04T_TRIG_GPIO_PIN_NUM, GPIO_DIR_OUT));
    RET_ERR(gpio_pin_write(drv_data->gpio, CONFIG_JSN_SR04T_TRIG_GPIO_PIN_NUM, 0));

    // Echo
    RET_ERR(gpio_pin_configure(drv_data->gpio, CONFIG_JSN_SR04T_ECHO_GPIO_PIN_NUM,
                               GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE));
    gpio_init_callback(&drv_data->echo_cb, sr04t_echo_interrupt,
                       BIT(CONFIG_JSN_SR04T_ECHO_GPIO_PIN_NUM));
    RET_ERR(gpio_add_callback(drv_data->gpio, &drv_data->echo_cb));

    // Enable
    RET_ERR(gpio_pin_configure(drv_data->gpio, CONFIG_JSN_SR04T_EN_GPIO_PIN_NUM, GPIO_DIR_OUT));
    RET_ERR(gpio_pin_write(drv_data->gpio, CONFIG_JSN_SR04T_EN_GPIO_PIN_NUM, 0));

    return 0;
}

const struct sensor_driver_api sr04t_api = {
        .sample_fetch = &sr04t_sample_fetch,
        .channel_get = &sr04t_channel_get,
};

struct sr04t_data sr04t_data;

DEVICE_DEFINE(sr04t_dev, CONFIG_JSN_SR04T_NAME, &sr04t_init, &sr04t_pm_control, &sr04t_data,
              NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &sr04t_api);