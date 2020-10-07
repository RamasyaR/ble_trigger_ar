#include <string.h>
#include "ble_cus.h"
#include "sdk_common.h"
#include "boards.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

static void on_connect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    p_cus->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
    ble_cus_evt_t evt;
    evt.evt_type = BLE_CUS_EVT_CONNECTED;
    p_cus->evt_handler(p_cus, &evt);
}

static void on_disconnect(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_cus->conn_handle = BLE_CONN_HANDLE_INVALID;
}

static void on_write(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write; 
    if ((p_evt_write->handle == p_cus->led_handles.value_handle)
        && (p_evt_write->len == 1)
        && (p_cus->led_write_handler != NULL))
    {
        p_cus->led_write_handler(p_ble_evt->evt.gap_evt.conn_handle, p_cus, p_evt_write->data[0]);
    }

    // Check if the Custom value CCCD is written to and that the value is the appropriate length, i.e 2 bytes.
    if ((p_evt_write->handle == p_cus->temp_handles.cccd_handle)
        && (p_evt_write->len == 2)
       )
    {
        // CCCD written, call application event handler
        if (p_cus->evt_handler != NULL)
        {
            ble_cus_evt_t evt;

            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_CUS_EVT_NOTIFICATION_ENABLED;
            }
            else
            {
                evt.evt_type = BLE_CUS_EVT_NOTIFICATION_DISABLED;
            }
            // Call the application event handler.
            p_cus->evt_handler(p_cus, &evt);
        }
    }
}



void ble_cus_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context)
{
   ble_cus_t * p_cus = (ble_cus_t *) p_context;

   if (p_cus == NULL || p_ble_evt == NULL)
   {
       return;
   }
   
   switch (p_ble_evt->header.evt_id)
   {
       case BLE_GAP_EVT_CONNECTED:
           on_connect(p_cus, p_ble_evt);
           break;
       case BLE_GAP_EVT_DISCONNECTED:
           on_disconnect(p_cus, p_ble_evt);
           break;
       case BLE_GATTS_EVT_WRITE:
           on_write(p_cus, p_ble_evt);
           break;
       default:
           // No implementation needed.
           break;
   }
}

static uint32_t temp_char_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init)
{
    uint32_t            err_code;
    ble_gatts_char_md_t temp_char_md;
    ble_gatts_attr_md_t temp_char_cccd_md;
    ble_gatts_attr_t    temp_char_attr;
    ble_uuid_t          temp_ble_uuid;
    ble_gatts_attr_md_t temp_char_attr_md;

    memset(&temp_char_cccd_md, 0, sizeof(temp_char_cccd_md));

    //Read  operation on Cccd should be possible without authentication.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&temp_char_cccd_md.read_perm);

    temp_char_cccd_md.write_perm = p_cus_init->temp_value_char_attr_md.cccd_write_perm;  
    temp_char_cccd_md.vloc       = BLE_GATTS_VLOC_STACK;

    memset(&temp_char_md, 0, sizeof(temp_char_md));

    temp_char_md.char_props.read   = 0;
    temp_char_md.char_props.write  = 0;
    temp_char_md.char_props.notify = 0; 
    temp_char_md.p_char_user_desc  = NULL;
    temp_char_md.p_char_pf         = NULL;
    temp_char_md.p_user_desc_md    = NULL;
    temp_char_md.p_cccd_md         = NULL; 
    temp_char_md.p_sccd_md         = NULL;
    temp_char_md.char_props.notify = 1;  
    temp_char_md.p_cccd_md         = &temp_char_cccd_md; 
		
    memset(&temp_char_attr_md, 0, sizeof(temp_char_attr_md));

    temp_char_attr_md.write_perm = p_cus_init->temp_value_char_attr_md.write_perm;
    temp_char_attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    temp_char_attr_md.rd_auth    = 0;
    temp_char_attr_md.wr_auth    = 0;
    temp_char_attr_md.vlen       = 0;

    temp_ble_uuid.type = p_cus->uuid_type;
    temp_ble_uuid.uuid = TEMP_CHAR_UUID;

    memset(&temp_char_attr, 0, sizeof(temp_char_attr));

    temp_char_attr.p_uuid    = &temp_ble_uuid;
    temp_char_attr.p_attr_md = &temp_char_attr_md;
    temp_char_attr.init_len  = sizeof(uint8_t);
    temp_char_attr.init_offs = 0;
    temp_char_attr.max_len   = sizeof(uint64_t);

    err_code = sd_ble_gatts_characteristic_add(p_cus->service_handle, &temp_char_md,
                                               &temp_char_attr,
                                               &p_cus->temp_handles);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}

static uint32_t led_char_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init)
{
    uint32_t            err_code;
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.read   = 0;
    char_md.char_props.write  = 1;
    char_md.char_props.notify = 0; 
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL; 
    char_md.p_sccd_md         = NULL;
    char_md.p_cccd_md         = NULL;         

    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.write_perm = p_cus_init->led_char_attr_md.write_perm;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = LED_CHAR_UUID;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(uint8_t);
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = sizeof(uint8_t);

    err_code = sd_ble_gatts_characteristic_add(p_cus->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_cus->led_handles);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}

uint32_t  ble_cus_init ( ble_cus_t * p_cus, const  ble_cus_init_t * p_cus_init)
{
    if (p_cus == NULL || p_cus_init == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    p_cus->evt_handler = p_cus_init->evt_handler;
    p_cus->led_write_handler = p_cus_init->led_write_handler;
    p_cus->conn_handle = BLE_CONN_HANDLE_INVALID;

    ble_uuid128_t base_uuid = {CUSTOM_SERVICE_UUID_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_cus->uuid_type);
    VERIFY_SUCCESS(err_code);
    
    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = CUSTOM_SERVICE_UUID;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cus->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = temp_char_add(p_cus, p_cus_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
        err_code = led_char_add (p_cus, p_cus_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    return err_code;
}

uint32_t ble_cus_temp_update(ble_cus_t * p_cus, uint64_t data)
{
    if (p_cus == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;
    uint8_t data_size = sizeof(data);
    uint8_t data_arr[data_size];
    
    for(uint8_t i=0; i<data_size; i++)
      data_arr[i] = (uint8_t)(data >> (8*(data_size-i-1)));

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = sizeof(data_arr);
    gatts_value.offset  = 0;
    gatts_value.p_value = data_arr;

    // Update database.
    err_code = sd_ble_gatts_value_set(p_cus->conn_handle,
                                      p_cus->temp_handles.value_handle,
                                      &gatts_value);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Send value if connected and notifying.
    if ((p_cus->conn_handle != BLE_CONN_HANDLE_INVALID)) 
    {
        ble_gatts_hvx_params_t hvx_params;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_cus->temp_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = gatts_value.offset;
        hvx_params.p_len  = &gatts_value.len;
        hvx_params.p_data = gatts_value.p_value;

        err_code = sd_ble_gatts_hvx(p_cus->conn_handle, &hvx_params);
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }
    return err_code;
}