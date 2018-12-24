#include <gpio.h>
#include <sensor.h>
#include <logging/log.h>

#include "../common.h"
#include "jsn_sr04t.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(JSN_SRO4T);

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
    struct sr04t_data* drv_data = dev->driver_data;
    int err = 0;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_DISTANCE);

    IF_ERR(gpio_pin_write(drv_data->gpio, CONFIG_JSN_SR04T_TRIG_GPIO_PIN_NUM, 1)) goto cleanup;
    k_busy_wait(50);
    IF_ERR(gpio_pin_write(drv_data->gpio, CONFIG_JSN_SR04T_TRIG_GPIO_PIN_NUM, 0)) goto cleanup;

    drv_data->state = STATE_WAIT_ECHO_START;

    k_sem_reset(&drv_data->echo_end_sem);

    IF_ERR(gpio_pin_enable_callback(drv_data->gpio, CONFIG_JSN_SR04T_ECHO_GPIO_PIN_NUM)) goto cleanup;

    // Wait for interrupt processing to finish
    while (1) {
        // Retry on busy
        err = k_sem_take(&drv_data->echo_end_sem, K_MSEC(CONFIG_JSN_SR04T_ECHO_TIMEOUT));
        if (!err) {
            break;
        } else if (err == -EAGAIN) {
            // Timeout
            err = -EIO;
            goto cleanup;
        }
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

    return err;
}

static int sr04t_channel_get(struct device* dev,
                             enum sensor_channel chan,
                             struct sensor_value* val) {
    struct sr04t_data* drv_data = dev->driver_data;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_DISTANCE);

    memcpy(val, &drv_data->value, sizeof(drv_data->value));

    return 0;
}

static int sr04t_init(struct device* dev) {
    int err;

    struct sr04t_data* drv_data = dev->driver_data;

    drv_data->state = STATE_READY;

    k_sem_init(&drv_data->echo_end_sem, 0, 1);

    drv_data->gpio = device_get_binding(CONFIG_JSN_SR04T_GPIO_DEV_NAME);
    if (drv_data->gpio == NULL) {
        LOG_ERR("Failed to get GPIO device");
        return -EINVAL;
    }

    RET_ERR(gpio_pin_configure(drv_data->gpio, CONFIG_JSN_SR04T_TRIG_GPIO_PIN_NUM, GPIO_DIR_OUT));
    RET_ERR(gpio_pin_configure(drv_data->gpio, CONFIG_JSN_SR04T_ECHO_GPIO_PIN_NUM,
                               GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE));

    RET_ERR(gpio_pin_write(drv_data->gpio, CONFIG_JSN_SR04T_TRIG_GPIO_PIN_NUM, 0));

    gpio_init_callback(&drv_data->echo_cb, sr04t_echo_interrupt,
                       BIT(CONFIG_JSN_SR04T_ECHO_GPIO_PIN_NUM));

    if ((err = gpio_add_callback(drv_data->gpio, &drv_data->echo_cb)) < 0) {
        LOG_ERR("Failed to add echo callback");
        return err;
    }

    return 0;
}

const struct sensor_driver_api sr04t_api = {
        .sample_fetch = &sr04t_sample_fetch,
        .channel_get = &sr04t_channel_get,
};

struct sr04t_data sr04t_data;

DEVICE_AND_API_INIT(sr04t_dev, CONFIG_JSN_SR04T_NAME, &sr04t_init, &sr04t_data,
                    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &sr04t_api);