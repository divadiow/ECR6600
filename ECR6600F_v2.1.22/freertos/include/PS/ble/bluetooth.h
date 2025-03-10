/**
 ****************************************************************************************
 *
 * @file bluetooth.h
 *
 * @brief bluetooth Interface Header.
 *
 * @par Copyright (C):
 *      ESWIN 2015-2020
 *
 ****************************************************************************************
 */

#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "bluetooth_err.h"
#include "stdint.h"
#include "stdbool.h"


/*
 * GLOBAL VARIABLES DEFINITIONS
 ****************************************************************************************
 */
#define VALUE_INVALID_PARAM 0xFF
#define ATT_UUID_128_LEN                        0x0010
#define ATT_BASE_UUID_128(h_b, l_b) {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, l_b, h_b, 0x00, 0x00}
#define ATT_USER_UUID_128(h_b, l_b) {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, l_b, h_b, 0x00, 0x00}

#define GAP_BD_ADDR_LEN       (6)

#define ECR_BLE_GAP_ADDR_TYPE_PUBLIC                                        (0x00)  /**< Public (identity) address.*/
#define ECR_BLE_GAP_ADDR_TYPE_RANDOM                                        (0x01)  /**< Random (identity) address. */

#define ECR_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED                      (0x01)  /**< CONN and scannable undirected*/
#define ECR_BLE_GAP_ADV_TYPE_CONN_NONSCANNABLE_DIR_HIGHDUTY_CYCLE           (0x02)  /**< CONN non-scannable directed advertising
                                                                                        events. Advertising interval is less that 3.75 ms.
                                                                                        Use this type for fast reconnections.
                                                                                        @note Advertising data is not supported. */
#define ECR_BLE_GAP_ADV_TYPE_CONN_NONSCANNABLE_DIRECTED                     (0x03)  /**< CONN non-scannable directed advertising
                                                                                        events.
                                                                                        @note Advertising data is not supported. */
#define ECR_BLE_GAP_ADV_TYPE_NONCONN_SCANNABLE_UNDIRECTED                   (0x04)  /**< Non-CONN scannable undirected
                                                                                        advertising events. */
#define ECR_BLE_GAP_ADV_TYPE_NONCONN_NONSCANNABLE_UNDIRECTED                (0x05)  /**< Non-CONN non-scannable undirected
                                                                                        advertising events. */
#define ECR_BLE_GAP_ADV_TYPE_EXTENDED_CONN_NONSCANNABLE_UNDIRECTED          (0x06)  /**< CONN non-scannable undirected advertising
                                                                                        events using extended advertising PDUs. */
#define ECR_BLE_GAP_ADV_TYPE_EXTENDED_CONN_NONSCANNABLE_DIRECTED            (0x07)  /**< CONN non-scannable directed advertising
                                                                                        events using extended advertising PDUs. */
#define ECR_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_SCANNABLE_UNDIRECTED          (0x08)  /**< Non-CONN scannable undirected advertising
                                                                                        events using extended advertising PDUs.
                                                                                        @note Only scan response data is supported. */
#define ECR_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_SCANNABLE_DIRECTED            (0x09)  /**< Non-CONN scannable directed advertising
                                                                                        events using extended advertising PDUs.
                                                                                        @note Only scan response data is supported. */
#define ECR_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_NONSCANNABLE_UNDIRECTED       (0x0A)  /**< Non-CONN non-scannable undirected advertising
                                                                                        events using extended advertising PDUs. */
#define ECR_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_NONSCANNABLE_DIRECTED         (0x0B)  /**< Non-CONN non-scannable directed advertising
                                                                                        events using extended advertising PDUs. */

#define ECR_BLE_GATT_INVALID_HANDLE                                         (0xFFFF) /**< Invalid Connect Handle */

#define ECR_BLE_GAP_ADV_STATE_IDLE                                          (0x00)  /**< Idle, no advertising */
#define ECR_BLE_GAP_ADV_STATE_START                                         (0x01)  /**< Start Advertising. A temporary state, haven't received the result.*/
#define ECR_BLE_GAP_ADV_STATE_ADVERTISING                                   (0x02)  /**< Advertising State */
#define ECR_BLE_GAP_ADV_STATE_STOP                                          (0x04)  /**< Stop Advertising. A temporary state, haven't received the result.*/

#define BLE_ATT_MTU_DEF        (512)

/// Advertising interval (in 625us slot) (chapter 2.E.7.8.5)
#define BLE_ADV_INTERVAL_MIN    0x0020 //(20 ms)
#define BLE_ADV_INTERVAL_MAX    0x4000 //(10.24 sec)

