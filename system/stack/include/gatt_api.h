/******************************************************************************
 *
 *  Copyright 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#ifndef GATT_API_H
#define GATT_API_H

#include <base/strings/stringprintf.h>
#include <bluetooth/log.h>

#include <cstdint>
#include <list>
#include <string>

#include "btm_ble_api.h"
#include "gattdefs.h"
#include "hardware/bt_gatt_types.h"
#include "include/hardware/bt_common_types.h"
#include "internal_include/bt_target.h"
#include "macros.h"
#include "stack/include/btm_ble_api_types.h"
#include "stack/include/hci_error_code.h"
#include "types/bluetooth/uuid.h"
#include "types/bt_transport.h"
#include "types/raw_address.h"

/*****************************************************************************
 *  Constants
 ****************************************************************************/
/* Success code and error codes */
typedef enum GattStatus : uint8_t {
  GATT_SUCCESS = 0x00,
  GATT_INVALID_HANDLE = 0x01,
  GATT_READ_NOT_PERMIT = 0x02,
  GATT_WRITE_NOT_PERMIT = 0x03,
  GATT_INVALID_PDU = 0x04,
  GATT_INSUF_AUTHENTICATION = 0x05,
  GATT_REQ_NOT_SUPPORTED = 0x06,
  GATT_INVALID_OFFSET = 0x07,
  GATT_INSUF_AUTHORIZATION = 0x08,
  GATT_PREPARE_Q_FULL = 0x09,
  GATT_NOT_FOUND = 0x0a,
  GATT_NOT_LONG = 0x0b,
  GATT_INSUF_KEY_SIZE = 0x0c,
  GATT_INVALID_ATTR_LEN = 0x0d,
  GATT_ERR_UNLIKELY = 0x0e,
  GATT_INSUF_ENCRYPTION = 0x0f,
  GATT_UNSUPPORT_GRP_TYPE = 0x10,
  GATT_INSUF_RESOURCE = 0x11,
  GATT_DATABASE_OUT_OF_SYNC = 0x12,
  GATT_VALUE_NOT_ALLOWED = 0x13,
  GATT_ILLEGAL_PARAMETER = 0x87,
  GATT_NO_RESOURCES = 0x80,
  GATT_INTERNAL_ERROR = 0x81,
  GATT_WRONG_STATE = 0x82,
  GATT_DB_FULL = 0x83,
  GATT_BUSY = 0x84,
  GATT_ERROR = 0x85,
  GATT_CMD_STARTED = 0x86,
  GATT_PENDING = 0x88,
  GATT_AUTH_FAIL = 0x89,
  GATT_INVALID_CFG = 0x8b,
  GATT_SERVICE_STARTED = 0x8c,
  GATT_ENCRYPED_MITM = GATT_SUCCESS,
  GATT_ENCRYPED_NO_MITM = 0x8d,
  GATT_NOT_ENCRYPTED = 0x8e,
  GATT_CONGESTED = 0x8f,
  GATT_DUP_REG = 0x90,      /* 0x90 */
  GATT_ALREADY_OPEN = 0x91, /* 0x91 */
  GATT_CANCEL = 0x92,       /* 0x92 */
  GATT_CONNECTION_TIMEOUT = 0x93,
  WRITE_REJECTED = 0xFC,
  /* = 0xE0 ~ 0xFC reserved for future use */

  /* Client Characteristic Configuration Descriptor Improperly Configured */
  GATT_CCC_CFG_ERR = 0xFD,
  /* Procedure Already in progress */
  GATT_PRC_IN_PROGRESS = 0xFE,
  /* Attribute value out of range */
  GATT_OUT_OF_RANGE = 0xFF,
} tGATT_STATUS;

inline std::string gatt_status_text(const tGATT_STATUS& status) {
  switch (status) {
    CASE_RETURN_TEXT(GATT_SUCCESS);  // Also GATT_ENCRYPED_MITM
    CASE_RETURN_TEXT(GATT_INVALID_HANDLE);
    CASE_RETURN_TEXT(GATT_READ_NOT_PERMIT);
    CASE_RETURN_TEXT(GATT_WRITE_NOT_PERMIT);
    CASE_RETURN_TEXT(GATT_INVALID_PDU);
    CASE_RETURN_TEXT(GATT_INSUF_AUTHENTICATION);
    CASE_RETURN_TEXT(GATT_REQ_NOT_SUPPORTED);
    CASE_RETURN_TEXT(GATT_INVALID_OFFSET);
    CASE_RETURN_TEXT(GATT_INSUF_AUTHORIZATION);
    CASE_RETURN_TEXT(GATT_PREPARE_Q_FULL);
    CASE_RETURN_TEXT(GATT_NOT_FOUND);
    CASE_RETURN_TEXT(GATT_NOT_LONG);
    CASE_RETURN_TEXT(GATT_INSUF_KEY_SIZE);
    CASE_RETURN_TEXT(GATT_INVALID_ATTR_LEN);
    CASE_RETURN_TEXT(GATT_ERR_UNLIKELY);
    CASE_RETURN_TEXT(GATT_INSUF_ENCRYPTION);
    CASE_RETURN_TEXT(GATT_UNSUPPORT_GRP_TYPE);
    CASE_RETURN_TEXT(GATT_INSUF_RESOURCE);
    CASE_RETURN_TEXT(GATT_DATABASE_OUT_OF_SYNC);
    CASE_RETURN_TEXT(GATT_VALUE_NOT_ALLOWED);
    CASE_RETURN_TEXT(GATT_ILLEGAL_PARAMETER);
    CASE_RETURN_TEXT(GATT_NO_RESOURCES);
    CASE_RETURN_TEXT(GATT_INTERNAL_ERROR);
    CASE_RETURN_TEXT(GATT_WRONG_STATE);
    CASE_RETURN_TEXT(GATT_DB_FULL);
    CASE_RETURN_TEXT(GATT_BUSY);
    CASE_RETURN_TEXT(GATT_ERROR);
    CASE_RETURN_TEXT(GATT_CMD_STARTED);
    CASE_RETURN_TEXT(GATT_PENDING);
    CASE_RETURN_TEXT(GATT_AUTH_FAIL);
    CASE_RETURN_TEXT(GATT_INVALID_CFG);
    CASE_RETURN_TEXT(GATT_SERVICE_STARTED);
    CASE_RETURN_TEXT(GATT_ENCRYPED_NO_MITM);
    CASE_RETURN_TEXT(GATT_NOT_ENCRYPTED);
    CASE_RETURN_TEXT(GATT_CONGESTED);
    CASE_RETURN_TEXT(GATT_DUP_REG);
    CASE_RETURN_TEXT(GATT_ALREADY_OPEN);
    CASE_RETURN_TEXT(GATT_CANCEL);
    CASE_RETURN_TEXT(GATT_CONNECTION_TIMEOUT);
    CASE_RETURN_TEXT(GATT_CCC_CFG_ERR);
    CASE_RETURN_TEXT(GATT_PRC_IN_PROGRESS);
    CASE_RETURN_TEXT(GATT_OUT_OF_RANGE);
    default:
      return base::StringPrintf("UNKNOWN[%hhu]", status);
  }
}

