#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <logging/log.h>
#include <settings/settings.h>
#include <mgmt/smp_bt.h>
#include "bluetooth.h"
#include "common.h"
#include "water_level.h"

LOG_MODULE_REGISTER(bluetooth);

#define BT_CPF_FORMAT_UINT16 0x6

// Water Level Service (WLS)
#define BT_UUID_WLS_VAL \
    0x01, 0xc7, 0xe3, 0x00, 0xfa, 0xaf, 0x11, 0xe8, \
    0x8b, 0xe5, 0x07, 0x9b, 0x30, 0x95, 0xdd, 0x67

// System Control Service (SCS)
#define BT_UUID_SCS_VAL \
    0xa1, 0xf8, 0x20, 0x0f, 0xa2, 0xc0, 0x4e, 0x72, \
    0x8e, 0x88, 0x61, 0x96, 0xcb, 0xdf, 0xef, 0x89

static u8_t battery_level = 0;
static s16_t temperature = 0;

static atomic_t error = ATOMIC_INIT(0);
static atomic_t status = ATOMIC_INIT(0);

static void bluetooth_ready(int err);

static ssize_t bluetooth_battery_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                      void* buf, u16_t len, u16_t offset);

static ssize_t bluetooth_temperature_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                          void* buf, u16_t len, u16_t offset);

static ssize_t bluetooth_water_level_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                          void* buf, u16_t len, u16_t offset);

static ssize_t bluetooth_water_distance_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                             void* buf, u16_t len, u16_t offset);

static ssize_t bluetooth_tank_depth_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                         void* buf, u16_t len, u16_t offset);

static ssize_t bluetooth_tank_depth_write(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                          const void* buf, u16_t len, u16_t offset, u8_t flags);

static ssize_t bluetooth_error_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                    void* buf, u16_t len, u16_t offset);

static ssize_t bluetooth_error_write(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                     const void* buf, u16_t len, u16_t offset, u8_t flags);

static ssize_t bluetooth_status_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                     void* buf, u16_t len, u16_t offset);

static ssize_t bluetooth_status_write(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                      const void* buf, u16_t len, u16_t offset, u8_t flags);

static const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                      0x0F, 0x18, // Battery Service (0x180F)
                      0x1A, 0x18 // Environmental Sensing Service (0x181A)
        ),
        BT_DATA_BYTES(BT_DATA_UUID128_SOME, BT_UUID_WLS_VAL)
};

static const struct bt_data sd[] = {
        BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1)
};

// Battery Service
BT_GATT_SERVICE_DEFINE(
        bas_service,
        BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
        BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
                               BT_GATT_CHRC_READ,
                               BT_GATT_PERM_READ_AUTHEN, bluetooth_battery_read, NULL, NULL),
);

// Environmental Sensing Service
BT_GATT_SERVICE_DEFINE(
        ess_service,
        BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),
        BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
                               BT_GATT_CHRC_READ,
                               BT_GATT_PERM_READ_AUTHEN, bluetooth_temperature_read, NULL, NULL),
);

// Water Level Service
static struct bt_uuid_128 bt_uuid_wls = BT_UUID_INIT_128(BT_UUID_WLS_VAL);

static struct bt_uuid_128 bt_uuid_wls_water_level = BT_UUID_INIT_128(
        0x24, 0x4c, 0xbc, 0x99, 0x57, 0x6d, 0x4e, 0x4a,
        0xb5, 0xa1, 0x9a, 0x72, 0xa5, 0xe6, 0xf2, 0x7a
);

static struct bt_uuid_128 bt_uuid_wls_water_distance = BT_UUID_INIT_128(
        0x01, 0x31, 0x2d, 0x06, 0x4f, 0xa7, 0x42, 0x94,
        0x82, 0x3b, 0x84, 0x47, 0x54, 0x55, 0x47, 0xfe
);

static struct bt_uuid_128 bt_uuid_wls_tank_depth = BT_UUID_INIT_128(
        0xd3, 0xc6, 0xec, 0xcb, 0x2f, 0x4a, 0x49, 0x8c,
        0xbf, 0xde, 0x5c, 0x76, 0x3d, 0xee, 0x57, 0xd3
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
                               BT_GATT_PERM_READ_AUTHEN,
                               bluetooth_water_level_read, NULL, NULL),
        BT_GATT_CPF(&wls_water_level_cpf),
        BT_GATT_CHARACTERISTIC(&bt_uuid_wls_water_distance.uuid,
                               BT_GATT_CHRC_READ,
                               BT_GATT_PERM_READ_AUTHEN,
                               bluetooth_water_distance_read, NULL, NULL),
        BT_GATT_CPF(&wls_water_level_cpf),
        BT_GATT_CHARACTERISTIC(&bt_uuid_wls_tank_depth.uuid,
                               BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                               BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
                               bluetooth_tank_depth_read, bluetooth_tank_depth_write, NULL),
        BT_GATT_CPF(&wls_water_level_cpf),
);

static struct bt_uuid_128 bt_uuid_scs = BT_UUID_INIT_128(BT_UUID_SCS_VAL);

static struct bt_uuid_128 bt_uuid_scs_error = BT_UUID_INIT_128(
        0xe3, 0xf1, 0x14, 0x77, 0xc3, 0xcb, 0x4a, 0xa7,
        0xbd, 0xc6, 0x7b, 0x84, 0x83, 0x2f, 0x5f, 0xc26
);