/// Scanning interval (in 625us slot) (chapter 2.E.7.8.10)
#define BLE_SCAN_INTERVAL_MIN   0x0004 //(2.5 ms)
#define BLE_SCAN_INTERVAL_MAX   0x4000 //(10.24 sec)
#define BLE_SCAN_INTERVAL_DFT   0x0010 //(10 ms)

/// Scanning window (in 625us slot) (chapter 2.E.7.8.10)
#define BLE_SCAN_WINDOW_MIN     0x0004 //(2.5 ms)
#define BLE_SCAN_WINDOW_MAX     0x4000 //(10.24 sec)
#define BLE_SCAN_WINDOW_DFT     0x0010 //(10 ms)

/// Connection interval (N*1.250ms) (chapter 2.E.7.8.12)
#define BLE_CON_INTERVAL_MIN    0x0006  //(7.5 msec)
#define BLE_CON_INTERVAL_MAX    0x0C80  //(4 sec)
#define BLE_CON_INTERVAL_DFT    0x000C  //(15 msec)

/// Connection latency (N*cnx evt) (chapter 2.E.7.8.12)
#define BLE_CON_LATENCY_MIN     0x0000
#define BLE_CON_LATENCY_MAX     0x01F3  // (499)
/// Supervision TO (N*10ms) (chapter 2.E.7.8.12)
#define BLE_CON_SUP_TO_MIN      0x000A  //(100 msec)
#define BLE_CON_SUP_TO_MAX      0x0C80  //(32 sec)

typedef struct
{
    ///6-byte array address value
    uint8_t  addr[GAP_BD_ADDR_LEN];
} ecr_bd_addr_t;

typedef enum 
{
	ECR_BT_ADV_IDX,
	ECR_BT_SCAN_IDX,
	ECR_BT_INIT_IDX,
	
	ECR_BT_IDX_MAX,
}ECR_BT_ACT_T;

typedef enum
{
	/// Stopped state
	ECR_BLE_GAP_ACT_STATE_STOPPED = (0x01),
	/// Started state
	ECR_BLE_GAP_ACT_STATE_STARTED = (0x02),

	/// Invalid state
	ECR_BLE_GAP_ACT_STATE_INVALID = (0xFF),
}ECR_GAP_ACT_STATE_T;
	
enum ecr_ser_declation
{
	/*---------------- DECLARATIONS -----------------*/
    /// Primary service Declaration
    UUID_PRIMARY_SERVICE                     = (0x2800),
    /// Secondary service Declaration
    UUID_SECONDARY_SERVICE                   = (0x2801),
    /// Include Declaration
    UUID_INCLUDE                             = (0x2802),
    /// Characteristic Declaration
    UUID_CHARACTERISTIC                      = (0x2803),

	/// Client characteristic configuration
    UUID_CLIENT_CHAR_CFG                     = (0x2902),
};

/*---------------- DECLARATIONS -----------------*/
/// Primary service Declaration
#define    UUID_PRIMARY_SERVICE_128                     ATT_BASE_UUID_128(0x28,0x00)
/// Secondary service Declaration
#define    UUID_SECONDARY_SERVICE_128                   ATT_BASE_UUID_128(0x28,0x01)
/// Include Declaration
#define    UUID_INCLUDE_128                             ATT_BASE_UUID_128(0x28,0x02)
/// Characteristic Declaration
#define    UUID_CHARACTERISTIC_128                      ATT_BASE_UUID_128(0x28,0x03)
/// Client characteristic configuration
#define    UUID_CLIENT_CHAR_CFG_128                     ATT_BASE_UUID_128(0x29,0x02)

/// Connection role
enum ble_conn_role
{
    /// Master
    ECR_BLE_ROLE_MASTER = 0,
    /// Slave
    ECR_BLE_ROLE_SLAVE,
};


enum ble_role
{
	/// No role set yet
	BLE_ROLE_NONE		 = 0x00,

	/// Observer role
	BLE_ROLE_OBSERVER	 = 0x01,

	/// Broadcaster role
	BLE_ROLE_BROADCASTER = 0x02,

	/// Master/Central role
	BLE_ROLE_CENTRAL	 = (0x04 | BLE_ROLE_OBSERVER),

	/// Peripheral/Slave role
	BLE_ROLE_PERIPHERAL  = (0x08 | BLE_ROLE_BROADCASTER),

	/// Device has all role, both peripheral and central
	BLE_ROLE_ALL		 = (BLE_ROLE_CENTRAL | BLE_ROLE_PERIPHERAL),
};

