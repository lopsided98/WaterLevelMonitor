#pragma once

#include <device.h>
#include <sensor.h>

#define SR04T_ECHO_START_WAIT_DURATION	18000
#define SR04T_ECHO_MAX_WAIT_DURATION 100

enum sr04t_state {
    STATE_READY = 0,
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
