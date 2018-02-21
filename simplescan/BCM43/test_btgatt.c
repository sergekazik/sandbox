/*
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2014  Google Inc.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include "lib/bluetooth.h"
#include "lib/hci.h"
#include "lib/hci_lib.h"
#include "lib/l2cap.h"
#include "lib/uuid.h"

#include "src/shared/mainloop.h"
#include "src/shared/util.h"
#include "src/shared/att.h"
#include "src/shared/queue.h"
#include "src/shared/timeout.h"
#include "src/shared/gatt-server.h"
#include "pthread.h"

// Note: differ from original gatt-db.h - modified for Ring Pairing
#include "gatt_db.h"

#define UUID_GAP			0x1800
#define UUID_GATT			0x1801
#define UUID_RING			0xFACE

#define UUID_HEART_RATE             0x180d
#define UUID_HEART_RATE_MSRMT		0x2a37
#define UUID_HEART_RATE_BODY		0x2a38
#define UUID_HEART_RATE_CTRL		0x2a39

#define ATT_CID 4

#define PRLOG(...) \
    do { \
        printf(__VA_ARGS__); \
        print_prompt(); \
    } while (0)

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define COLOR_OFF	"\x1B[0m"
#define COLOR_RED	"\x1B[0;91m"
#define COLOR_GREEN	"\x1B[0;92m"
#define COLOR_YELLOW	"\x1B[0;93m"
#define COLOR_BLUE	"\x1B[0;94m"
#define COLOR_MAGENTA	"\x1B[0;95m"
#define COLOR_BOLDGRAY	"\x1B[1;30m"
#define COLOR_BOLDWHITE	"\x1B[1;37m"

static const char test_device_name[] = "RingattskiBluez";
static bool verbose = false;

struct server {
    int fd;
    struct bt_att *att;
    struct gatt_db *db;
    struct bt_gatt_server *gatt;

    uint8_t *device_name;
    size_t name_len;

    uint16_t ring_svc_handle;
};

static void print_prompt(void)
{
    printf(COLOR_BLUE "[GATT server]" COLOR_OFF "# ");
    fflush(stdout);
}

static void att_disconnect_cb(int err, void *user_data)
{
    printf("Device disconnected: %s\n", strerror(err));

    mainloop_quit();
}

static void att_debug_cb(const char *str, void *user_data)
{
    const char *prefix = user_data;

    PRLOG(COLOR_BOLDGRAY "%s" COLOR_BOLDWHITE "%s\n" COLOR_OFF, prefix,
                                    str);
}

static void gatt_debug_cb(const char *str, void *user_data)
{
    const char *prefix = user_data;

    PRLOG(COLOR_GREEN "%s%s\n" COLOR_OFF, prefix, str);
}

static void gap_device_name_read_cb(struct gatt_db_attribute *attrib,
                    unsigned int id, uint16_t offset,
                    uint8_t opcode, struct bt_att *att,
                    void *user_data)
{
    struct server *server = user_data;
    uint8_t error = 0;
    size_t len = 0;
    const uint8_t *value = NULL;

    PRLOG("GAP Device Name Read called\n");

    len = server->name_len;

    if (offset > len) {
        error = BT_ATT_ERROR_INVALID_OFFSET;
        goto done;
    }

    len -= offset;
    value = len ? &server->device_name[offset] : NULL;

done:
    gatt_db_attribute_read_result(attrib, id, error, value, len);
}

static void gap_device_name_write_cb(struct gatt_db_attribute *attrib,
                    unsigned int id, uint16_t offset,
                    const uint8_t *value, size_t len,
                    uint8_t opcode, struct bt_att *att,
                    void *user_data)
{
    struct server *server = user_data;
    uint8_t error = 0;

    PRLOG("GAP Device Name Write called\n");

    /* If the value is being completely truncated, clean up and return */
    if (!(offset + len)) {
        free(server->device_name);
        server->device_name = NULL;
        server->name_len = 0;
        goto done;
    }

    /* Implement this as a variable length attribute value. */
    if (offset > server->name_len) {
        error = BT_ATT_ERROR_INVALID_OFFSET;
        goto done;
    }

    if (offset + len != server->name_len) {
        uint8_t *name;

        name = realloc(server->device_name, offset + len);
        if (!name) {
            error = BT_ATT_ERROR_INSUFFICIENT_RESOURCES;
            goto done;
        }

        server->device_name = name;
        server->name_len = offset + len;
    }

    if (value)
        memcpy(server->device_name + offset, value, len);