typedef enum {
	ECR_BLE_STATIC_ADDR,									/**< Public or Private Static Address according to device address configuration */
	ECR_BLE_GEN_RSLV_ADDR,									/**< Generated resolvable private random address */
	ECR_BLE_GEN_NON_RSLV_ADDR,								/**< Generated non-resolvable private random address */
} ECR_BLE_ADDR_TYPE_E;

/**@brief GAP address parameters. */
typedef struct {
 	ECR_BLE_ADDR_TYPE_E	 type;						 /**< Mac Address Type*/
 	uint8_t	 addr[6];								 /**< Mac Address, Address size, 6 bytes */
} ECR_BLE_GAP_ADDR_T;

typedef struct {
	uint16_t	 length;								 /**< Ble Data Len */
	uint8_t	 *p_data;								 /**< Ble Data Pointer */
} ECR_BLE_DATA_T;
 
typedef enum {
	ECR_BLE_ADV_DATA,									 /**< Adv Data - Only */
	ECR_BLE_RSP_DATA,									 /**< Scan Response Data - Only */
	ECR_BLE_ADV_RSP_DATA,								 /**< Adv Data + Scan Response Data */
} ECR_BLE_GAP_ADV_TYPE_E;

 /**@brief GAP Scanning Types. */
 typedef enum 
  {
	  ECR_SCAN_TYPE_GEN_DISC = 0,						 /**< General discovery */
		 
	  ECR_SCAN_TYPE_LIM_DISC,							 /**< Limited discovery */
	  
	  ECR_SCAN_TYPE_OBSERVER,							 /**< Observer */
	  
	  ECR_SCAN_TYPE_SEL_OBSERVER,						 /**< Selective observer */
	  
	  ECR_SCAN_TYPE_CONN_DISC,							 /**< Connectable discovery */
	  
	  ECR_SCAN_TYPE_SEL_CONN_DISC,						 /**< Selective connectable discovery */
  }ECR_BLE_SCAN_TYPE_E;
	  
  /**@brief GAP filtering policy for duplicated packets. */
  typedef enum 
  {
	  ECR_DUP_FILT_DIS = 0, 							 /**< Disable filtering of duplicated packets */
		 
	  ECR_DUP_FILT_EN,									 /**< Enable filtering of duplicated packets */
	  
	  ECR_DUP_FILT_EN_PERIOD,							 /**< Enable filtering of duplicated packets, reset for each scan period */
  }ECR_BLE_DUP_FILTER_POL_E;

/**@brief Scanning properties bit field bit value. */
  typedef enum 
  {
	  ECR_BLE_SCAN_PROP_PHY_1M_BIT 	      = (1 << 0),			/**< Scan advertisement on the LE 1M PHY */

	  ECR_BLE_SCAN_PROP_PHY_CODED_BIT	  = (1 << 1),			/**< Scan advertisement on the LE Coded PHY */

	  ECR_BLE_SCAN_PROP_ACTIVE_1M_BIT	  = (1 << 2),			/**< Active scan on LE 1M PHY (Scan Request PDUs may be sent) */

	  ECR_BLE_SCAN_PROP_ACTIVE_CODED_BIT  = (1 << 3),			/**< Active scan on LE Coded PHY (Scan Request PDUs may be sent) */

	  ECR_BLE_SCAN_PROP_ACCEPT_RPA_BIT    = (1 << 4),			/**< Accept directed advertising packets if we use a RPA and target address cannot be solved by the controller*/

	  ECR_BLE_SCAN_PROP_FILT_TRUNC_BIT    = (1 << 5),			/**< Filter truncated advertising or scan response reports */
  }ECR_BLE_SCAN_PROP;

 /**@brief GAP advertising parameters. */
typedef struct {
	uint8_t				 adv_type;					 /**< Adv Type. Refer to ECR_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED etc.*/
	ECR_BLE_GAP_ADDR_T 	 direct_addr;				 /**< For Directed Advertising, you can fill in direct address */

	uint16_t				 adv_interval_min;			 /**< Range: 0x0020 to 0x4000  Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec */
	uint16_t				 adv_interval_max;			 /**< Range: 0x0020 to 0x4000  Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec */
	uint8_t				 adv_channel_map;			 /**< Advertising Channel Map, 0x01 = adv channel index 37,  0x02 = adv channel index 38,
															 0x04 = adv channel index 39. Default Value: 0x07*/
} ECR_BLE_GAP_ADV_PARAMS_T;
 
