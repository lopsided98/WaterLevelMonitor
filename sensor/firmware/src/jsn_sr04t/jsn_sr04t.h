#pragma once

#include <device.h>
#include <drivers/sensor.h>

enum sr04t_state {
    STATE_OFF = 0,
    STATE_READY,
    STATE_WAIT_ECHO_START,
    STATE_WAIT_ECHO_END
};

struct sr04t_config {
    char *trig_gpio_name;
	u8_t trig_gpio_pin;
	int trig_gpio_flags;
    char *echo_gpio_name;
    u8_t echo_gpio_pin;
    int echo_gpio_flags;
    char *en_gpio_name;
    u8_t en_gpio_pin;
    int en_gpio_flags;
};

struct sr04t_data {
    struct device* trig_gpio;
    struct device* echo_gpio;
    struct device* en_gpio;
    struct gpio_callback echo_cb;
    struct k_sem echo_end_sem;

    enum sr04t_state state;

    u32_t echo_start_cycles;
    u32_t echo_end_cycles;
    struct sensor_value value;
};
