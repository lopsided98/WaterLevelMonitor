#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include "bluetooth.h"

#define BT_CPF_FORMAT_UINT16 0x6

// Water level service (WLS)
#define BT_UUID_WLS_VAL \
    0x01, 0xc7, 0xe3, 0x00, 0xfa, 0xaf, 0x11, 0xe8, \
    0x8b, 0xe5, 0x07, 0x9b, 0x30, 0x95, 0xdd, 0x67

#define BT_UUID_WLS BT_UUID_DECLARE_128(BT_UUID_WLS_VAL)

#define BT_UUID_WLS_WATER_LEVEL_VAL \
    0x01, 0xc7, 0xe3, 0x00, 0xfa, 0xaf, 0x11, 0xe8, \
    0x8b, 0xe5, 0x07, 0x9b, 0x30, 0x95, 0xdd, 0x67

#define BT_UUID_WLS_WATER_LEVEL BT_UUID_DECLARE_128(BT_UUID_WLS_WATER_LEVEL_VAL)

static uint8_t battery_level = 87;
static uint16_t water_level = 233;

static void bluetooth_ready(int err);

static ssize_t bluetooth_battery_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                      void* buf, u16_t len, u16_t offset);

static ssize_t bluetooth_water_level_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                          void* buf, u16_t len, u16_t offset);

static const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                      0x0F, 0x18 // Battery Service (0x180F)
        )
};

static const struct bt_data sd[] = {
        BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_WLS_VAL)
};

static struct bt_gatt_attr bas_attrs[] = {
        BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
        BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
                               BT_GATT_CHRC_READ,
                               BT_GATT_PERM_READ, bluetooth_battery_read, NULL, NULL),
};
static struct bt_gatt_service bas_service = BT_GATT_SERVICE(bas_attrs);

static const struct bt_gatt_cpf wls_water_level_cpf = {
        .format = BT_CPF_FORMAT_UINT16,
};

static struct bt_gatt_attr wls_attrs[] = {
        BT_GATT_PRIMARY_SERVICE(BT_UUID_WLS),
        BT_GATT_CHARACTERISTIC(BT_UUID_WLS_WATER_LEVEL,
                               BT_GATT_CHRC_READ,
                               BT_GATT_PERM_READ, bluetooth_water_level_read, NULL, NULL),
        BT_GATT_CPF(&wls_water_level_cpf)
};
static struct bt_gatt_service wls_service = BT_GATT_SERVICE(wls_attrs);

static void bluetooth_ready(int err) {
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");

    bt_gatt_service_register(&bas_service);
    bt_gatt_service_register(&wls_service);

    err = bt_le_adv_start(BT_LE_ADV_PARAM(
                                  BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
                                  BT_GAP_ADV_SLOW_INT_MIN,
                                  BT_GAP_ADV_SLOW_INT_MAX
                          ), ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}

static ssize_t bluetooth_battery_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                      void* buf, u16_t len, u16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &battery_level, sizeof(battery_level));
}

static ssize_t bluetooth_water_level_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                          void* buf, u16_t len, u16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &water_level, sizeof(water_level));
}

int bluetooth_init() {
    int err;

    if ((err = bt_enable(bluetooth_ready) < 0)) return err;

    return 0;
}