typedef enum : uint8_t {
  GATT_RSP_ERROR = 0x01,
  GATT_REQ_MTU = 0x02,
  GATT_RSP_MTU = 0x03,
  GATT_REQ_FIND_INFO = 0x04,
  GATT_RSP_FIND_INFO = 0x05,
  GATT_REQ_FIND_TYPE_VALUE = 0x06,
  GATT_RSP_FIND_TYPE_VALUE = 0x07,
  GATT_REQ_READ_BY_TYPE = 0x08,
  GATT_RSP_READ_BY_TYPE = 0x09,
  GATT_REQ_READ = 0x0A,
  GATT_RSP_READ = 0x0B,
  GATT_REQ_READ_BLOB = 0x0C,
  GATT_RSP_READ_BLOB = 0x0D,
  GATT_REQ_READ_MULTI = 0x0E,
  GATT_RSP_READ_MULTI = 0x0F,
  GATT_REQ_READ_BY_GRP_TYPE = 0x10,
  GATT_RSP_READ_BY_GRP_TYPE = 0x11,
  /*                 0001-0010 (write)*/
  GATT_REQ_WRITE = 0x12,
  GATT_RSP_WRITE = 0x13,
  /* changed in V4.0 01001-0010(write cmd)*/
  GATT_CMD_WRITE = 0x52,
  GATT_REQ_PREPARE_WRITE = 0x16,
  GATT_RSP_PREPARE_WRITE = 0x17,
  GATT_REQ_EXEC_WRITE = 0x18,
  GATT_RSP_EXEC_WRITE = 0x19,
  GATT_HANDLE_VALUE_NOTIF = 0x1B,
  GATT_HANDLE_VALUE_IND = 0x1D,
  GATT_HANDLE_VALUE_CONF = 0x1E,

  GATT_REQ_READ_MULTI_VAR = 0x20,
  GATT_RSP_READ_MULTI_VAR = 0x21,
  GATT_HANDLE_MULTI_VALUE_NOTIF = 0x23,

  /* changed in V4.0 1101-0010 (signed write)  see write cmd above*/
  GATT_SIGN_CMD_WRITE = 0xD2,
  /* 0x1E = 30 + 1 = 31*/
  GATT_OP_CODE_MAX = (GATT_HANDLE_MULTI_VALUE_NOTIF + 1),
} tGATT_OP_CODE;

typedef enum : uint8_t {
  MTU_EXCHANGE_DEVICE_DISCONNECTED = 0x00,
  MTU_EXCHANGE_NOT_ALLOWED,
  MTU_EXCHANGE_NOT_DONE_YET,
  MTU_EXCHANGE_IN_PROGRESS,
  MTU_EXCHANGE_ALREADY_DONE,
} tGATTC_TryMtuRequestResult;

inline std::string gatt_op_code_text(const tGATT_OP_CODE& op_code) {
  switch (op_code) {
    case GATT_RSP_ERROR:
      return std::string("GATT_RSP_ERROR");
    case GATT_REQ_MTU:
      return std::string("GATT_REQ_MTU");
    case GATT_RSP_MTU:
      return std::string("GATT_RSP_MTU");
    case GATT_REQ_FIND_INFO:
      return std::string("GATT_REQ_FIND_INFO");
    case GATT_RSP_FIND_INFO:
      return std::string("GATT_RSP_FIND_INFO");
    case GATT_REQ_FIND_TYPE_VALUE:
      return std::string("GATT_REQ_FIND_TYPE_VALUE");
    case GATT_RSP_FIND_TYPE_VALUE:
      return std::string("GATT_RSP_FIND_TYPE_VALUE");
    case GATT_REQ_READ_BY_TYPE:
      return std::string("GATT_REQ_READ_BY_TYPE");
    case GATT_RSP_READ_BY_TYPE:
      return std::string("GATT_RSP_READ_BY_TYPE");
    case GATT_REQ_READ:
      return std::string("GATT_REQ_READ");
    case GATT_RSP_READ:
      return std::string("GATT_RSP_READ");
    case GATT_REQ_READ_BLOB:
      return std::string("GATT_REQ_READ_BLOB");
    case GATT_RSP_READ_BLOB:
      return std::string("GATT_RSP_READ_BLOB");
    case GATT_REQ_READ_MULTI:
      return std::string("GATT_REQ_READ_MULTI");
    case GATT_RSP_READ_MULTI:
      return std::string("GATT_RSP_READ_MULTI");
    case GATT_REQ_READ_BY_GRP_TYPE:
      return std::string("GATT_REQ_READ_BY_GRP_TYPE");
    case GATT_RSP_READ_BY_GRP_TYPE:
      return std::string("GATT_RSP_READ_BY_GRP_TYPE");
    case GATT_REQ_WRITE:
      return std::string("GATT_REQ_WRITE");
    case GATT_RSP_WRITE:
      return std::string("GATT_RSP_WRITE");
    case GATT_CMD_WRITE:
      return std::string("GATT_CMD_WRITE");
    case GATT_REQ_PREPARE_WRITE:
      return std::string("GATT_REQ_PREPARE_WRITE");
    case GATT_RSP_PREPARE_WRITE:
      return std::string("GATT_RSP_PREPARE_WRITE");
    case GATT_REQ_EXEC_WRITE:
      return std::string("GATT_REQ_EXEC_WRITE");
    case GATT_RSP_EXEC_WRITE:
      return std::string("GATT_RSP_EXEC_WRITE");
    case GATT_HANDLE_VALUE_NOTIF:
      return std::string("GATT_HANDLE_VALUE_NOTIF");
    case GATT_HANDLE_VALUE_IND:
      return std::string("GATT_HANDLE_VALUE_IND");
    case GATT_HANDLE_VALUE_CONF:
      return std::string("GATT_HANDLE_VALUE_CONF");
    case GATT_REQ_READ_MULTI_VAR:
      return std::string("GATT_REQ_READ_MULTI_VAR");
    case GATT_RSP_READ_MULTI_VAR:
      return std::string("GATT_RSP_READ_MULTI_VAR");
    case GATT_HANDLE_MULTI_VALUE_NOTIF:
      return std::string("GATT_HANDLE_MULTI_VALUE_NOTIF");
    case GATT_SIGN_CMD_WRITE:
      return std::string("GATT_SIGN_CMD_WRITE");
    case GATT_OP_CODE_MAX:
      return std::string("GATT_OP_CODE_MAX");
  };
}

#define GATT_HANDLE_IS_VALID(x) ((x) != 0)

typedef enum : uint16_t {
  GATT_CONN_OK = 0,
  /* general L2cap failure  */
  GATT_CONN_L2C_FAILURE = 1,
  /* 0x08 connection timeout  */
  GATT_CONN_TIMEOUT = HCI_ERR_CONNECTION_TOUT,
  /* 0x13 connection terminate by peer user  */
  GATT_CONN_TERMINATE_PEER_USER = HCI_ERR_PEER_USER,
  /* 0x16 connection terminated by local host  */
  GATT_CONN_TERMINATE_LOCAL_HOST = HCI_ERR_CONN_CAUSE_LOCAL_HOST,
  /* 0x22 connection fail for LMP response tout */
  GATT_CONN_LMP_TIMEOUT = HCI_ERR_LMP_RESPONSE_TIMEOUT,

  GATT_CONN_FAILED_ESTABLISHMENT = HCI_ERR_CONN_FAILED_ESTABLISHMENT,

  GATT_CONN_TERMINATED_POWER_OFF = HCI_ERR_REMOTE_POWER_OFF,

  BTA_GATT_CONN_NONE = 0x0101, /* 0x0101 no connection to cancel  */

} tGATT_DISCONN_REASON;