static struct bt_uuid_128 bt_uuid_scs_status = BT_UUID_INIT_128(
        0x5b, 0x32, 0xf5, 0x09, 0xf9, 0x61, 0x4b, 0x28,
        0x95, 0xc1, 0xd4, 0xed, 0xae, 0x5d, 0xc1, 0x57
);

BT_GATT_SERVICE_DEFINE(
        scs_service,
        BT_GATT_PRIMARY_SERVICE(&bt_uuid_scs),
        BT_GATT_CHARACTERISTIC(&bt_uuid_scs_error.uuid,
                               BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                               BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
                               bluetooth_error_read, bluetooth_error_write, NULL),
        BT_GATT_CHARACTERISTIC(&bt_uuid_scs_status.uuid,
                               BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                               BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
                               bluetooth_status_read, bluetooth_status_write, NULL),
);

static void bluetooth_connected(struct bt_conn* conn, u8_t err) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_ERR("Failed to connect to: %s (err %d)", log_strdup(addr), err);
        return;
    }

    LOG_DBG("Connected to: %s", log_strdup(addr));

//    IF_ERR(bt_conn_security(conn, BT_SECURITY_FIPS)) {
//        LOG_ERR("Failed to enable security (err %d)", err);
//        bt_conn_disconnect(conn, BT_HCI_ERR_INSUFFICIENT_SECURITY);
//    }
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

    LOG_WRN("Authentication failed with: %s", log_strdup(addr));
}

static void bluetooth_pairing_complete(struct bt_conn* conn, bool bonded) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Paired with: %s\n", log_strdup(addr));
}

static void bluetooth_pairing_failed(struct bt_conn* conn) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_WRN("Pairing failed with: %s", log_strdup(addr));
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
        LOG_ERR("Initialization failed (err %d)\n", err);
        return;
    }

    IF_ERR(settings_load()) {
        LOG_ERR("Loading settings failed (err %d)\n", err);
    }

    IF_ERR(bt_passkey_set(CONFIG_BT_DEFAULT_PASSKEY)) {
        LOG_ERR("Setting passkey failed (err %d)\n", err);
    }

    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_cb_register(&auth_callbacks);

    IF_ERR(bt_le_adv_start(BT_LE_ADV_PARAM(
            BT_LE_ADV_OPT_CONNECTABLE,
            BT_GAP_ADV_SLOW_INT_MIN,
            BT_GAP_ADV_SLOW_INT_MAX
    ), ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd))) {
        LOG_ERR("Advertising failed to start (err %d)\n", err);
        return;
    }

    LOG_DBG("Advertising started\n");
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
    const u16_t water_level = water_level_get();
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &water_level, sizeof(water_level));
}

static ssize_t bluetooth_water_distance_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                             void* buf, u16_t len, u16_t offset) {
    const u16_t water_distance = water_level_get_water_distance();
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &water_distance, sizeof(water_distance));
}

static ssize_t bluetooth_tank_depth_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                         void* buf, u16_t len, u16_t offset) {
    const u16_t tank_depth = water_level_get_tank_depth();
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &tank_depth, sizeof(tank_depth));
}

static ssize_t bluetooth_tank_depth_write(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                          const void* buf, u16_t len, u16_t offset, u8_t flags) {
    u16_t tank_depth = 0;

    if (offset + len > sizeof(tank_depth)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(((u8_t*) &tank_depth) + offset, buf, len);
    water_level_set_tank_depth(tank_depth);

    return len;
}

static ssize_t bluetooth_error_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                    void* buf, u16_t len, u16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &error, sizeof(error));
}

static ssize_t bluetooth_error_write(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                     const void* buf, u16_t len, u16_t offset, u8_t flags) {
    if (offset + len > sizeof(error)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    u32_t error_write = 0;
    u32_t mask_write = 0;
    memcpy(((u8_t*) &error_write) + offset, buf, len);
    memset(((u8_t*) &mask_write) + offset, 0xff, len);

    // Don't let the user set error bits, only clear
    atomic_and(&error, ~mask_write | error_write);

    return len;
}

static ssize_t bluetooth_status_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                     void* buf, u16_t len, u16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &status, sizeof(status));
}

static ssize_t bluetooth_status_write(struct bt_conn* conn, const struct bt_gatt_attr* attr,
                                      const void* buf, u16_t len, u16_t offset, u8_t flags) {
    if (offset + len > sizeof(status)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    u32_t status_write = 0;
    u32_t mask_write = 0;
    memcpy(((u8_t*) &status_write) + offset, buf, len);
    memset(((u8_t*) &mask_write) + offset, 0xff, len);

    // Only set bits specified by the write mask
    // FIXME: this is cheating because we don't use atomic operations. It
    //  should be fine though because this function is called from a
    //  cooperative thread.
    status = (status & ~mask_write) | status_write;

    // Stop advertising if the client confirms that it has read the data
//    if (!(status & STATUS_NEW_DATA)) {
//        bt_le_adv_stop();
//    }

    return len;
}

int bluetooth_init(void) {
    int err = 0;

    RET_ERR(bt_enable(bluetooth_ready));

    // Setup mcumgr over BLE
    RET_ERR(smp_bt_register());

    return 0;
}

int bluetooth_set_battery_level(u8_t level) {
    battery_level = level;
    return 0;
}

int bluetooth_set_temperature(s16_t temp) {
    temperature = temp;
    return 0;
}

void bluetooth_set_error(enum system_error e) {
    atomic_or(&error, e);
}

bool bluetooth_get_error(enum system_error e) {
    return error & e;
}

void bluetooth_set_status(enum system_status s, bool value) {
    if (value) {
        atomic_or(&status, s);
    } else {
        atomic_and(&status, ~s);
    }
}