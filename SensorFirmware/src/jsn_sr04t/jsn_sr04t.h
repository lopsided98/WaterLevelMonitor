#pragma once

#include <device.h>
#include <sensor.h>

enum sr04t_state {
    STATE_OFF = 0,
    STATE_READY,
    STATE_WAIT_ECHO_START,
    STATE_WAIT_ECHO_END
};

struct sr04t_data {
    struct device* gpio;
    struct gpio_callback echo_cb;
    struct k_sem echo_end_sem;

    enum sr04t_state state;

    u32_t echo_start_cycles;
    u32_t echo_end_cycles;
    struct sensor_value value;
};