/**@brief GAP adv report parameters. */
typedef struct {
	ECR_BLE_GAP_ADV_TYPE_E  adv_type;					 /**< Advertising report type. Refer to @ECR_BLE_GAP_ADV_TYPE_E */
	ECR_BLE_GAP_ADDR_T 	 peer_addr; 				 /**< Bluetooth address of the peer device. */
	int8_t 				 rssi;						 /**< Received Signal Strength Indication in dBm of the last packet received. */
	uint8_t				 channel_index; 			 /**< Channel Index on which the last advertising packet is received (37-39).channel index = 37, it means that we do advertisement in channel 37. */
	ECR_BLE_DATA_T 		 data;						 /**< Received advertising or scan response data.  */
} ECR_BLE_GAP_ADV_REPORT_T;

/**@brief GAP scanning parameters. */
typedef struct {
    uint8_t                 scan_type;                   /**< Type of scanning to be started @ECR_BLE_SCAN_TYPE_E*/
    uint8_t                 dup_filt_pol;                /**< Duplicate packet filtering policy @ECR_BLE_DUP_FILTER_POL_E*/
	uint8_t                 scan_prop;                   /**< Properties for the scan procedure (@ECR_BLE_SCAN_PROP for bit signification)*/
    uint16_t                interval;                   /**< Scan interval in 625 us units. */
    uint16_t                window;                     /**< Scan window in 625 us units. */
    uint16_t                duration;                    /**< Scan duration (in unit of 10ms). 0 means that the controller will scan continuously until
  														 /// reception of a stop command from the application */
	uint16_t 				period;						 /**< Scan period (in unit of 1.28s). Time interval betweem two consequent starts of a scan duration
    													 /// by the controller. 0 means that the scan procedure is not periodic */
    uint8_t                 scan_channel_map;           /**< Scan Channel Index, refer to @ECR_BLE_GAP_ADV_PARAMS_T*/
} ECR_BLE_GAP_SCAN_PARAMS_T;

/** @brief  Definition of LE connection request parameter.*/
typedef struct {
    uint16_t                conn_interval_min;          /**< Minimum value for the connection interval.  */
    uint16_t                conn_interval_max;          /**< Maximum value for the connection interval. */
    uint16_t                conn_latency;               /**< Slave latency for the connection in number of connection events.*/
    uint16_t                conn_sup_timeout;           /**< Supervision timeout for the LE Link (in unit of 10ms).*/
    uint16_t                conn_establish_timeout;     /**< Timeout for connection establishment (in unit of 10ms),only for establish connection*/
} ECR_BLE_GAP_CONN_PARAMS_T;

/**@brief GAP initiating parameters. */
typedef struct {

} ECR_BLE_GAP_INIT_PARAMS_T;

typedef enum {
    ECR_BLE_UUID_TYPE_16,                               /**< UUID 16 bit */
    ECR_BLE_UUID_TYPE_32,                               /**< UUID 32 bit */
    ECR_BLE_UUID_TYPE_128,                              /**< UUID 128 bit */
} ECR_BLE_UUID_TYPE_E;

/** @brief  Bluetooth Low Energy UUID type, encapsulates both 16-bit and 128-bit UUIDs. */
typedef struct {
    ECR_BLE_UUID_TYPE_E     uuid_type;                  /**< UUID Type, Refer to @ECR_BLE_UUID_TYPE_E */

    union {
        uint16_t            uuid16;                     /**< 16-bit UUID value  */
        uint32_t              uuid32;                     /**< 32-bit UUID value */
        uint8_t             uuid128[16];                /**< Little-Endian UUID bytes. 128bit uuid*/
    }uuid;
} ECR_BLE_UUID_T;

/**< GATT characteristic property bit field values */
typedef enum {
    ECR_BLE_GATT_CHAR_PROP_BROADCAST            = 0x01, /**< If set, permits broadcasts of the Characteristic Value using Server Characteristic Configuration Descriptor. */
    ECR_BLE_GATT_CHAR_PROP_READ                 = 0x02, /**< If set, permits reads of the Characteristic Value */
    ECR_BLE_GATT_CHAR_PROP_WRITE_NO_RSP         = 0x04, /**< If set, permit writes of the Characteristic Value without response */
    ECR_BLE_GATT_CHAR_PROP_WRITE                = 0x08, /**< If set, permits writes of the Characteristic Value with response */
    ECR_BLE_GATT_CHAR_PROP_NOTIFY               = 0x10, /**< If set, permits notifications of a Characteristic Value without acknowledgment */
    ECR_BLE_GATT_CHAR_PROP_INDICATE             = 0x20, /**< If set, permits indications of a Characteristic Value with acknowledgment */
    ECR_BLE_GATT_CHAR_PROP_WRITE_AUTHEN_SIGNED  = 0x40, /**< If set, permits signed writes to the Characteristic Value */
    ECR_BLE_GATT_CHAR_PROP_EXT_PROP             = 0x80, /**< If set, additional characteristic properties are defined in the Characteristic */
}ECR_BLE_CHAR_PROP_TYPE_E;

