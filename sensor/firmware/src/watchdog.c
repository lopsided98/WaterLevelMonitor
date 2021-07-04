#include "watchdog.h"

#include <hal/nrf_power.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <power/reboot.h>
#include <zephyr.h>

#include "bluetooth.h"

LOG_MODULE_REGISTER(watchdog);

int watchdog_init(void) {
    uint32_t reset_reason = nrf_power_resetreas_get(NRF_POWER);
    nrf_power_resetreas_clear(NRF_POWER, 0xFFFFFFFF);

    if (reset_reason & (NRF_POWER_RESETREAS_SREQ_MASK | NRF_POWER_RESETREAS_LOCKUP_MASK)) {
        bluetooth_set_error(ERROR_CRASH);
    }

    return 0;
}

FUNC_NORETURN void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t* esf) {
    LOG_ERR("FATAL ERROR... Resetting");
    LOG_PANIC();
    // NRF51 implementation passes reason to GPREGRET
    sys_reboot(0);
    CODE_UNREACHABLE;
}