done:
    gatt_db_attribute_write_result(attrib, id, error);
}

static void gap_device_name_ext_prop_read_cb(struct gatt_db_attribute *attrib,
                    unsigned int id, uint16_t offset,
                    uint8_t opcode, struct bt_att *att,
                    void *user_data)
{
    uint8_t value[2];

    PRLOG("Device Name Extended Properties Read called\n");

    value[0] = BT_GATT_CHRC_EXT_PROP_RELIABLE_WRITE;
    value[1] = 0;

    gatt_db_attribute_read_result(attrib, id, 0, value, sizeof(value));
}

static void confirm_write(struct gatt_db_attribute *attr, int err,
                            void *user_data)
{
    if (!err)
        return;

    fprintf(stderr, "Error caching attribute %p - err: %d\n", attr, err);
    exit(1);
}

static void populate_gap_service(struct server *server)
{
    bt_uuid_t uuid;
    struct gatt_db_attribute *service, *tmp;
    uint16_t appearance;

    /* Add the GAP service */
    bt_uuid16_create(&uuid, UUID_GAP);
    service = gatt_db_add_service(server->db, &uuid, true, 6);

    /*
     * Device Name characteristic. Make the value dynamically read and
     * written via callbacks.
     */
    bt_uuid16_create(&uuid, GATT_CHARAC_DEVICE_NAME);
    gatt_db_service_add_characteristic(service, &uuid,
                    BT_ATT_PERM_READ | BT_ATT_PERM_WRITE,
                    BT_GATT_CHRC_PROP_READ |
                    BT_GATT_CHRC_PROP_EXT_PROP,
                    gap_device_name_read_cb,
                    gap_device_name_write_cb,
                    server);

    bt_uuid16_create(&uuid, GATT_CHARAC_EXT_PROPER_UUID);
    gatt_db_service_add_descriptor(service, &uuid, BT_ATT_PERM_READ,
                    gap_device_name_ext_prop_read_cb,
                    NULL, server);

    /*
     * Appearance characteristic. Reads and writes should obtain the value
     * from the database.
     */
    bt_uuid16_create(&uuid, GATT_CHARAC_APPEARANCE);
    tmp = gatt_db_service_add_characteristic(service, &uuid,
                            BT_ATT_PERM_READ,
                            BT_GATT_CHRC_PROP_READ,
                            NULL, NULL, server);

    /*
     * Write the appearance value to the database, since we're not using a
     * callback.
     */
    put_le16(128, &appearance);
    gatt_db_attribute_write(tmp, 0, (void *) &appearance,
                            sizeof(appearance),
                            BT_ATT_OP_WRITE_REQ,
                            NULL, confirm_write,
                            NULL);

    gatt_db_service_set_active(service, true);
}

#define RING_PAIRING_TABLE_ATTR_ENUM
enum GattAttributeIndexByName {
    #include "gatt_svc_defs.h"
};

typedef struct _CharacteristicInfo {
    uint16_t handle;
    char *name;
} CharacteristicInfo_t;

#define BCM_DEFINE_CHAR_INFO_LIST
static CharacteristicInfo_t sChInfoList[] =
{
    #include "gatt_svc_defs.h"
};

