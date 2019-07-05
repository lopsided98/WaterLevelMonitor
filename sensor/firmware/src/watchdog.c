#include <zephyr.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <power/reboot.h>

LOG_MODULE_REGISTER(watchdog);

int watchdog_init() {

}

FUNC_NORETURN void z_SysFatalErrorHandler(unsigned int reason, const NANO_ESF* pEsf) {
    LOG_ERR("FATAL ERROR... Resetting");
    LOG_PANIC();
    sys_reboot(SYS_REBOOT_COLD);
    CODE_UNREACHABLE;
}