inline std::string gatt_disconnection_reason_text(
    const tGATT_DISCONN_REASON& reason) {
  switch (reason) {
    CASE_RETURN_TEXT(GATT_CONN_OK);
    CASE_RETURN_TEXT(GATT_CONN_L2C_FAILURE);
    CASE_RETURN_TEXT(GATT_CONN_TIMEOUT);
    CASE_RETURN_TEXT(GATT_CONN_TERMINATE_PEER_USER);
    CASE_RETURN_TEXT(GATT_CONN_TERMINATE_LOCAL_HOST);
    CASE_RETURN_TEXT(GATT_CONN_LMP_TIMEOUT);
    CASE_RETURN_TEXT(GATT_CONN_FAILED_ESTABLISHMENT);
    CASE_RETURN_TEXT(BTA_GATT_CONN_NONE);
    CASE_RETURN_TEXT(GATT_CONN_TERMINATED_POWER_OFF);
    default:
      return base::StringPrintf("UNKNOWN[%hu]", reason);
  }
}

/* MAX GATT MTU size
*/
#ifndef GATT_MAX_MTU_SIZE
#define GATT_MAX_MTU_SIZE 517
#endif

/* default GATT MTU size over LE link
*/
#define GATT_DEF_BLE_MTU_SIZE 23

/* invalid connection ID
*/
#define GATT_INVALID_CONN_ID 0xFFFF

/* GATT notification caching timer, default to be three seconds
*/
#ifndef GATTC_NOTIF_TIMEOUT
#define GATTC_NOTIF_TIMEOUT 3
#endif

/*****************************************************************************
 * GATT Structure Definition
 ****************************************************************************/

/* Attribute permissions
*/
#define GATT_PERM_READ (1 << 0)              /* bit 0 */
#define GATT_PERM_READ_ENCRYPTED (1 << 1)    /* bit 1 */
#define GATT_PERM_READ_ENC_MITM (1 << 2)     /* bit 2 */
#define GATT_PERM_WRITE (1 << 4)             /* bit 4 */
#define GATT_PERM_WRITE_ENCRYPTED (1 << 5)   /* bit 5 */
#define GATT_PERM_WRITE_ENC_MITM (1 << 6)    /* bit 6 */
#define GATT_PERM_WRITE_SIGNED (1 << 7)      /* bit 7 */
#define GATT_PERM_WRITE_SIGNED_MITM (1 << 8) /* bit 8 */
#define GATT_PERM_READ_IF_ENCRYPTED_OR_DISCOVERABLE (1 << 9) /* bit 9 */
typedef uint16_t tGATT_PERM;

/* the MS nibble of tGATT_PERM; key size 7=0; size 16=9 */
#define GATT_ENCRYPT_KEY_SIZE_MASK (0xF000)

#define GATT_READ_ALLOWED                                                \
  (GATT_PERM_READ | GATT_PERM_READ_ENCRYPTED | GATT_PERM_READ_ENC_MITM | \
   GATT_PERM_READ_IF_ENCRYPTED_OR_DISCOVERABLE)
#define GATT_READ_AUTH_REQUIRED (GATT_PERM_READ_ENCRYPTED)
#define GATT_READ_MITM_REQUIRED (GATT_PERM_READ_ENC_MITM)
#define GATT_READ_ENCRYPTED_REQUIRED \
  (GATT_PERM_READ_ENCRYPTED | GATT_PERM_READ_ENC_MITM)

#define GATT_WRITE_ALLOWED                                                  \
  (GATT_PERM_WRITE | GATT_PERM_WRITE_ENCRYPTED | GATT_PERM_WRITE_ENC_MITM | \
   GATT_PERM_WRITE_SIGNED | GATT_PERM_WRITE_SIGNED_MITM)

#define GATT_WRITE_AUTH_REQUIRED \
  (GATT_PERM_WRITE_ENCRYPTED | GATT_PERM_WRITE_SIGNED)

#define GATT_WRITE_MITM_REQUIRED \
  (GATT_PERM_WRITE_ENC_MITM | GATT_PERM_WRITE_SIGNED_MITM)

#define GATT_WRITE_ENCRYPTED_PERM \
  (GATT_PERM_WRITE_ENCRYPTED | GATT_PERM_WRITE_ENC_MITM)

#define GATT_WRITE_SIGNED_PERM \
  (GATT_PERM_WRITE_SIGNED | GATT_PERM_WRITE_SIGNED_MITM)

/* Characteristic properties
*/
#define GATT_CHAR_PROP_BIT_BROADCAST (1 << 0)
#define GATT_CHAR_PROP_BIT_READ (1 << 1)
#define GATT_CHAR_PROP_BIT_WRITE_NR (1 << 2)
#define GATT_CHAR_PROP_BIT_WRITE (1 << 3)
#define GATT_CHAR_PROP_BIT_NOTIFY (1 << 4)
#define GATT_CHAR_PROP_BIT_INDICATE (1 << 5)
#define GATT_CHAR_PROP_BIT_AUTH (1 << 6)
#define GATT_CHAR_PROP_BIT_EXT_PROP (1 << 7)
typedef uint8_t tGATT_CHAR_PROP;

/* Format of the value of a characteristic. enumeration type
*/
enum {
  GATT_FORMAT_RES,     /* rfu */
  GATT_FORMAT_BOOL,    /* 0x01 boolean */
  GATT_FORMAT_2BITS,   /* 0x02 2 bit */
  GATT_FORMAT_NIBBLE,  /* 0x03 nibble */
  GATT_FORMAT_UINT8,   /* 0x04 uint8 */
  GATT_FORMAT_UINT12,  /* 0x05 uint12 */
  GATT_FORMAT_UINT16,  /* 0x06 uint16 */
  GATT_FORMAT_UINT24,  /* 0x07 uint24 */
  GATT_FORMAT_UINT32,  /* 0x08 uint32 */
  GATT_FORMAT_UINT48,  /* 0x09 uint48 */
  GATT_FORMAT_UINT64,  /* 0x0a uint64 */
  GATT_FORMAT_UINT128, /* 0x0B uint128 */
  GATT_FORMAT_SINT8,   /* 0x0C signed 8 bit integer */
  GATT_FORMAT_SINT12,  /* 0x0D signed 12 bit integer */
  GATT_FORMAT_SINT16,  /* 0x0E signed 16 bit integer */
  GATT_FORMAT_SINT24,  /* 0x0F signed 24 bit integer */
  GATT_FORMAT_SINT32,  /* 0x10 signed 32 bit integer */
  GATT_FORMAT_SINT48,  /* 0x11 signed 48 bit integer */
  GATT_FORMAT_SINT64,  /* 0x12 signed 64 bit integer */
  GATT_FORMAT_SINT128, /* 0x13 signed 128 bit integer */
  GATT_FORMAT_FLOAT32, /* 0x14 float 32 */
  GATT_FORMAT_FLOAT64, /* 0x15 float 64*/
  GATT_FORMAT_SFLOAT,  /* 0x16 IEEE-11073 16 bit SFLOAT */
  GATT_FORMAT_FLOAT,   /* 0x17 IEEE-11073 32 bit SFLOAT */
  GATT_FORMAT_DUINT16, /* 0x18 IEEE-20601 format */
  GATT_FORMAT_UTF8S,   /* 0x19 UTF-8 string */
  GATT_FORMAT_UTF16S,  /* 0x1a UTF-16 string */
  GATT_FORMAT_STRUCT,  /* 0x1b Opaque structure*/
  GATT_FORMAT_MAX      /* 0x1c or above reserved */
};
typedef uint8_t tGATT_FORMAT;