static void ring_characteristic_read_cb(struct gatt_db_attribute *attrib,
                    unsigned int id, uint16_t offset,
                    uint8_t opcode, struct bt_att *att,
                    void *user_data)
{
    struct server *server = user_data;

    uint16_t handle = gatt_db_attribute_get_handle(attrib);
    for (int idx = 0; idx < sizeof(sChInfoList)/sizeof(CharacteristicInfo_t); idx++)
    {
        if (sChInfoList[idx].handle == handle)
        {
            PRLOG("ring_characteristic_read_cb called for [%s]\n", sChInfoList[idx].name);

            if (GET_PAIRING_STATE == idx)
            {
                uint8_t value[] = {"OK-notify"};
                bt_gatt_server_send_notification(server->gatt, handle, value, sizeof(value));
            }

            return;
        }
    }
    PRLOG("ring_characteristic_read_cb UNKNOWN for id %d ?\n", id);
}
static void ring_characteristic_write_cb(struct gatt_db_attribute *attrib,
                    unsigned int id, uint16_t offset,
                    const uint8_t *value, size_t len,
                    uint8_t opcode, struct bt_att *att,
                    void *user_data)
{
    uint16_t handle = gatt_db_attribute_get_handle(attrib);
    for (int idx = 0; idx < sizeof(sChInfoList)/sizeof(CharacteristicInfo_t); idx++)
    {
        if (sChInfoList[idx].handle == handle)
        {
            PRLOG("ring_characteristic_write_cb called for [%s] for %d bytes, [%s]\n", sChInfoList[idx].name, len, (char*) value);
            return;
        }
    }
    PRLOG("ring_characteristic_write_cb UNKNOWN called for id %d with %d bytes, [%s]\n", id, len, (char*) value);
}

static void add_ring_characteristic(struct gatt_db_attribute *ringsvc, int handle_idx, uint128_t u, void* user_data, unsigned int pld_size, unsigned char *pld)
{
    bt_uuid_t uuid;
    bt_uuid128_create(&uuid, u);

    struct gatt_db_attribute *attr_new =  gatt_db_service_add_characteristic(ringsvc, &uuid,
                                          BT_ATT_PERM_READ | BT_ATT_PERM_WRITE,
                                          BT_GATT_CHRC_PROP_READ | BT_GATT_CHRC_PROP_WRITE | BT_GATT_CHRC_PROP_NOTIFY,
                                          NULL, NULL, // don't overwrite callbacks for read/write
                                          user_data);

    sChInfoList[handle_idx].handle = gatt_db_attribute_get_handle(attr_new);

    if (pld_size > 0)
    {
        // printf("add_ring_characteristic write %d bytes payload [%s]\n", pld_size, pld);
        gatt_db_attribute_write(attr_new, 0, (void *) pld, pld_size,
                                BT_ATT_OP_WRITE_REQ,
                                NULL, confirm_write, NULL);
    }
}

static void populate_ring_service(struct server *server)
{
    bt_uuid_t uuid;
    struct gatt_db_attribute *ringsvc;

    /* Add the RING ervice */
    static uint128_t RING_PAIRING_SVC = {{0x00,0x00,0xFA,0xCE,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5F,0x9B,0x34,0xFB}};
    bt_uuid128_create(&uuid, RING_PAIRING_SVC);
    ringsvc = gatt_db_add_service_ext(server->db, &uuid, true, 32, ring_characteristic_read_cb, ring_characteristic_write_cb);
    server->ring_svc_handle = gatt_db_attribute_get_handle(ringsvc);

    #define DECLARE_BCM_RING_CHARACTERISTIC_UUID
    #include "gatt_svc_defs.h"

    #define ADD_BCM_RING_CHARACTERISTICS
    #include "gatt_svc_defs.h"

    bool ret = gatt_db_service_set_active(ringsvc, true);
    printf("populate_ring_service %d, ret %d\n", __LINE__, ret);
}

