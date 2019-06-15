#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <logging/log.h>
#include <settings/settings.h>
#include "bluetooth.h"
#include "common.h"

LOG_MODULE_REGISTER(bluetooth);

#define BT_CPF_FORMAT_UINT16 0x6

// Water level service (WLS)
#define BT_UUID_WLS_VAL \
    0x01, 0xc7, 0xe3, 0x00, 0xfa, 0xaf, 0x11, 0xe8, \
    0x8b, 0xe5, 0x07, 0x9b, 0x30, 0x95, 0xdd, 0x67

static u8_t battery_level = 87;
static s16_t temperature = 0;
static u16_t water_level = 233;

static void bluetooth_ready(int err);

static ssize_t bluetooth_battery_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                      void* buf, u16_t len, u16_t offset);

static ssize_t bluetooth_temperature_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                          void* buf, u16_t len, u16_t offset);

static ssize_t bluetooth_water_level_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                          void* buf, u16_t len, u16_t offset);

static const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                      0x0F, 0x18, // Battery Service (0x180F)
                      0x1A, 0x18 // Environmental Sensing Service (0x181A)
        ),
        BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_WLS_VAL)
};

static const struct bt_data sd[] = {
        BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1)
};

BT_GATT_SERVICE_DEFINE(
        bas_service,
        BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
        BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
                               BT_GATT_CHRC_READ,
                               BT_GATT_PERM_READ, bluetooth_battery_read, NULL, NULL),
);

BT_GATT_SERVICE_DEFINE(
        ess_service,
        BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),
        BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
                               BT_GATT_CHRC_READ,
                               BT_GATT_PERM_READ, bluetooth_temperature_read, NULL, NULL),
);

static struct bt_uuid_128 bt_uuid_wls = BT_UUID_INIT_128(BT_UUID_WLS_VAL);

static struct bt_uuid_128 bt_uuid_wls_water_level = BT_UUID_INIT_128(
        0x01, 0xc7, 0xe3, 0x00, 0xfa, 0xaf, 0x11, 0xe8,
        0x8b, 0xe5, 0x07, 0x9b, 0x30, 0x95, 0xdd, 0x67
);

static const struct bt_gatt_cpf wls_water_level_cpf = {
        .format = BT_CPF_FORMAT_UINT16,
        .exponent = -3
};

BT_GATT_SERVICE_DEFINE(
        wls_service,
        BT_GATT_PRIMARY_SERVICE(&bt_uuid_wls),
        BT_GATT_CHARACTERISTIC(&bt_uuid_wls_water_level.uuid,
                               BT_GATT_CHRC_READ,
                               BT_GATT_PERM_READ, bluetooth_water_level_read, NULL, NULL),
        BT_GATT_CPF(&wls_water_level_cpf)
);

static void bluetooth_connected(struct bt_conn *conn, u8_t err) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_ERR("failed to connect to: %s (err %d)", addr, err);
        return;
    }

    LOG_INF("connected to: %s", addr);

    IF_ERR(bt_conn_security(conn, BT_SECURITY_FIPS)) {
        LOG_ERR("failed to enable security (err %d)", err);
    }
}

static struct bt_conn_cb conn_callbacks = {
        .connected = bluetooth_connected,
};

static void bluetooth_passkey_display(struct bt_conn* conn, unsigned int passkey) {
    LOG_INF("passkey: %d\n", passkey);
}

static void bluetooth_auth_cancel(struct bt_conn* conn) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_WRN("autentication failed with: %s", addr);
}

static void bluetooth_pairing_complete(struct bt_conn* conn, bool bonded) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("paired with: %s\n", addr);
}

static void bluetooth_pairing_failed(struct bt_conn* conn) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_WRN("pairing failed with: %s", addr);
    bt_conn_disconnect(conn, BT_HCI_ERR_AUTHENTICATION_FAIL);
}

static struct bt_conn_auth_cb auth_callbacks = {
        .passkey_display = bluetooth_passkey_display,
        .passkey_entry = NULL,
        .passkey_confirm = NULL,
        .cancel = bluetooth_auth_cancel,
        .pairing_complete = bluetooth_pairing_complete,
        .pairing_failed = bluetooth_pairing_failed,
};

static void bluetooth_ready(int err) {
    if (err) {
        LOG_ERR("initialization failed (err %d)\n", err);
        return;
    }

    LOG_DBG("initialized\n");

    // FIXME: why does this need to go here, rather than before bt_enable()?
    IF_ERR(settings_load()) {
        LOG_ERR("failed to load settings (err %d)\n", err);
    }

    IF_ERR(bt_passkey_set(CONFIG_BT_DEFAULT_PASSKEY)) {
        LOG_ERR("setting passkey failed (err %d)\n", err);
    }

    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_cb_register(&auth_callbacks);

    err = bt_le_adv_start(BT_LE_ADV_PARAM(
                                  BT_LE_ADV_OPT_CONNECTABLE,
                                  BT_GAP_ADV_SLOW_INT_MIN,
                                  BT_GAP_ADV_SLOW_INT_MAX
                          ), ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("advertising failed to start (err %d)\n", err);
        return;
    }

    LOG_DBG("advertising started\n");
}

static ssize_t bluetooth_battery_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                      void* buf, u16_t len, u16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &battery_level, sizeof(battery_level));
}

static ssize_t bluetooth_temperature_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                          void* buf, u16_t len, u16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &temperature, sizeof(temperature));
}

static ssize_t bluetooth_water_level_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                          void* buf, u16_t len, u16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &water_level, sizeof(water_level));
}

int bluetooth_init(void) {
    int err = 0;

    RET_ERR(bt_enable(bluetooth_ready));

    return err;
}

int bluetooth_set_battery_level(u8_t level) {
    battery_level = level;
    return 0;
}

int bluetooth_set_temperature(s16_t temp) {
    temperature = temp;
    return 0;
}

int bluetooth_set_water_level(u16_t level) {
    water_level = level;
    return 0;
}