/* Characteristic Presentation Format Descriptor value
*/
typedef struct {
  uint16_t unit;  /* as UUIUD defined by SIG */
  uint16_t descr; /* as UUID as defined by SIG */
  tGATT_FORMAT format;
  int8_t exp;
  uint8_t name_spc; /* The name space of the description */
} tGATT_CHAR_PRES;

/* Characteristic Report reference Descriptor format
*/
typedef struct {
  uint8_t rpt_id;   /* report ID */
  uint8_t rpt_type; /* report type */
} tGATT_CHAR_RPT_REF;

#define GATT_VALID_RANGE_MAX_SIZE 16
typedef struct {
  uint8_t format;
  uint16_t len;
  uint8_t lower_range[GATT_VALID_RANGE_MAX_SIZE]; /* in little endian format */
  uint8_t upper_range[GATT_VALID_RANGE_MAX_SIZE];
} tGATT_VALID_RANGE;

/* Characteristic Aggregate Format attribute value
*/
#define GATT_AGGR_HANDLE_NUM_MAX 10
typedef struct {
  uint8_t num_handle;
  uint16_t handle_list[GATT_AGGR_HANDLE_NUM_MAX];
} tGATT_CHAR_AGGRE;

/* Characteristic descriptor: Extended Properties value
*/
/* permits reliable writes of the Characteristic Value */
#define GATT_CHAR_BIT_REL_WRITE 0x0001
/* permits writes to the characteristic descriptor */
#define GATT_CHAR_BIT_WRITE_AUX 0x0002

/* characteristic descriptor: client configuration value
*/
#define GATT_CLT_CONFIG_NONE 0x0000
#define GATT_CLT_CONFIG_NOTIFICATION 0x0001
#define GATT_CLT_CONFIG_INDICATION 0x0002

/* characteristic descriptor: server configuration value
*/
#define GATT_SVR_CONFIG_NONE 0x0000
#define GATT_SVR_CONFIG_BROADCAST 0x0001
typedef uint16_t tGATT_SVR_CHAR_CONFIG;

/* Characteristic descriptor: Extended Properties value
*/
/* permits reliable writes of the Characteristic Value */
#define GATT_CHAR_BIT_REL_WRITE 0x0001
/* permits writes to the characteristic descriptor */
#define GATT_CHAR_BIT_WRITE_AUX 0x0002

/* authentication requirement
*/
#define GATT_AUTH_REQ_NONE 0
#define GATT_AUTH_REQ_NO_MITM 1 /* unauthenticated encryption */
#define GATT_AUTH_REQ_MITM 2    /* authenticated encryption */
#define GATT_AUTH_REQ_SIGNED_NO_MITM 3
#define GATT_AUTH_REQ_SIGNED_MITM 4
typedef uint8_t tGATT_AUTH_REQ;

/* Attribute Value structure
*/
typedef struct {
  uint16_t conn_id;
  uint16_t handle; /* attribute handle */
  uint16_t offset; /* attribute value offset, if no offset is needed for the
                      command, ignore it */
  uint16_t len;    /* length of attribute value */
  tGATT_AUTH_REQ auth_req;          /*  authentication request */
  uint8_t value[GATT_MAX_ATTR_LEN]; /* the actual attribute value */
} tGATT_VALUE;

/* Union of the event data which is used in the server respond API to carry the
 * server response information
*/
typedef union {
  /* data type            member          event   */
  tGATT_VALUE attr_value; /* READ, HANDLE_VALUE_IND, PREPARE_WRITE */
                          /* READ_BLOB, READ_BY_TYPE */
  uint16_t handle;        /* WRITE, WRITE_BLOB */

} tGATTS_RSP;

#define GATT_PREP_WRITE_CANCEL 0x00
#define GATT_PREP_WRITE_EXEC 0x01
typedef uint8_t tGATT_EXEC_FLAG;

/* read request always based on UUID */
typedef struct {
  uint16_t handle;
  uint16_t offset;
  bool is_long;
  bt_gatt_db_attribute_type_t
      gatt_type; /* are we writing characteristic or descriptor */
} tGATT_READ_REQ;

/* write request data */
typedef struct {
  uint16_t handle; /* attribute handle */
  uint16_t offset; /* attribute value offset, if no offset is needed for the
                      command, ignore it */
  uint16_t len;    /* length of attribute value */
  uint8_t value[GATT_MAX_ATTR_LEN]; /* the actual attribute value */
  bool need_rsp;                    /* need write response */
  bool is_prep;                     /* is prepare write */
  bt_gatt_db_attribute_type_t
      gatt_type; /* are we writing characteristic or descriptor */
} tGATT_WRITE_REQ;

/* callback data for server access request from client */
typedef union {
  tGATT_READ_REQ read_req; /* read request, read by Type, read blob */

  tGATT_WRITE_REQ write_req;  /* write */
                              /* prepare write */
                              /* write blob */
  uint16_t handle;            /* handle value confirmation */
  uint16_t mtu;               /* MTU exchange request */
  tGATT_EXEC_FLAG exec_write; /* execute write */
} tGATTS_DATA;

typedef uint8_t tGATT_SERV_IF; /* GATT Service Interface */

enum {
  GATTS_REQ_TYPE_READ_CHARACTERISTIC = 1, /* Char read request */
  GATTS_REQ_TYPE_READ_DESCRIPTOR,         /* Desc read request */
  GATTS_REQ_TYPE_WRITE_CHARACTERISTIC,    /* Char write request */
  GATTS_REQ_TYPE_WRITE_DESCRIPTOR,        /* Desc write request */
  GATTS_REQ_TYPE_WRITE_EXEC,              /* Execute write */
  GATTS_REQ_TYPE_MTU,                     /* MTU exchange information */
  GATTS_REQ_TYPE_CONF                     /* handle value confirmation */
};
typedef uint8_t tGATTS_REQ_TYPE;

/* Client Used Data Structure
*/
/* definition of different discovery types */
typedef enum : uint8_t {
  GATT_DISC_SRVC_ALL = 1, /* discover all services */
  GATT_DISC_SRVC_BY_UUID, /* discover service of a special type */
  GATT_DISC_INC_SRVC,     /* discover the included service within a service */
  GATT_DISC_CHAR, /* discover characteristics of a service with/without type
                     requirement */
  GATT_DISC_CHAR_DSCPT, /* discover characteristic descriptors of a character */
  GATT_DISC_MAX         /* maximum discover type */
} tGATT_DISC_TYPE;