/**< GATT attribute permission bit field values */
typedef enum {
    ECR_BLE_GATT_PERM_NONE                      = 0x01, /**< No operations supported, e.g. for notify-only */
    ECR_BLE_GATT_PERM_READ                      = 0x02, /**< Attribute read permission. */
    ECR_BLE_GATT_PERM_WRITE                     = 0x04, /**< Attribute write permission. */
    ECR_BLE_GATT_PERM_READ_ENCRYPT              = 0x08, /**< Attribute read permission with encryption. */
    ECR_BLE_GATT_PERM_WRITE_ENCRYPT             = 0x10, /**< Attribute write permission with encryption. */
    ECR_BLE_GATT_PERM_READ_AUTHEN               = 0x20, /**< Attribute read permission with authentication. */
    ECR_BLE_GATT_PERM_WRITE_AUTHEN              = 0x40, /**< Attribute write permission with authentication. */
    ECR_BLE_GATT_PERM_PREPARE_WRITE             = 0x80, /**< Attribute prepare write permission. */
} ECR_BLE_ATTR_PERM_E;

typedef struct {
    uint16_t        handle;                             /**< [Output] After init the characteristic, we will get the char-handle, we need to restore it */

    ECR_BLE_UUID_T  char_uuid;                          /**< Characteristics UUID */
    uint8_t         property;                           /**< Characteristics property , Refer to ECR_BLE_CHAR_PROP_TYPE_E */
    uint8_t         permission;                         /**< Characteristics value attribute permission */
    uint16_t        value_len;                          /**< Characteristics value length */
} ECR_BLE_CHAR_PARAMS_T;

typedef enum {
    ECR_BLE_UUID_UNKNOWN                    = 0x0000,   /**< Reserved UUID. */
    ECR_BLE_UUID_SERVICE_PRIMARY            = 0x2800,   /**< Primary Service. */
    ECR_BLE_UUID_SERVICE_SECONDARY          = 0x2801,   /**< Secondary Service. */
    ECR_BLE_UUID_SERVICE_INCLUDE            = 0x2802,   /**< Include. */
    ECR_BLE_UUID_CHARACTERISTIC             = 0x2803,   /**< Characteristic. */
} ECR_BLE_SERVICE_TYPE_E;

typedef struct {
    uint16_t                    handle;                 /**< After init the service, we will get the svc-handle */

    ECR_BLE_UUID_T              svc_uuid;               /**< Service UUID */
    ECR_BLE_SERVICE_TYPE_E      type;                   /**< Service Type */
    
    uint8_t                     char_num;               /**< Number of characteristic */
    ECR_BLE_CHAR_PARAMS_T       *p_char;                /**< Pointer of characteristic */
} ECR_BLE_SERVICE_PARAMS_T;

typedef struct {
    uint8_t                     svc_num;                /**< If we only use service(0xFD50), the svc_num will be set into 1 */
    ECR_BLE_SERVICE_PARAMS_T    *p_service;
} ECR_BLE_GATTS_PARAMS_T;

typedef enum {
	ECR_BLE_GAP_EVT_RESET,							  /**< RESET */

    ECR_BLE_GAP_EVT_CONNECT,                            /**< Connected as peripheral role */

    ECR_BLE_GAP_EVT_DISCONNECT,                         /**< Disconnected */

    ECR_BLE_GAP_EVT_ADV_STATE,                          /**< Adv State */

    ECR_BLE_GAP_EVT_ADV_REPORT,                         /**< Scan result report */

    ECR_BLE_GAP_EVT_CONN_PARAM_REQ,                     /**< Parameter update request */

    ECR_BLE_GAP_EVT_CONN_PARAM_UPDATE,                  /**< Parameter update successfully */
    
    ECR_BLE_GAP_EVT_CONN_RSSI,                          /**< Got RSSI value of link peer device */

	ECR_BLE_GAP_IRK_GEN_IND,							  /**< Generate IRK */
} ECR_BLE_GAP_EVT_TYPE_E;

