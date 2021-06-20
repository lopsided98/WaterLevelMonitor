#pragma once

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>

enum sr04t_state {
    STATE_OFF = 0,
    STATE_READY,
    STATE_WAIT_ECHO_START,
    STATE_WAIT_ECHO_END
};

struct sr04t_config {
    const struct gpio_dt_spec trig_gpio;
    const struct gpio_dt_spec echo_gpio;
    const struct gpio_dt_spec supply_gpio;
};

struct sr04t_data {
    struct gpio_callback echo_cb;
    struct k_sem echo_end_sem;

    enum sr04t_state state;

    uint32_t echo_start_cycles;
    uint32_t echo_end_cycles;
    struct sensor_value value;
};