/* GATT read type enumeration
*/
enum {
  GATT_READ_BY_TYPE = 1,
  GATT_READ_BY_HANDLE,
  GATT_READ_MULTIPLE,
  GATT_READ_MULTIPLE_VAR_LEN,
  GATT_READ_CHAR_VALUE,
  GATT_READ_PARTIAL,
  GATT_READ_MAX
};
typedef uint8_t tGATT_READ_TYPE;

/* Read By Type Request (GATT_READ_BY_TYPE) Data
*/
typedef struct {
  tGATT_AUTH_REQ auth_req;
  uint16_t s_handle;
  uint16_t e_handle;
  bluetooth::Uuid uuid;
} tGATT_READ_BY_TYPE;

/*   GATT_READ_MULTIPLE request data
*/
#define GATT_MAX_READ_MULTI_HANDLES \
  10 /* Max attributes to read in one request */
typedef struct {
  tGATT_AUTH_REQ auth_req;
  uint16_t num_handles;                          /* number of handles to read */
  uint16_t handles[GATT_MAX_READ_MULTI_HANDLES]; /* handles list to be read */
  bool variable_len;
} tGATT_READ_MULTI;

/*   Read By Handle Request (GATT_READ_BY_HANDLE) data */
typedef struct {
  tGATT_AUTH_REQ auth_req;
  uint16_t handle;
} tGATT_READ_BY_HANDLE;

/*   READ_BT_HANDLE_Request data */
typedef struct {
  tGATT_AUTH_REQ auth_req;
  uint16_t handle;
  uint16_t offset;
} tGATT_READ_PARTIAL;

/* Read Request Data
*/
typedef union {
  tGATT_READ_BY_TYPE service;
  tGATT_READ_BY_TYPE char_type; /* characteristic type */
  tGATT_READ_MULTI read_multiple;
  tGATT_READ_BY_HANDLE by_handle;
  tGATT_READ_PARTIAL partial;
} tGATT_READ_PARAM;

/* GATT write type enumeration */
enum { GATT_WRITE_NO_RSP = 1, GATT_WRITE, GATT_WRITE_PREPARE };
typedef uint8_t tGATT_WRITE_TYPE;

/* Client Operation Complete Callback Data
*/
typedef union {
  tGATT_VALUE att_value;
  uint16_t mtu;
  uint16_t handle;
  uint16_t cid;
} tGATT_CL_COMPLETE;

/* GATT client operation type, used in client callback function
*/
typedef enum : uint8_t {
  GATTC_OPTYPE_NONE = 0,
  GATTC_OPTYPE_DISCOVERY = 1,
  GATTC_OPTYPE_READ = 2,
  GATTC_OPTYPE_WRITE = 3,
  GATTC_OPTYPE_EXE_WRITE = 4,
  GATTC_OPTYPE_CONFIG = 5,
  GATTC_OPTYPE_NOTIFICATION = 6,
  GATTC_OPTYPE_INDICATION = 7,
} tGATTC_OPTYPE;

/* characteristic declaration
*/
typedef struct {
  tGATT_CHAR_PROP char_prop; /* characteristic properties */
  uint16_t val_handle;       /* characteristic value attribute handle */
  bluetooth::Uuid char_uuid; /* characteristic UUID type */
} tGATT_CHAR_DCLR_VAL;

/* primary service group data
*/
typedef struct {
  uint16_t e_handle;     /* ending handle of the group */
  bluetooth::Uuid service_type; /* group type */
} tGATT_GROUP_VALUE;

/* included service attribute value
*/
typedef struct {
  bluetooth::Uuid service_type; /* included service UUID */
  uint16_t s_handle;     /* starting handle */
  uint16_t e_handle;     /* ending handle */
} tGATT_INCL_SRVC;

typedef union {
  tGATT_INCL_SRVC incl_service;  /* include service value */
  tGATT_GROUP_VALUE group_value; /* Service UUID type.
                                    This field is used with GATT_DISC_SRVC_ALL
                                    or GATT_DISC_SRVC_BY_UUID
                                    type of discovery result callback. */

  uint16_t handle; /* When used with GATT_DISC_INC_SRVC type discovery result,
                      it is the included service starting handle.*/

  tGATT_CHAR_DCLR_VAL
      dclr_value; /* Characteristic declaration value.
                     This field is used with GATT_DISC_CHAR type discovery.*/
} tGATT_DISC_VALUE;

/* discover result record
*/
typedef struct {
  bluetooth::Uuid type;
  uint16_t handle;
  tGATT_DISC_VALUE value;
} tGATT_DISC_RES;

#define GATT_LINK_IDLE_TIMEOUT_WHEN_NO_APP  \
  1 /* start a idle timer for this duration \
     when no application need to use the link */

#define GATT_LINK_NO_IDLE_TIMEOUT 0xFFFF

#define GATT_INVALID_ACL_HANDLE 0xFFFF
/* discover result callback function */
typedef void(tGATT_DISC_RES_CB)(uint16_t conn_id, tGATT_DISC_TYPE disc_type,
                                tGATT_DISC_RES* p_data);

/* discover complete callback function */
typedef void(tGATT_DISC_CMPL_CB)(uint16_t conn_id, tGATT_DISC_TYPE disc_type,
                                 tGATT_STATUS status);

/* Define a callback function for when read/write/disc/config operation is
 * completed. */
typedef void(tGATT_CMPL_CBACK)(uint16_t conn_id, tGATTC_OPTYPE op,
                               tGATT_STATUS status, tGATT_CL_COMPLETE* p_data);

/* Define a callback function when an initialized connection is established. */
typedef void(tGATT_CONN_CBACK)(tGATT_IF gatt_if, const RawAddress& bda,
                               uint16_t conn_id, bool connected,
                               tGATT_DISCONN_REASON reason,
                               tBT_TRANSPORT transport);

/* attribute request callback for ATT server */
typedef void(tGATT_REQ_CBACK)(uint16_t conn_id, uint32_t trans_id,
                              tGATTS_REQ_TYPE type, tGATTS_DATA* p_data);

/* channel congestion/uncongestion callback */
typedef void(tGATT_CONGESTION_CBACK)(uint16_t conn_id, bool congested);

/* Define a callback function when encryption is established. */
typedef void(tGATT_ENC_CMPL_CB)(tGATT_IF gatt_if, const RawAddress& bda);

/* Define a callback function when phy is updated. */
typedef void(tGATT_PHY_UPDATE_CB)(tGATT_IF gatt_if, uint16_t conn_id,
                                  uint8_t tx_phy, uint8_t rx_phy,
                                  tGATT_STATUS status);

/* Define a callback function when connection parameters are updated */
typedef void(tGATT_CONN_UPDATE_CB)(tGATT_IF gatt_if, uint16_t conn_id,
                                   uint16_t interval, uint16_t latency,
                                   uint16_t timeout, tGATT_STATUS status);