typedef enum {      
    ECR_BLE_GATT_EVT_MTU_REQUEST,              		    /**< MTU exchange request event, For Ble peripheral, we need to do reply*/

    ECR_BLE_GATT_EVT_MTU_RSP,                           /**< MTU exchange respond event, For Ble Central, the ble central has finished the MTU-Request */

    ECR_BLE_GATT_EVT_PRIM_SEV_DISCOVERY,                /**< [Ble Central] Discovery Service */

    ECR_BLE_GATT_EVT_CHAR_DISCOVERY,                    /**< [Ble Central] Discovery Characteristics*/

    ECR_BLE_GATT_EVT_CHAR_DESC_DISCOVERY,               /**< [Ble Central] Discovery descriptors */

    ECR_BLE_GATT_EVT_NOTIFY_INDICATE_TX,                /**< [Ble peripheral] Transfer data Callback, only report Result */

    ECR_BLE_GATT_EVT_WRITE_REQ,                         /**< [Ble Peripheral] Get Client-Write Char Request*/

    ECR_BLE_GATT_EVT_WRITE_CMP,                         /**< [Ble Central] Transfer data Callback, only report Result */

    ECR_BLE_GATT_EVT_NOTIFY_INDICATE_RX,                /**< [Ble Central] Get Notification or Indification data */

    ECR_BLE_GATT_EVT_READ_RX,                           /**< [Ble Central] Get Char-Read Data */
} ECR_BLE_GATT_EVT_TYPE_E;

typedef struct {
	uint8_t irk[16];
} ECR_BLE_GAP_GEN_IRK_T;

typedef struct {
    uint8_t                         role;               /**< BLE role for this connection, see @ref ECR_BLE_ROLE_SERVER, or ECR_BLE_ROLE_SLAVE */
    ECR_BLE_GAP_ADDR_T              peer_addr;          /**< Reserved, [Ble Central],For some platform, we will get the peer address after connect one device */
    ECR_BLE_GAP_CONN_PARAMS_T       conn_params;        /**< Report Connection Parameters */
} ECR_BLE_GAP_CONNECT_EVT_T;

typedef struct {
    uint8_t                         role;               /**< BLE role for this disconnection */
    int32_t                           reason;             /**< Report Disconnection Reason */
} ECR_BLE_GAP_DISCONNECT_EVT_T;

typedef struct {
    uint16_t                        char_handle;        /**< Notify Characteristic Handle */
    int32_t                           result;             /**< Notify Result */
} ECR_BLE_NOTIFY_RESULT_EVT_T;

typedef struct {
    uint16_t                        char_handle;        /**< Write Characteristic Handle */
    int32_t                           result;             /**< Write Result */
} ECR_BLE_WRITE_RESULT_EVT_T;


typedef struct {
    ECR_BLE_UUID_T              uuid;               /**< Discovery Service UUID */
    uint16_t                    start_handle;       /**< Discovery Start Handle */
    uint16_t                    end_handle;         /**< Discovery End Handle */
} ECR_BLE_GATT_SVC_DISC_TYPE_T;

typedef struct {
    ECR_BLE_UUID_T              uuid;               /**< Discovery Service UUID */
    uint16_t                    handle;             /**< Discovery Char value Handle */
} ECR_BLE_GATT_CHAR_DISC_TYPE_T;

typedef struct {
    uint16_t                        cccd_handle;        /**< Discovery Descriptor Handle, Return CCCD Handle */
    uint8_t                         uuid_len;           /**< UUID length */
    uint16_t                        uuid16;             /**< Descriptor UUID */
} ECR_BLE_GATT_DESC_DISC_TYPE_T;

typedef struct {
    uint16_t                        char_handle;        /**< Specify one characteristic handle */
    ECR_BLE_DATA_T                  report;             /**< Report Data, Refer to @ ECR_BLE_DATA_T */
} ECR_BLE_DATA_REPORT_T;

typedef struct {
    ECR_BLE_GAP_EVT_TYPE_E              type;           /**< Gap Event */
    uint16_t                            conn_handle;    /**< Connection Handle */
    int32_t                               result;         /**< Will Refer to HOST STACK Error Code */

    union {
        ECR_BLE_GAP_CONNECT_EVT_T       connect;        /**< Receive connect callback, This value can be used with ECR_BLE_EVT_PERIPHERAL_CONNECT and ECR_BLE_EVT_CENTRAL_CONNECT_DISCOVERY*/
        ECR_BLE_GAP_DISCONNECT_EVT_T    disconnect;     /**< Receive disconnect callback*/
        ECR_BLE_GAP_ADV_REPORT_T        adv_report;     /**< Receive Adv and Respond report*/
        ECR_BLE_GAP_CONN_PARAMS_T       conn_param;     /**< We will update connect parameters.This value can be used with ECR_BLE_EVT_CONN_PARAM_REQ and ECR_BLE_EVT_CONN_PARAM_UPDATE*/
        int8_t                          link_rssi;      /**< Peer device RSSI value */
		ECR_BLE_GAP_GEN_IRK_T       gen_irk;
	}gap_event;
} ECR_BLE_GAP_PARAMS_EVT_T;

