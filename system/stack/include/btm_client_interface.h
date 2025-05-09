/*
 * Copyright 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstdint>

#include "device/include/esco_parameters.h"
#include "stack/btm/neighbor_inquiry.h"
#include "stack/btm/power_mode.h"
#include "stack/include/acl_client_callbacks.h"
#include "stack/include/bt_hdr.h"
#include "stack/include/btm_api.h"
#include "stack/include/btm_api_types.h"
#include "stack/include/btm_ble_api_types.h"
#include "stack/include/btm_status.h"
#include "stack/include/btm_vendor_types.h"
#include "stack/include/security_client_callbacks.h"
#include "types/bt_transport.h"
#include "types/raw_address.h"

struct btm_client_interface_t {
  struct {
    [[nodiscard]] tBTM_STATUS (*BTM_PmRegister)(uint8_t mask, uint8_t* p_pm_id,
                                                tBTM_PM_STATUS_CBACK* p_cback);
    [[nodiscard]] uint16_t (*BTM_GetHCIConnHandle)(const RawAddress& bd_addr,
                                                   tBT_TRANSPORT transport);
    void (*BTM_VendorSpecificCommand)(uint16_t opcode, uint8_t param_len,
                                      uint8_t* p_param_buf,
                                      tBTM_VSC_CMPL_CB* p_cb);
    void (*ACL_RegisterClient)(struct acl_client_callback_s* callbacks);
    void (*ACL_UnregisterClient)(struct acl_client_callback_s* callbacks);
    void (*btm_init)();
    void (*btm_free)();
    void (*btm_ble_init)();
    void (*btm_ble_free)();
    void (*BTM_reset_complete)();
  } lifecycle;

  // Acl peer and lifecycle
  struct {
    [[nodiscard]] bool (*BTM_IsAclConnectionUp)(const RawAddress& bd_addr,
                                                tBT_TRANSPORT transport);
    [[nodiscard]] bool (*BTM_ReadConnectedTransportAddress)(
        RawAddress* bd_addr, tBT_TRANSPORT transport);
    [[nodiscard]] tBTM_STATUS (*BTM_CancelRemoteDeviceName)(void);
    [[nodiscard]] tBTM_STATUS (*BTM_ReadRemoteDeviceName)(
        const RawAddress& bd_addr, tBTM_NAME_CMPL_CB* p_cb,
        tBT_TRANSPORT transport);
    [[nodiscard]] uint8_t* (*BTM_ReadRemoteFeatures)(const RawAddress&);
    void (*BTM_ReadDevInfo)(const RawAddress& bd_addr,
                            tBT_DEVICE_TYPE* p_dev_type,
                            tBLE_ADDR_TYPE* p_addr_type);
    [[nodiscard]] uint16_t (*BTM_GetMaxPacketSize)(const RawAddress& bd_addr);
    [[nodiscard]] bool (*BTM_ReadRemoteVersion)(const RawAddress& addr,
                                                uint8_t* lmp_version,
                                                uint16_t* manufacturer,
                                                uint16_t* lmp_sub_version);
    [[nodiscard]] tBT_DEVICE_TYPE (*BTM_GetPeerDeviceTypeFromFeatures)(
        const RawAddress& bd_addr);
    void (*BTM_RequestPeerSCA)(const RawAddress& remote_bda,
                               tBT_TRANSPORT transport);
    [[nodiscard]] uint8_t (*BTM_GetPeerSCA)(const RawAddress& remote_bda,
                                            tBT_TRANSPORT transport);
    [[nodiscard]] bool (*BTM_IsPhy2mSupported)(const RawAddress& remote_bda,
                                               tBT_TRANSPORT transport);
  } peer;

  struct {
    [[nodiscard]] tBTM_STATUS (*BTM_GetRole)(const RawAddress& remote_bd_addr,
                                             tHCI_ROLE* p_role);
    [[nodiscard]] tBTM_STATUS (*BTM_SetPowerMode)(uint8_t pm_id,
                                                  const RawAddress& bd_addr,
                                                  const tBTM_PM_PWR_MD* p_mode);
    [[nodiscard]] tBTM_STATUS (*BTM_SetSsrParams)(const RawAddress& bd_addr,
                                                  uint16_t max_lat,
                                                  uint16_t min_rmt_to,
                                                  uint16_t min_loc_to);
    [[nodiscard]] tBTM_STATUS (*BTM_SwitchRoleToCentral)(
        const RawAddress& remote_bd_addr);
    void (*BTM_block_role_switch_for)(const RawAddress& peer_addr);
    void (*BTM_block_sniff_mode_for)(const RawAddress& peer_addr);
    void (*BTM_default_unblock_role_switch)();
    void (*BTM_unblock_role_switch_for)(const RawAddress& peer_addr);
    void (*BTM_unblock_sniff_mode_for)(const RawAddress& peer_addr);
    void (*BTM_WritePageTimeout)(uint16_t timeout);
  } link_policy;

  struct {
    [[nodiscard]] tBTM_STATUS (*BTM_GetLinkSuperTout)(const RawAddress& bd_addr,
                                                      uint16_t* p_timeout);
    [[nodiscard]] tBTM_STATUS (*BTM_ReadRSSI)(const RawAddress& bd_addr,
                                              tBTM_CMPL_CB* p_cb);
  } link_controller;

  SecurityClientInterface security;
  struct {
    [[nodiscard]] tBTM_STATUS (*BTM_BleGetEnergyInfo)(
        tBTM_BLE_ENERGY_INFO_CBACK* callback);
    [[nodiscard]] tBTM_STATUS (*BTM_BleObserve)(
        bool start, uint8_t duration, tBTM_INQ_RESULTS_CB* p_results_cb,
        tBTM_CMPL_CB* p_cmpl_cb, bool low_latency_scan);
    [[nodiscard]] tBTM_STATUS (*BTM_SetBleDataLength)(const RawAddress& bd_addr,
                                                      uint16_t tx_pdu_length);
    void (*BTM_BleReadControllerFeatures)(
        tBTM_BLE_CTRL_FEATURES_CBACK* p_vsc_cback);
    void (*BTM_BleSetPhy)(const RawAddress& bd_addr, uint8_t tx_phys,
                          uint8_t rx_phys, uint16_t phy_options);
    void (*BTM_BleSetPrefConnParams)(const RawAddress& bd_addr,
                                     uint16_t min_conn_int,
                                     uint16_t max_conn_int,
                                     uint16_t peripheral_latency,
                                     uint16_t supervision_tout);
    [[nodiscard]] bool (*BTM_UseLeLink)(const RawAddress& bd_addr);
    [[nodiscard]] bool (*BTM_IsRemoteVersionReceived)(
        const RawAddress& remote_bda);
    void (*BTM_SetConsolidationCallback)(BTM_CONSOLIDATION_CB* cb);
  } ble;

  struct {
    [[nodiscard]] tBTM_STATUS (*BTM_CreateSco)(const RawAddress* bd_addr,
                                               bool is_orig, uint16_t pkt_types,
                                               uint16_t* p_sco_inx,
                                               tBTM_SCO_CB* p_conn_cb,
                                               tBTM_SCO_CB* p_disc_cb);
    [[nodiscard]] tBTM_STATUS (*BTM_RegForEScoEvts)(
        uint16_t sco_inx, tBTM_ESCO_CBACK* p_esco_cback);
    [[nodiscard]] tBTM_STATUS (*BTM_RemoveSco)(uint16_t sco_inx);
    void (*BTM_WriteVoiceSettings)(uint16_t settings);
    void (*BTM_EScoConnRsp)(uint16_t sco_inx, tHCI_STATUS hci_status,
                            enh_esco_params_t* p_parms);
    [[nodiscard]] uint8_t (*BTM_GetNumScoLinks)();
    [[nodiscard]] tBTM_STATUS (*BTM_SetEScoMode)(enh_esco_params_t* p_parms);
    [[nodiscard]] tBTM_SCO_DEBUG_DUMP (*BTM_GetScoDebugDump)(void);
    [[nodiscard]] bool (*BTM_IsScoActiveByBdaddr)(const RawAddress& remote_bda);
  } sco;

  struct {
    [[nodiscard]] tBTM_STATUS (*BTM_ReadLocalDeviceName)(const char** p_name);
    [[nodiscard]] tBTM_STATUS (*BTM_ReadLocalDeviceNameFromController)(
        tBTM_CMPL_CB* p_rln_cmpl_cback);
    [[nodiscard]] tBTM_STATUS (*BTM_SetLocalDeviceName)(const char* p_name);
    [[nodiscard]] tBTM_STATUS (*BTM_SetDeviceClass)(DEV_CLASS dev_class);
    [[nodiscard]] bool (*BTM_IsDeviceUp)();
    [[nodiscard]] DEV_CLASS (*BTM_ReadDeviceClass)();
  } local;

  struct {
    [[nodiscard]] tBTM_STATUS (*BTM_WriteEIR)(BT_HDR* p_buff);
    [[nodiscard]] uint8_t (*BTM_GetEirSupportedServices)(uint32_t* p_eir_uuid,
                                                         uint8_t** p,
                                                         uint8_t max_num_uuid16,
                                                         uint8_t* p_num_uuid16);
    [[nodiscard]] uint8_t (*BTM_GetEirUuidList)(
        const uint8_t* p_eir, size_t eir_len, uint8_t uuid_size,
        uint8_t* p_num_uuid, uint8_t* p_uuid_list, uint8_t max_num_uuid);
    void (*BTM_AddEirService)(uint32_t* p_eir_uuid, uint16_t uuid16);
    void (*BTM_RemoveEirService)(uint32_t* p_eir_uuid, uint16_t uuid16);
  } eir;

  struct {
    [[nodiscard]] tBTM_INQ_INFO* (*BTM_InqDbRead)(const RawAddress& p_bda);
    [[nodiscard]] tBTM_INQ_INFO* (*BTM_InqDbFirst)();
    [[nodiscard]] tBTM_INQ_INFO* (*BTM_InqDbNext)(tBTM_INQ_INFO* p_cur);
    [[nodiscard]] tBTM_STATUS (*BTM_ClearInqDb)(const RawAddress* p_bda);
  } db;

  struct {
    void (*BTM_ReadVendorAddOnFeatures)();
    bt_device_host_add_on_features_t* (*BTM_GetHostAddOnFeatures)(
        uint8_t* host_add_on_features_len);
    bt_device_soc_add_on_features_t* (*BTM_GetSocAddOnFeatures)(
        uint8_t* soc_add_on_features_len);
    bt_device_qll_local_supported_features_t* (*BTM_GetQllLocalSupportedFeatures)(
        uint8_t* qll_local_supported_features_len);
    char* (*BTM_GetA2dpOffloadCapablity)();
    bool (*BTM_IsSpiltA2dpSupported)();
    bool (*BTM_IsAACFrameCtrlEnabled)();
    uint8_t* (*BTM_GetScramblingSupportedFreqs)(uint8_t* number_of_freqs);
    bool (*BTM_IsQHSPhySupported)(const RawAddress& bda,
                                  tBT_TRANSPORT transport);
    void (*BTM_SetPowerBackOffState)(bool status);
  } vendor;
};

struct btm_client_interface_t& get_btm_client_interface();