static void populate_db(struct server *server)
{
    populate_gap_service(server);
    populate_ring_service(server);
}

static struct server *server_create(int fd, uint16_t mtu)
{
    struct server *server;
    size_t name_len = strlen(test_device_name);

    server = new0(struct server, 1);
    if (!server) {
        fprintf(stderr, "Failed to allocate memory for server\n");
        return NULL;
    }

    server->att = bt_att_new(fd, false);
    if (!server->att) {
        fprintf(stderr, "Failed to initialze ATT transport layer\n");
        goto fail;
    }

    if (!bt_att_set_close_on_unref(server->att, true)) {
        fprintf(stderr, "Failed to set up ATT transport layer\n");
        goto fail;
    }

    if (!bt_att_register_disconnect(server->att, att_disconnect_cb, NULL,
                                    NULL)) {
        fprintf(stderr, "Failed to set ATT disconnect handler\n");
        goto fail;
    }

    server->name_len = name_len + 1;
    server->device_name = malloc(name_len + 1);
    if (!server->device_name) {
        fprintf(stderr, "Failed to allocate memory for device name\n");
        goto fail;
    }

    memcpy(server->device_name, test_device_name, name_len);
    server->device_name[name_len] = '\0';

    server->fd = fd;
    server->db = gatt_db_new();
    if (!server->db) {
        fprintf(stderr, "Failed to create GATT database\n");
        goto fail;
    }

    server->gatt = bt_gatt_server_new(server->db, server->att, mtu);
    if (!server->gatt) {
        fprintf(stderr, "Failed to create GATT server\n");
        goto fail;
    }

    if (verbose) {
        bt_att_set_debug(server->att, att_debug_cb, "att: ", NULL);
        bt_gatt_server_set_debug(server->gatt, gatt_debug_cb,
                            "server: ", NULL);
    }

    /* Random seed for generating fake Heart Rate measurements */
    srand(time(NULL));

    /* bt_gatt_server already holds a reference */
    populate_db(server);

    return server;

fail:
    gatt_db_unref(server->db);
    free(server->device_name);
    bt_att_unref(server->att);
    free(server);

    return NULL;
}

static void server_destroy(struct server *server)
{
    bt_gatt_server_unref(server->gatt);
    gatt_db_unref(server->db);
}

typedef int (*cb_on_accept) (int fd);
static void* l2cap_le_att_listen_and_accept(void *data)
{
    int sk, nsk;
    struct sockaddr_l2 srcaddr, addr;
    socklen_t optlen;
    struct bt_security btsec;
    char ba[18];

    bdaddr_t src_addr;
    int dev_id = -1;
    int sec = BT_SECURITY_LOW;
    uint8_t src_type = BDADDR_LE_PUBLIC;
    cb_on_accept cb_function_pointer = (cb_on_accept) data;

    if (dev_id == -1)
        bacpy(&src_addr, BDADDR_ANY);
    else if (hci_devba(dev_id, &src_addr) < 0) {
        perror("Adapter not available");
        return (void*) EXIT_FAILURE;
    }

    sk = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (sk < 0) {
        perror("Failed to create L2CAP socket");
        return (void*) -1;
    }

    /* Set up source address */
    memset(&srcaddr, 0, sizeof(srcaddr));
    srcaddr.l2_family = AF_BLUETOOTH;
    srcaddr.l2_cid = htobs(ATT_CID);
    srcaddr.l2_bdaddr_type = src_type;
    bacpy(&srcaddr.l2_bdaddr, &src_addr);

    if (bind(sk, (struct sockaddr *) &srcaddr, sizeof(srcaddr)) < 0) {
        perror("Failed to bind L2CAP socket");
        goto fail;
    }

    /* Set the security level */
    memset(&btsec, 0, sizeof(btsec));
    btsec.level = sec;
    if (setsockopt(sk, SOL_BLUETOOTH, BT_SECURITY, &btsec,
                            sizeof(btsec)) != 0) {
        fprintf(stderr, "Failed to set L2CAP security level\n");
        goto fail;
    }

    if (listen(sk, 10) < 0) {
        perror("Listening on socket failed");
        goto fail;
    }

    printf("Started listening on ATT channel. Waiting for connections\n");

    memset(&addr, 0, sizeof(addr));
    optlen = sizeof(addr);
    nsk = accept(sk, (struct sockaddr *) &addr, &optlen);
    if (nsk < 0) {
        perror("Accept failed");
        goto fail;
    }

    ba2str(&addr.l2_bdaddr, ba);
    printf("Connect from %s\n", ba);
    close(sk);

    (*(cb_function_pointer))(nsk);

    pthread_exit(NULL);
    return (void*) 0;

fail:
    close(sk);
    pthread_exit(NULL);
    return (void*) -1;
}

