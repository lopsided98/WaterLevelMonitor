#pragma once

#include <power/reboot.h>

enum watchdog_reset_reason {
    RESET_POWER = 0,
    RESET_MANUAL = SYS_REBOOT_COLD,
    RESET_PANIC,
    RESET_WATCHDOG,
    RESET_BROWNOUT
};

int watchdog_init(void);

enum watchdog_reset_reason watchdog_get_reset_reason();