typedef struct {
    ECR_BLE_GATT_EVT_TYPE_E             type;           /**< Gatt Event */
    uint16_t                            conn_handle;    /**< Connection Handle */
    int32_t                               result;         /**< Will Refer to HOST STACK Error Code */

    union {
        uint16_t                        exchange_mtu;   /**< This value can be used with ECR_BLE_GATT_EVT_MTU_REQUEST and ECR_BLE_GATT_EVT_MTU_RSP*/
        ECR_BLE_GATT_SVC_DISC_TYPE_T    svc_disc;       /**< Discovery All Service, if result is BT_ERROR_DISC_DONE,the procedure ends  */
        ECR_BLE_GATT_CHAR_DISC_TYPE_T   char_disc;      /**< Discovery Specific Characteristic, if result is BT_ERROR_DISC_DONE,the procedure ends */
        ECR_BLE_GATT_DESC_DISC_TYPE_T   desc_disc;      /**< Discovery Specific Descriptors*/
        ECR_BLE_NOTIFY_RESULT_EVT_T     notify_result;  /**< This value can be used with ECR_BLE_GATT_EVT_NOTIFY_TX*/
        ECR_BLE_DATA_REPORT_T           write_report;   /**< This value can be used with ECR_BLE_GATT_EVT_WRITE_REQ*/
        ECR_BLE_WRITE_RESULT_EVT_T      write_result;   /**< This value can be used with ECR_BLE_GATT_EVT_WRITE_CMP*/
        ECR_BLE_DATA_REPORT_T           data_report;    /**< This value can be used with ECR_BLE_GATT_EVT_NOTIFY_INDICATE_RX*/
        ECR_BLE_DATA_REPORT_T           data_read;      /**< After we do read attr in central mode, we will get the callback*/
    }gatt_event;
} ECR_BLE_GATT_PARAMS_EVT_T;

/// Privacy configuration
enum ble_priv_cfg
{
	/// Indicate if identity address is a public (0) or static private random (1) address
	BLE_PRIV_CFG_PRIV_ADDR_BIT = (1 << 0),
	BLE_PRIV_CFG_PRIV_ADDR_POS = 0,
	/// Reserved
	BLE_PRIV_CFG_RSVD			= (1 << 1),
	/// Indicate if controller privacy is enabled
	BLE_PRIV_CFG_PRIV_EN_BIT	= (1 << 2),
	BLE_PRIV_CFG_PRIV_EN_POS	= 2,
};

enum ble_pairing_mode
{
    /// No pairing authorized
    BLE_PAIRING_DISABLE  = 0,
    /// Legacy pairing Authorized
    BLE_PAIRING_LEGACY   = (1 << 0),
    /// Secure Connection pairing Authorized
    BLE_PAIRING_SEC_CON  = (1 << 1),
};

typedef struct{
	/// Device Role: Central, Peripheral, Observer, Broadcaster or All roles.
	uint8_t role;

	/// -------------- Privacy Config -----------------------
	/// Duration before regenerate device address when privacy is enabled. - in seconds
	uint16_t renew_dur;
	/// Provided own static private random address
	ecr_bd_addr_t addr;
	/// Device IRK used for resolvable random BD address generation (LSB first)
    /// Key value MSB -> LSB
    uint8_t irk[16];
	/// Privacy configuration bit field (@see enum ble_priv_cfg for bit signification)
	uint8_t privacy_cfg;

	/// -------------- Security Config -----------------------

	/// Pairing mode authorized (@see enum ble_pairing_mode)
	uint8_t pairing_mode;

	/// --------------- L2CAP Configuration ---------------------------
	/// Maximal MTU acceptable for device
	uint16_t max_mtu;
	/// Maximal MPS Packet size acceptable for device, MPS <= MTU
	uint16_t max_mps;

}ECR_BLE_DEV_CONFIG;

/**< GAP Callback Register function definition */
typedef VOID(*ECR_BLE_GAP_EVT_FUNC_CB)(ECR_BLE_GAP_PARAMS_EVT_T *p_event);

/**< GATT Callback Register function definition */
typedef VOID(*ECR_BLE_GATT_EVT_FUNC_CB)(ECR_BLE_GATT_PARAMS_EVT_T *p_event);