/* Define a callback function when subrate change event is received */
typedef void(tGATT_SUBRATE_CHG_CB)(tGATT_IF gatt_if, uint16_t conn_id,
                                   uint16_t subrate_factor, uint16_t latency,
                                   uint16_t cont_num, uint16_t timeout,
                                   tGATT_STATUS status);

/* Define the structure that applications use to register with
 * GATT. This structure includes callback functions. All functions
 * MUST be provided.
*/
typedef struct {
  tGATT_CONN_CBACK* p_conn_cb{nullptr};
  tGATT_CMPL_CBACK* p_cmpl_cb{nullptr};
  tGATT_DISC_RES_CB* p_disc_res_cb{nullptr};
  tGATT_DISC_CMPL_CB* p_disc_cmpl_cb{nullptr};
  tGATT_REQ_CBACK* p_req_cb{nullptr};
  tGATT_ENC_CMPL_CB* p_enc_cmpl_cb{nullptr};
  tGATT_CONGESTION_CBACK* p_congestion_cb{nullptr};
  tGATT_PHY_UPDATE_CB* p_phy_update_cb{nullptr};
  tGATT_CONN_UPDATE_CB* p_conn_update_cb{nullptr};
  tGATT_SUBRATE_CHG_CB* p_subrate_chg_cb{nullptr};
} tGATT_CBACK;

/*****************  Start Handle Management Definitions   *********************/

typedef struct {
  bluetooth::Uuid app_uuid128;
  bluetooth::Uuid svc_uuid;
  uint16_t s_handle;
  uint16_t e_handle;
  bool is_primary; /* primary service or secondary */
} tGATTS_HNDL_RANGE;

#define GATTS_SRV_CHG_CMD_ADD_CLIENT 1
#define GATTS_SRV_CHG_CMD_UPDATE_CLIENT 2
#define GATTS_SRV_CHG_CMD_REMOVE_CLIENT 3
#define GATTS_SRV_CHG_CMD_READ_NUM_CLENTS 4
#define GATTS_SRV_CHG_CMD_READ_CLENT 5
typedef uint8_t tGATTS_SRV_CHG_CMD;

typedef struct {
  RawAddress bda;
  bool srv_changed;
} tGATTS_SRV_CHG;

typedef union {
  tGATTS_SRV_CHG srv_chg;
  uint8_t client_read_index; /* only used for sequential reading client srv chg
                                info */
} tGATTS_SRV_CHG_REQ;

typedef union {
  tGATTS_SRV_CHG srv_chg;
  uint8_t num_clients;
} tGATTS_SRV_CHG_RSP;

/* Attribute server handle ranges NV storage callback functions
 */
typedef void(tGATTS_NV_SAVE_CBACK)(bool is_saved,
                                   tGATTS_HNDL_RANGE* p_hndl_range);
typedef bool(tGATTS_NV_SRV_CHG_CBACK)(tGATTS_SRV_CHG_CMD cmd,
                                      tGATTS_SRV_CHG_REQ* p_req,
                                      tGATTS_SRV_CHG_RSP* p_rsp);

typedef struct {
  tGATTS_NV_SAVE_CBACK* p_nv_save_callback;
  tGATTS_NV_SRV_CHG_CBACK* p_srv_chg_callback;
} tGATT_APPL_INFO;

/********************  End Handle Management Definitions   ********************/

/*******************************************************************************
 *  External Function Declarations
 ******************************************************************************/

/******************************************************************************/
/* GATT Profile API Functions */
/******************************************************************************/
/* GATT Profile Server Functions */
/******************************************************************************/

/*******************************************************************************
 *
 * Function         GATTS_NVRegister
 *
 * Description      Application manager calls this function to register for
 *                  NV save callback function.  There can be one and only one
 *                  NV save callback function.
 *
 * Parameter        p_cb_info : callback information
 *
 * Returns          true if registered OK, else false
 *
 ******************************************************************************/
[[nodiscard]] bool GATTS_NVRegister(tGATT_APPL_INFO* p_cb_info);

/*******************************************************************************
 *
 * Function         BTA_GATTS_AddService
 *
 * Description      Add a service. When service is ready, a callback
 *                  event BTA_GATTS_ADD_SRVC_EVT is called to report status
 *                  and handles to the profile.
 *
 * Parameters       server_if: server interface.
 *                  service: pointer array describing service.
 *                  count: number of elements in service array.
 *
 * Returns          on success GATT_SERVICE_STARTED is returned, and
 *                  attribute_handle field inside service elements are filled.
 *                  on error error status is returned.
 *
 ******************************************************************************/
[[nodiscard]] tGATT_STATUS GATTS_AddService(tGATT_IF gatt_if,
                                            btgatt_db_element_t* service,
                                            int count);

/*******************************************************************************
 *
 * Function         GATTS_DeleteService
 *
 * Description      This function is called to delete a service.
 *
 * Parameter        gatt_if       : application interface
 *                  p_svc_uuid    : service UUID
 *                  svc_inst      : instance of the service inside the
 *                                  application
 *
 * Returns          true if operation succeed, else false
 *
 ******************************************************************************/
[[nodiscard]] bool GATTS_DeleteService(tGATT_IF gatt_if,
                                       bluetooth::Uuid* p_svc_uuid,
                                       uint16_t svc_inst);

/*******************************************************************************
 *
 * Function         GATTS_StopService
 *
 * Description      This function is called to stop a service
 *
 * Parameter         service_handle : this is the start handle of a service
 *
 * Returns          None.
 *
 ******************************************************************************/
void GATTS_StopService(uint16_t service_handle);

/*******************************************************************************
 *
 * Function         GATTs_HandleValueIndication
 *
 * Description      This function sends a handle value indication to a client.
 *
 * Parameter        conn_id: connection identifier.
 *                  attr_handle: Attribute handle of this handle value
 *                               indication.
 *                  val_len: Length of the indicated attribute value.
 *                  p_val: Pointer to the indicated attribute value data.
 *
 * Returns          GATT_SUCCESS if successfully sent or queued; otherwise error
 *                               code.
 *
 ******************************************************************************/
[[nodiscard]] tGATT_STATUS GATTS_HandleValueIndication(uint16_t conn_id,
                                                       uint16_t attr_handle,
                                                       uint16_t val_len,
                                                       uint8_t* p_val);

/*******************************************************************************
 *
 * Function         GATTS_HandleValueNotification
 *
 * Description      This function sends a handle value notification to a client.
 *
 * Parameter       conn_id: connection identifier.
 *                  attr_handle: Attribute handle of this handle value
 *                               indication.
 *                  val_len: Length of the indicated attribute value.
 *                  p_val: Pointer to the indicated attribute value data.
 *
 * Returns          GATT_SUCCESS if successfully sent; otherwise error code.
 *
 ******************************************************************************/
[[nodiscard]] tGATT_STATUS GATTS_HandleValueNotification(uint16_t conn_id,
                                                         uint16_t attr_handle,
                                                         uint16_t val_len,
                                                         uint8_t* p_val);