static void notify_usage(void)
{
    printf("Usage: notify [options] <value_handle> <value>\n"
                    "Options:\n"
                    "\t -i, --indicate\tSend indication\n"
                    "e.g.:\n"
                    "\tnotify 0x0001 00 01 00\n");
}

static struct option notify_options[] = {
    { "indicate",	0, 0, 'i' },
    { }
};

static bool parse_args(char *str, int expected_argc,  char **argv, int *argc)
{
    char **ap;

    for (ap = argv; (*ap = strsep(&str, " \t")) != NULL;) {
        if (**ap == '\0')
            continue;

        (*argc)++;
        ap++;

        if (*argc > expected_argc)
            return false;
    }

    return true;
}

static void conf_cb(void *user_data)
{
    PRLOG("Received confirmation\n");
}

static void cmd_notify(struct server *server, char *cmd_str)
{
    int opt, i;
    char *argvbuf[516];
    char **argv = argvbuf;
    int argc = 1;
    uint16_t handle;
    char *endptr = NULL;
    int length;
    uint8_t *value = NULL;
    bool indicate = false;

    if (!parse_args(cmd_str, 514, argv + 1, &argc)) {
        printf("Too many arguments\n");
        notify_usage();
        return;
    }

    optind = 0;
    argv[0] = "notify";
    while ((opt = getopt_long(argc, argv, "+i", notify_options,
                                NULL)) != -1) {
        switch (opt) {
        case 'i':
            indicate = true;
            break;
        default:
            notify_usage();
            return;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc < 1) {
        notify_usage();
        return;
    }

    handle = strtol(argv[0], &endptr, 16);
    if (!endptr || *endptr != '\0' || !handle) {
        printf("Invalid handle: %s\n", argv[0]);
        return;
    }

    length = argc - 1;

    if (length > 0) {
        if (length > UINT16_MAX) {
            printf("Value too long\n");
            return;
        }

        value = malloc(length);
        if (!value) {
            printf("Failed to construct value\n");
            return;
        }

        for (i = 1; i < argc; i++) {
            if (strlen(argv[i]) != 2) {
                printf("Invalid value byte: %s\n",
                                argv[i]);
                goto done;
            }

            value[i-1] = strtol(argv[i], &endptr, 16);
            if (endptr == argv[i] || *endptr != '\0'
                            || errno == ERANGE) {
                printf("Invalid value byte: %s\n",
                                argv[i]);
                goto done;
            }
        }
    }

    if (indicate) {
        if (!bt_gatt_server_send_indication(server->gatt, handle,
                            value, length,
                            conf_cb, NULL, NULL))
            printf("Failed to initiate indication\n");
    } else if (!bt_gatt_server_send_notification(server->gatt, handle,
                                value, length))
        printf("Failed to initiate notification\n");

done:
    free(value);
}

static void print_uuid(const bt_uuid_t *uuid)
{
    char uuid_str[MAX_LEN_UUID_STR];
    bt_uuid_t uuid128;

    bt_uuid_to_uuid128(uuid, &uuid128);
    bt_uuid_to_string(&uuid128, uuid_str, sizeof(uuid_str));

    printf("%s\n", uuid_str);
}

static void print_incl(struct gatt_db_attribute *attr, void *user_data)
{
    struct server *server = user_data;
    uint16_t handle, start, end;
    struct gatt_db_attribute *service;
    bt_uuid_t uuid;

    if (!gatt_db_attribute_get_incl_data(attr, &handle, &start, &end))
        return;

    service = gatt_db_get_attribute(server->db, start);
    if (!service)
        return;

    gatt_db_attribute_get_service_uuid(service, &uuid);

    printf("\t  " COLOR_GREEN "include" COLOR_OFF " - handle: "
                    "0x%04x, - start: 0x%04x, end: 0x%04x,"
                    "uuid: ", handle, start, end);
    print_uuid(&uuid);
}

static void print_desc(struct gatt_db_attribute *attr, void *user_data)
{
    printf("\t\t  " COLOR_MAGENTA "descr" COLOR_OFF
                    " - handle: 0x%04x, uuid: ",
                    gatt_db_attribute_get_handle(attr));
    print_uuid(gatt_db_attribute_get_type(attr));
}

static void print_chrc(struct gatt_db_attribute *attr, void *user_data)
{
    uint16_t handle, value_handle;
    uint8_t properties;
    uint16_t ext_prop;
    bt_uuid_t uuid;

    if (!gatt_db_attribute_get_char_data(attr, &handle,
                                &value_handle,
                                &properties,
                                &ext_prop,
                                &uuid))
        return;

    printf("\t  " COLOR_YELLOW "charac" COLOR_OFF
                " - start: 0x%04x, value: 0x%04x, "
                "props: 0x%02x, ext_prop: 0x%04x, uuid: ",
                handle, value_handle, properties, ext_prop);
    print_uuid(&uuid);

    gatt_db_service_foreach_desc(attr, print_desc, NULL);
}

static void print_service(struct gatt_db_attribute *attr, void *user_data)
{
    struct server *server = user_data;
    uint16_t start, end;
    bool primary;
    bt_uuid_t uuid;

    if (!gatt_db_attribute_get_service_data(attr, &start, &end, &primary,
                                    &uuid))
        return;

    printf(COLOR_RED "service" COLOR_OFF " - start: 0x%04x, "
                "end: 0x%04x, type: %s, uuid: ",
                start, end, primary ? "primary" : "secondary");
    print_uuid(&uuid);

    gatt_db_service_foreach_incl(attr, print_incl, server);
    gatt_db_service_foreach_char(attr, print_chrc, NULL);

    printf("\n");
}

static void cmd_services(struct server *server, char *cmd_str)
{
    gatt_db_foreach_service(server->db, NULL, print_service, server);
}

static bool convert_sign_key(char *optarg, uint8_t key[16])
{
    int i;

    if (strlen(optarg) != 32) {
        printf("sign-key length is invalid\n");
        return false;
    }

    for (i = 0; i < 16; i++) {
        if (sscanf(optarg + (i * 2), "%2hhx", &key[i]) != 1)
            return false;
    }

    return true;
}

static void set_sign_key_usage(void)
{
    printf("Usage: set-sign-key [options]\nOptions:\n"
        "\t -c, --sign-key <remote csrk>\tRemote CSRK\n"
        "e.g.:\n"
        "\tset-sign-key -c D8515948451FEA320DC05A2E88308188\n");
}

static bool remote_counter(uint32_t *sign_cnt, void *user_data)
{
    static uint32_t cnt = 0;

    if (*sign_cnt < cnt)
        return false;

    cnt = *sign_cnt;

    return true;
}

static void cmd_set_sign_key(struct server *server, char *cmd_str)
{
    char *argv[3];
    int argc = 0;
    uint8_t key[16];

    memset(key, 0, 16);

    if (!parse_args(cmd_str, 2, argv, &argc)) {
        set_sign_key_usage();
        return;
    }

    if (argc != 2) {
        set_sign_key_usage();
        return;
    }

    if (!strcmp(argv[0], "-c") || !strcmp(argv[0], "--sign-key")) {
        if (convert_sign_key(argv[1], key))
            bt_att_set_remote_key(server->att, key, remote_counter,
                                    server);
    } else
        set_sign_key_usage();
}

static void cmd_help(struct server *server, char *cmd_str);

typedef void (*command_func_t)(struct server *server, char *cmd_str);

static struct {
    char *cmd;
    command_func_t func;
    char *doc;
} command[] = {
    { "help", cmd_help, "\tDisplay help message" },
    { "notify", cmd_notify, "\tSend handle-value notification" },
    { "services", cmd_services, "\tEnumerate all services" },
    { "set-sign-key", cmd_set_sign_key,
            "\tSet remote signing key for signed write command"},
    { }
};

static void cmd_help(struct server *server, char *cmd_str)
{
    int i;

    printf("Commands:\n");
    for (i = 0; command[i].cmd; i++)
        printf("\t%-15s\t%s\n", command[i].cmd, command[i].doc);
}

static void prompt_read_cb(int fd, uint32_t events, void *user_data)
{
    ssize_t read;
    size_t len = 0;
    char *line = NULL;
    char *cmd = NULL, *args;
    struct server *server = user_data;
    int i;

    if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        mainloop_quit();
        return;
    }

    read = getline(&line, &len, stdin);
    if (read < 0)
        return;

    if (read <= 1) {
        cmd_help(server, NULL);
        print_prompt();
        return;
    }

    line[read-1] = '\0';
    args = line;

    while ((cmd = strsep(&args, " \t")))
        if (*cmd != '\0')
            break;

    if (!cmd)
        goto failed;

    for (i = 0; command[i].cmd; i++) {
        if (strcmp(command[i].cmd, cmd) == 0)
            break;
    }

    if (command[i].cmd)
        command[i].func(server, args);
    else
        fprintf(stderr, "Unknown command: %s\n", line);

failed:
    print_prompt();

    free(line);
}

