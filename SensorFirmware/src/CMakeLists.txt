# FIXME: does not work due to: https://github.com/zephyrproject-rtos/zephyr/issues/8851
# add_subdirectory_if_kconfig(jsn_sr04t)

target_sources(app PRIVATE
        main.c
        bluetooth.c
        battery.c
        temperature.c
        water_level.c
        watchdog.c
        jsn_sr04t/jsn_sr04t.c
        nrfx_adc_supply/nrfx_adc_supply.c)