/*******************************************************************************
 *
 * Function         GATTS_SendRsp
 *
 * Description      This function sends the server response to client.
 *
 * Parameter        conn_id: connection identifier.
 *                  trans_id: transaction id
 *                  status: response status
 *                  p_msg: pointer to message parameters structure.
 *
 * Returns          GATT_SUCCESS if successfully sent; otherwise error code.
 *
 ******************************************************************************/
[[nodiscard]] tGATT_STATUS GATTS_SendRsp(uint16_t conn_id, uint32_t trans_id,
                                         tGATT_STATUS status,
                                         tGATTS_RSP* p_msg);

/******************************************************************************/
/* GATT Profile Client Functions */
/******************************************************************************/

/*******************************************************************************
 *
 * Function         GATTC_ConfigureMTU
 *
 * Description      This function is called to configure the ATT MTU size for
 *                  a connection on an LE transport.
 *
 * Parameters       conn_id: connection identifier.
 *                  mtu    - attribute MTU size..
 *
 * Returns          GATT_SUCCESS if command started successfully.
 *
 ******************************************************************************/
[[nodiscard]] tGATT_STATUS GATTC_ConfigureMTU(uint16_t conn_id, uint16_t mtu);

/*******************************************************************************
 * Function         GATTC_UpdateUserAttMtuIfNeeded
 *
 * Description      This function to be called when user requested MTU after
 *                  MTU Exchange has been already done. This will update data
 *                  length in the controller.
 *
 * Parameters        remote_bda : peer device address. (input)
 *                   transport  : physical transport of the GATT connection
 *                                 (BR/EDR or LE) (input)
 *                   user_mtu: user request mtu
 *
 ******************************************************************************/
void GATTC_UpdateUserAttMtuIfNeeded(const RawAddress& remote_bda,
                                    tBT_TRANSPORT transport, uint16_t user_mtu);

/******************************************************************************
 *
 * Function         GATTC_TryMtuRequest
 *
 * Description      This function shall be called before calling
 *                  GATTC_ConfigureMTU in order to check if operation is
 *                  available to do.
 *
 * Parameters        remote_bda : peer device address. (input)
 *                   transport  : physical transport of the GATT connection
 *                                 (BR/EDR or LE) (input)
 *                   conn_id    : connection id  (input)
 *                   current_mtu: current mtu on the link (output)
 *
 * Returns          tGATTC_TryMtuRequestResult:
 *                  - MTU_EXCHANGE_NOT_DONE_YET: There was no MTU Exchange
 *                      procedure on the link. User can call GATTC_ConfigureMTU
 *                      now.
 *                  - MTU_EXCHANGE_NOT_ALLOWED : Not allowed for BR/EDR or if
 *                      link does not exist
 *                  - MTU_EXCHANGE_ALREADY_DONE: MTU Exchange is done. MTU
 *                      should be taken from current_mtu
 *                  - MTU_EXCHANGE_IN_PROGRESS : Other use is doing MTU
 *                      Exchange. Conn_id is stored for result.
 *
 ******************************************************************************/
[[nodiscard]] tGATTC_TryMtuRequestResult GATTC_TryMtuRequest(
    const RawAddress& remote_bda, tBT_TRANSPORT transport, uint16_t conn_id,
    uint16_t* current_mtu);

[[nodiscard]] std::list<uint16_t>
GATTC_GetAndRemoveListOfConnIdsWaitingForMtuRequest(
    const RawAddress& remote_bda);
/*******************************************************************************
 *
 * Function         GATTC_Discover
 *
 * Description      This function is called to do a discovery procedure on ATT
 *                  server.
 *
 * Parameters       conn_id: connection identifier.
 *                  disc_type:discovery type.
 *                  start_handle and end_handle: range of handles for discovery
 *                  uuid: uuid to discovery. set to Uuid::kEmpty for requests
 *                        that don't need it
 *
 * Returns          GATT_SUCCESS if command received/sent successfully.
 *
 ******************************************************************************/
[[nodiscard]] tGATT_STATUS GATTC_Discover(uint16_t conn_id,
                                          tGATT_DISC_TYPE disc_type,
                                          uint16_t start_handle,
                                          uint16_t end_handle,
                                          const bluetooth::Uuid& uuid);
[[nodiscard]] tGATT_STATUS GATTC_Discover(uint16_t conn_id,
                                          tGATT_DISC_TYPE disc_type,
                                          uint16_t start_handle,
                                          uint16_t end_handle);

/*******************************************************************************
 *
 * Function         GATTC_Read
 *
 * Description      This function is called to read the value of an attribute
 *                  from the server.
 *
 * Parameters       conn_id: connection identifier.
 *                  type    - attribute read type.
 *                  p_read  - read operation parameters.
 *
 * Returns          GATT_SUCCESS if command started successfully.
 *
 ******************************************************************************/
[[nodiscard]] tGATT_STATUS GATTC_Read(uint16_t conn_id, tGATT_READ_TYPE type,
                                      tGATT_READ_PARAM* p_read);

/*******************************************************************************
 *
 * Function         GATTC_Write
 *
 * Description      This function is called to read the value of an attribute
 *                  from the server.
 *
 * Parameters       conn_id: connection identifier.
 *                  type    - attribute write type.
 *                  p_write  - write operation parameters.
 *
 * Returns          GATT_SUCCESS if command started successfully.
 *
 ******************************************************************************/
[[nodiscard]] tGATT_STATUS GATTC_Write(uint16_t conn_id, tGATT_WRITE_TYPE type,
                                       tGATT_VALUE* p_write);

/*******************************************************************************
 *
 * Function         GATTC_ExecuteWrite
 *
 * Description      This function is called to send an Execute write request to
 *                  the server.
 *
 * Parameters       conn_id: connection identifier.
 *                  is_execute - to execute or cancel the prepare write
 *                               request(s)
 *
 * Returns          GATT_SUCCESS if command started successfully.
 *
 ******************************************************************************/
[[nodiscard]] tGATT_STATUS GATTC_ExecuteWrite(uint16_t conn_id,
                                              bool is_execute);

/*******************************************************************************
 *
 * Function         GATTC_SendHandleValueConfirm
 *
 * Description      This function is called to send a handle value confirmation
 *                  as response to a handle value notification from server.
 *
 * Parameters       conn_id: connection identifier.
 *                  handle: the handle of the attribute confirmation.
 *
 * Returns          GATT_SUCCESS if command started successfully.
 *
 ******************************************************************************/
[[nodiscard]] tGATT_STATUS GATTC_SendHandleValueConfirm(uint16_t conn_id,
                                                        uint16_t handle);

/*******************************************************************************
 *
 * Function         GATT_SetIdleTimeout
 *
 * Description      This function (common to both client and server) sets the
 *                  idle timeout for a transport connection
 *
 * Parameter        bd_addr:   target device bd address.
 *                  idle_tout: timeout value in seconds.
 *                  transport: transport option.
 *                  is_active: whether we should use this as a signal that an
 *                             active client now exists (which changes link
 *                             timeout logic, see
 *                             t_l2c_linkcb.with_active_local_clients for
 *                             details).
 *
 * Returns          void
 *
 ******************************************************************************/
void GATT_SetIdleTimeout(const RawAddress& bd_addr, uint16_t idle_tout,
                         tBT_TRANSPORT transport, bool is_active);

