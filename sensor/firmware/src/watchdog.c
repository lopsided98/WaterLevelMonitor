#include <zephyr.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <power/reboot.h>
#include <hal/nrf_power.h>

#include "watchdog.h"
#include "bluetooth.h"

LOG_MODULE_REGISTER(watchdog);

static u8_t reset_reason = RESET_POWER;

int watchdog_init(void) {
    reset_reason = nrf_power_gpregret_get();

    if (reset_reason == RESET_BROWNOUT) {
        bluetooth_set_error(ERROR_BROWNOUT);
    }

    // Set up the retained register
    // Any type of reset except brownout will set GPREGRET to a different
    // value. This allows us to detect brownouts.
    nrf_power_gpregret_set(RESET_BROWNOUT);

    return 0;
}

enum watchdog_reset_reason watchdog_get_reset_reason() {
    return reset_reason;
}

FUNC_NORETURN void z_SysFatalErrorHandler(unsigned int reason, const NANO_ESF* pEsf) {
    LOG_ERR("FATAL ERROR... Resetting");
    LOG_PANIC();
    // NRF5x implementation passes reason to GPREGRET
    sys_reboot(RESET_PANIC);
    CODE_UNREACHABLE;
}