#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"

#define CUSTOM_SERVICE_UUID_BASE         {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, \
                                          0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

#define CUSTOM_SERVICE_UUID               0xDFB0
#define TEMP_CHAR_UUID                    0xDFB1
#define LED_CHAR_UUID                     0xDFB2

#define BLE_CUS_DEF(_name) \
static ble_cus_t _name; \
NRF_SDH_BLE_OBSERVER(_name ## _obs, \
                     BLE_HRS_BLE_OBSERVER_PRIO, \
                     ble_cus_on_ble_evt, &_name)

typedef enum
{
    BLE_CUS_EVT_NOTIFICATION_ENABLED,                             /**< Custom value notification enabled event. */
    BLE_CUS_EVT_NOTIFICATION_DISABLED,                            /**< Custom value notification disabled event. */
    BLE_CUS_EVT_DISCONNECTED,
    BLE_CUS_EVT_CONNECTED
} ble_cus_evt_type_t;

typedef  struct 
{ 
    ble_cus_evt_type_t evt_type;                                  
} ble_cus_evt_t ;

typedef struct ble_cus_s ble_cus_t;

typedef void (*ble_cus_evt_handler_t) (ble_cus_t * p_cus, ble_cus_evt_t * p_evt);

typedef void (*ble_cus_service_led_write_handler_t) (uint16_t conn_handle, ble_cus_t * p_cus, uint8_t new_state);

typedef struct
{
    ble_cus_evt_handler_t               evt_handler;

    uint8_t                             initial_led;           
    uint8_t                             initial_temp;

    ble_cus_service_led_write_handler_t led_write_handler;
    
    ble_srv_cccd_security_mode_t        led_char_attr_md;               /**< Initial security level for Custom characteristics attribute */
    ble_srv_cccd_security_mode_t        temp_value_char_attr_md;
} ble_cus_init_t;

struct ble_cus_s
{
    ble_cus_evt_handler_t               evt_handler;
    uint16_t                            service_handle;                 /**< Handle of Custom Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t            led_handles;                    /**< Handles related to the Custom Value characteristic. */
    ble_cus_service_led_write_handler_t led_write_handler;
    ble_gatts_char_handles_t            temp_handles;
    uint16_t                            conn_handle;                    /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    uint8_t                             uuid_type; 
};

uint32_t ble_cus_temp_update(ble_cus_t * p_cus, uint64_t data); //
void ble_cus_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);
uint32_t ble_cus_init(ble_cus_t*, const ble_cus_init_t*);