/*******************************************************************************
 *
 * Function         GATT_Register
 *
 * Description      This function is called to register an  application
 *                  with GATT
 *
 * Parameter        p_app_uuid128: Application UUID
 *                  p_cb_info: callback functions.
 *                  eatt_support: set support for eatt
 *
 * Returns          0 for error, otherwise the index of the client registered
 *                  with GATT
 *
 ******************************************************************************/
[[nodiscard]] tGATT_IF GATT_Register(const bluetooth::Uuid& p_app_uuid128,
                                     const std::string& name,
                                     tGATT_CBACK* p_cb_info, bool eatt_support);

/*******************************************************************************
 *
 * Function         GATT_Deregister
 *
 * Description      This function deregistered the application from GATT.
 *
 * Parameters       gatt_if: application interface.
 *
 * Returns          None.
 *
 ******************************************************************************/
void GATT_Deregister(tGATT_IF gatt_if);

/*******************************************************************************
 *
 * Function         GATT_StartIf
 *
 * Description      This function is called after registration to start
 *                  receiving callbacks for registered interface.  Function may
 *                  call back with connection status and queued notifications
 *
 * Parameter        gatt_if: application interface.
 *
 * Returns          None
 *
 ******************************************************************************/
void GATT_StartIf(tGATT_IF gatt_if);

/*******************************************************************************
 *
 * Function         GATT_Connect
 *
 * Description      This function initiate a connection to a remote device on
 *                  GATT channel.
 *
 * Parameters       gatt_if: application interface
 *                  bd_addr: peer device address
 *                  addr_type: peer device address type
 *                  connection_type: connection type
 *                  transport : Physical transport for GATT connection
 *                              (BR/EDR or LE)
 *                  opportunistic: will not keep device connected if other apps
 *                      disconnect, will not update connected apps counter, when
 *                      disconnected won't cause physical disconnection.
 *
 * Returns          true if connection started; else false
 *
 ******************************************************************************/
[[nodiscard]] bool GATT_Connect(tGATT_IF gatt_if, const RawAddress& bd_addr,
                                tBTM_BLE_CONN_TYPE connection_type,
                                tBT_TRANSPORT transport, bool opportunistic);
[[nodiscard]] bool GATT_Connect(tGATT_IF gatt_if, const RawAddress& bd_addr,
                                tBTM_BLE_CONN_TYPE connection_type,
                                tBT_TRANSPORT transport, bool opportunistic,
                                uint8_t initiating_phys);
[[nodiscard]] bool GATT_Connect(tGATT_IF gatt_if, const RawAddress& bd_addr,
                                tBLE_ADDR_TYPE addr_type,
                                tBTM_BLE_CONN_TYPE connection_type,
                                tBT_TRANSPORT transport, bool opportunistic);
[[nodiscard]] bool GATT_Connect(tGATT_IF gatt_if, const RawAddress& bd_addr,
                                tBLE_ADDR_TYPE addr_type,
                                tBTM_BLE_CONN_TYPE connection_type,
                                tBT_TRANSPORT transport, bool opportunistic,
                                uint8_t initiating_phys);

/*******************************************************************************
 *
 * Function         GATT_CancelConnect
 *
 * Description      Terminate the connection initiation to a remote device on a
 *                  GATT channel.
 *
 * Parameters       gatt_if: client interface. If 0 used as unconditionally
 *                           disconnect, typically used for direct connection
 *                           cancellation.
 *                  bd_addr: peer device address.
 *                  is_direct: is a direct connection or a background auto
 *                             connection
 *
 * Returns          true if connection started; else false
 *
 ******************************************************************************/
[[nodiscard]] bool GATT_CancelConnect(tGATT_IF gatt_if,
                                      const RawAddress& bd_addr,
                                      bool is_direct);

/*******************************************************************************
 *
 * Function         GATT_Disconnect
 *
 * Description      Disconnect the GATT channel for this registered application.
 *
 * Parameters       conn_id: connection identifier.
 *
 * Returns          GATT_SUCCESS if disconnected.
 *
 ******************************************************************************/
[[nodiscard]] tGATT_STATUS GATT_Disconnect(uint16_t conn_id);

/*******************************************************************************
 *
 * Function         GATT_GetConnectionInfor
 *
 * Description      Use conn_id to find its associated BD address and
 *                  application interface
 *
 * Parameters        conn_id: connection id  (input)
 *                   p_gatt_if: application interface (output)
 *                   bd_addr: peer device address. (output)
 *                   transport : physical transport of the GATT connection
 *                                (BR/EDR or LE)
 *
 * Returns          true the logical link information is found for conn_id
 *
 ******************************************************************************/
[[nodiscard]] bool GATT_GetConnectionInfor(uint16_t conn_id,
                                           tGATT_IF* p_gatt_if,
                                           RawAddress& bd_addr,
                                           tBT_TRANSPORT* p_transport);

/*******************************************************************************
 *
 * Function         GATT_GetConnIdIfConnected
 *
 * Description      Find the conn_id if the logical link for a BD address
 *                  and application interface is connected
 *
 * Parameters        gatt_if: application interface (input)
 *                   bd_addr: peer device address. (input)
 *                   p_conn_id: connection id  (output)
 *                   transport :  physical transport of the GATT connection
 *                               (BR/EDR or LE)
 *
 * Returns          true the logical link is connected
 *
 ******************************************************************************/
[[nodiscard]] bool GATT_GetConnIdIfConnected(tGATT_IF gatt_if,
                                             const RawAddress& bd_addr,
                                             uint16_t* p_conn_id,
                                             tBT_TRANSPORT transport);

/*******************************************************************************
 *
 * Function         GATT_ConfigServiceChangeCCC
 *
 * Description      Configure service change indication on remote device
 *
 * Returns          None.
 *
 ******************************************************************************/
void GATT_ConfigServiceChangeCCC(const RawAddress& remote_bda, bool enable,
                                 tBT_TRANSPORT transport);

// Enables the GATT profile on the device.
// It clears out the control blocks, and registers with L2CAP.
void gatt_init(void);

// Frees resources used by the GATT profile.
void gatt_free(void);

// Link encryption complete notification for all encryption process
// initiated outside GATT.
void gatt_notify_enc_cmpl(const RawAddress& bd_addr);

/** Reset bg device list. If called after controller reset, set |after_reset| to
 * true, as there is no need to wipe controller acceptlist in this case. */
void gatt_reset_bgdev_list(bool after_reset);

// Initialize GATTS list of bonded device service change updates.
void gatt_load_bonded(void);

namespace fmt {
template <>
struct formatter<GattStatus> : enum_formatter<GattStatus> {};
template <>
struct formatter<tGATTC_OPTYPE> : enum_formatter<tGATTC_OPTYPE> {};
template <>
struct formatter<tGATT_OP_CODE> : enum_formatter<tGATT_OP_CODE> {};
template <>
struct formatter<tGATT_DISC_TYPE> : enum_formatter<tGATT_DISC_TYPE> {};
template <>
struct formatter<tGATT_DISCONN_REASON> : enum_formatter<tGATT_DISCONN_REASON> {};
}  // namespace fmt

#endif /* GATT_API_H */
