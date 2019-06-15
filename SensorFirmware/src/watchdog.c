#include <zephyr.h>
#include <logging/log.h>
#include <misc/reboot.h>

LOG_MODULE_REGISTER(watchdog);

int watchdog_init() {

}

FUNC_NORETURN void _SysFatalErrorHandler(unsigned int reason, const NANO_ESF *pEsf) {
    LOG_ERR("FATAL ERROR... Resetting");
    sys_reboot(SYS_REBOOT_COLD);
    CODE_UNREACHABLE;
}