static void signal_cb(int signum, void *user_data)
{
    (void) user_data;
    switch (signum) {
    case SIGINT:
    case SIGTERM:
        mainloop_quit();
        break;
    default:
        break;
    }
}

int cb_on_alisten_accept (int fd)
{
    sigset_t mask;
    struct server *server;

    if (fd < 0) {
        fprintf(stderr, "Failed to accept L2CAP ATT connection\n");
        return EXIT_FAILURE;
    }

    mainloop_init();

    server = server_create(fd, 0);
    if (!server) {
        close(fd);
        return EXIT_FAILURE;
    }

    if (mainloop_add_fd(fileno(stdin),
                EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR,
                prompt_read_cb, server, NULL) < 0) {
        fprintf(stderr, "Failed to initialize console\n");
        server_destroy(server);
        return EXIT_FAILURE;
    }

    printf("Running GATT server\n");

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    mainloop_set_signal(&mask, signal_cb, NULL, NULL);

    print_prompt();

    mainloop_run();

    printf("\n\nShutting down...\n");
    server_destroy(server);
    printf("\n\nserver_destroyed...\n");
    return 0;
}

int main(int argc, char *argv[])
{
    verbose = true;

    int rc;
    pthread_t thread_id;
    printf("=== l2cap_le_att_listen_and_accept\n");
    rc =  pthread_create(&thread_id, NULL, l2cap_le_att_listen_and_accept, (void *) cb_on_alisten_accept);

    if(rc) {
        printf("=== Failed to create thread\n");
        exit(-3);
    }

    pthread_join(thread_id, NULL);
    printf("=== main exiting\n");

    pthread_exit(NULL);

    return EXIT_SUCCESS;
}