/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/*
 * GAP INT
 */
/*
 *Use BLE Stack:
 *	ecr_ble_gap_callback_register();
 *	ecr_ble_gatt_callback_register();
 *	ecr_ble_set_dev_config();
 *
 *	if support security connect:
 *		ecr_ble_generate_irk(void);
 *		then in irk callback:
 *			ecr_ble_set_irk(irk);
 */
void ecr_ble_reset(void);


/* brief: Configuration can be modified only if no connection exists and no air
 * operation on-going.
 */
void ecr_ble_set_dev_config(ECR_BLE_DEV_CONFIG *config);

void ecr_ble_generate_irk(void);
void ecr_ble_set_irk(uint8_t *irk);

void ecr_ble_gap_callback_register(ECR_BLE_GAP_EVT_FUNC_CB gap_evt);

BT_RET_T ecr_ble_set_device_addr(ECR_BLE_GAP_ADDR_T *p_addr);
BT_RET_T ecr_ble_get_device_addr(ECR_BLE_GAP_ADDR_T *p_addr);

/*
 * brief: the max length of shortened local name is 18 octets
 */
BT_RET_T ecr_ble_set_device_name( uint8_t *dev_name, uint16_t dev_name_len);
BT_RET_T ecr_ble_get_device_name( uint8_t *dev_name, uint16_t dev_name_len);

//only when adv is started, adv data can be downloaded
BT_RET_T ecr_ble_adv_param_update(ECR_BLE_DATA_T CONST *p_adv, ECR_BLE_DATA_T CONST *p_scan_rsp);
BT_RET_T ecr_ble_adv_param_set(ECR_BLE_DATA_T CONST *p_adv, ECR_BLE_DATA_T CONST *p_scan_rsp);

BT_RET_T ecr_ble_gap_adv_start(ECR_BLE_GAP_ADV_PARAMS_T CONST *p_adv_params);
BT_RET_T ecr_ble_gap_adv_stop(void);

BT_RET_T ecr_ble_gap_scan_start(ECR_BLE_GAP_SCAN_PARAMS_T CONST *p_scan_params);
BT_RET_T ecr_ble_gap_scan_stop(void);
BT_RET_T ecr_ble_actv_params_check(uint8_t actv_idx,void CONST *p_actv_params);

//master connect slave
BT_RET_T ecr_ble_gap_connect(  ECR_BLE_GAP_ADDR_T CONST *p_peer_addr, ECR_BLE_GAP_CONN_PARAMS_T CONST *p_conn_params);
BT_RET_T ecr_ble_gap_connect_cancel(void);

//Disconnect from peer
BT_RET_T ecr_ble_gap_disconnect(uint16_t conn_handle, uint8_t reason);

BT_RET_T ecr_ble_connect_param_update(uint16_t conn_handle, ECR_BLE_GAP_CONN_PARAMS_T CONST *p_conn_params);

void ecr_ble_connect_rssi_get(uint16_t conn_handle);


/*
 * GATT INT
 */
void ecr_ble_gatt_callback_register(ECR_BLE_GATT_EVT_FUNC_CB gatt_evt);

// add user gatt service
void ecr_ble_gatts_service_add(ECR_BLE_GATTS_PARAMS_T *p_service, bool is_ues_deatult);

void ecr_ble_gatts_value_get(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length);
void ecr_ble_gatts_value_set(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length);

void ecr_ble_gatts_value_notify(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length);
void ecr_ble_gatts_value_indicate(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length);

void ecr_ble_gattc_exchange_mtu_rsp(uint16_t conn_handle, uint16_t server_mtu);


//master, to do
void ecr_ble_gattc_all_service_discovery(uint16_t conn_handle);
void ecr_ble_gattc_service_discovery_by_uuid(uint16_t conn_handle, ECR_BLE_UUID_T uuid);
void ecr_ble_gattc_all_char_discovery(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle);
void ecr_ble_gattc_char_desc_discovery(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle);
void ecr_ble_gattc_write_without_rsp(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length);
void ecr_ble_gattc_write(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length);
void ecr_ble_gattc_read(uint16_t conn_handle, uint16_t char_handle);
void ecr_ble_gattc_exchange_mtu_req(uint16_t conn_handle, uint16_t client_mtu);
uint16_t ecr_ble_gattc_get_mtu(uint16_t conn_handle);
ECR_GAP_ACT_STATE_T ecr_ble_actv_state_get(ECR_BT_ACT_T idx);

#endif /* _BLUETOOTH_H */

