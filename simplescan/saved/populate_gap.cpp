#if 0
static void gap_device_name_read_cb(struct gatt_db_attribute *attrib,
                    unsigned int id, uint16_t offset,
                    uint8_t opcode, struct bt_att *att,
                    void *user_data)
{
    (void) id, (void) offset, (void) opcode, (void) att,(void)user_data;

    uint8_t error = 0;
    const uint8_t *value = (const uint8_t *) "RingBLETST";
    size_t len = strlen((char*) value);
    gatt_db_attribute_read_result(attrib, id, error, value, len);
}

static void gap_device_name_write_cb(struct gatt_db_attribute *attrib,
                    unsigned int id, uint16_t offset,
                    const uint8_t *value, size_t len,
                    uint8_t opcode, struct bt_att *att,
                    void *user_data)
{
    (void) id, (void) offset, (void) opcode, (void) att,(void)user_data;
    (void) value, (void) len, (void) attrib;

}

static void gap_device_name_ext_prop_read_cb(struct gatt_db_attribute *attrib,
                    unsigned int id, uint16_t offset,
                    uint8_t opcode, struct bt_att *att,
                    void *user_data)
{
    (void) id, (void) offset, (void) opcode, (void) att,(void)user_data;
    uint8_t value[2];

    value[0] = BT_GATT_CHRC_EXT_PROP_RELIABLE_WRITE;
    value[1] = 0;

    gatt_db_attribute_read_result(attrib, id, 0, value, sizeof(value));
}

static void populate_gap_service()
{
    bt_uuid_t uuid;
    struct gatt_db_attribute *service, *tmp;
    uint16_t appearance;

    /* Add the GAP service */
    bt_uuid16_create(&uuid, UUID_GAP);
    service = gatt_db_add_service(GattSrv::mServer.sref->db, &uuid, true, 6);

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
                    NULL // server
                                       );

    bt_uuid16_create(&uuid, GATT_CHARAC_EXT_PROPER_UUID);
    gatt_db_service_add_descriptor(service, &uuid, BT_ATT_PERM_READ,
                    gap_device_name_ext_prop_read_cb,
                    NULL,
                    NULL // server
                                   );

    /*
     * Appearance characteristic. Reads and writes should obtain the value
     * from the database.
     */
    bt_uuid16_create(&uuid, GATT_CHARAC_APPEARANCE);
    tmp = gatt_db_service_add_characteristic(service, &uuid,
                            BT_ATT_PERM_READ,
                            BT_GATT_CHRC_PROP_READ,
                            NULL, NULL, NULL // server
                                             );

    /*
     * Write the appearance value to the database, since we're not using a
     * callback.
     */
    put_le16(BleApi::BLE_APPEARANCE_GENERIC_SENSOR, &appearance);
    gatt_db_attribute_write(tmp, 0, (const uint8_t *) &appearance,
                            sizeof(appearance),
                            BT_ATT_OP_WRITE_REQ,
                            NULL, confirm_write,
                            (void*) "appearance");

    gatt_db_service_set_active(service, true);
}
#endif

