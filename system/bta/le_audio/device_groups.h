/*
 * Copyright 2023 The Android Open Source Project
 * Copyright 2020 HIMSA II K/S - www.himsa.com. Represented by EHIMA
 * - www.ehima.com
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

/* LeAudioDeviceGroup class represents group of LeAudioDevices and allows to
 * perform operations on them. Group states are ASE states due to nature of
 * group which operates finally of ASEs.
 *
 * Group is created after adding a node to new group id (which is not on list).
 */

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <utility>  // for std::pair
#include <vector>

#ifdef __ANDROID__
#include <android/sysprop/BluetoothProperties.sysprop.h>
#endif

#include <bluetooth/log.h>
#include <com_android_bluetooth_flags.h>

#include "common/strings.h"
#include "devices.h"
#include "le_audio_log_history.h"
#include "le_audio_types.h"

namespace bluetooth::le_audio {

class LeAudioDeviceGroup {
 public:
  const int group_id_;

  class CigConfiguration {
   public:
    CigConfiguration() = delete;
    CigConfiguration(LeAudioDeviceGroup* group)
        : group_(group), state_(types::CigState::NONE) {}

    types::CigState GetState(void) const { return state_; }
    void SetState(bluetooth::le_audio::types::CigState state) {
      log::verbose("{} -> {}", bluetooth::common::ToString(state_),
                   bluetooth::common::ToString(state));
      state_ = state;
    }

    void GenerateCisIds(types::LeAudioContextType context_type);
    bool AssignCisIds(LeAudioDevice* leAudioDevice);
    void AssignCisConnHandles(const std::vector<uint16_t>& conn_handles);
    void UnassignCis(LeAudioDevice* leAudioDevice);

    std::vector<struct types::cis> cises;

   private:
    uint8_t GetFirstFreeCisId(types::CisType cis_type) const;

    LeAudioDeviceGroup* group_;
    types::CigState state_;
  } cig;

  bool IsGroupConfiguredTo(
      const set_configurations::AudioSetConfiguration& cfg) {
    if (!stream_conf.conf) return false;
    return cfg == *stream_conf.conf;
  }

  /* Current configuration strategy - recalculated on demand */
  mutable std::optional<types::LeAudioConfigurationStrategy> strategy_ =
      std::nullopt;

  /* Current audio stream configuration */
  struct stream_configuration stream_conf;
  bool notify_streaming_when_cises_are_ready_;

  uint8_t audio_directions_;
  types::AudioLocations snk_audio_locations_;
  types::AudioLocations src_audio_locations_;

  /* Whether LE Audio is preferred for OUTPUT_ONLY and DUPLEX cases */
  bool is_output_preference_le_audio;
  bool is_duplex_preference_le_audio;
  std::pair<bool, bool> lex_codec_disabled;

  struct {
    DsaMode mode;
    bool active;
  } dsa_;
  bool asymmetric_phy_for_unidirectional_cis_supported;

  explicit LeAudioDeviceGroup(const int group_id)
      : group_id_(group_id),
        cig(this),
        stream_conf({}),
        notify_streaming_when_cises_are_ready_(false),
        audio_directions_(0),
        dsa_({DsaMode::DISABLED, false}),
        is_enabled_(true),
        transport_latency_mtos_us_(0),
        transport_latency_stom_us_(0),
        configuration_context_type_(types::LeAudioContextType::UNINITIALIZED),
        metadata_context_type_({.sink = types::AudioContexts(
                                    types::LeAudioContextType::UNINITIALIZED),
                                .source = types::AudioContexts(
                                    types::LeAudioContextType::UNINITIALIZED)}),
        group_available_contexts_(
            {.sink =
                 types::AudioContexts(types::LeAudioContextType::UNINITIALIZED),
             .source = types::AudioContexts(
                 types::LeAudioContextType::UNINITIALIZED)}),
        pending_group_available_contexts_change_(
            types::LeAudioContextType::UNINITIALIZED),
        group_user_allowed_context_mask_(
            {.sink = types::AudioContexts(types::kLeAudioContextAllTypes),
             .source = types::AudioContexts(types::kLeAudioContextAllTypes)}),
        target_state_(types::AseState::BTA_LE_AUDIO_ASE_STATE_IDLE),
        current_state_(types::AseState::BTA_LE_AUDIO_ASE_STATE_IDLE),
        in_transition_(false), suspended_for_reconfig_(false) {
#ifdef __ANDROID__
    // 22 maps to BluetoothProfile#LE_AUDIO
    is_output_preference_le_audio = android::sysprop::BluetoothProperties::
                                        getDefaultOutputOnlyAudioProfile() ==
                                    LE_AUDIO_PROFILE_CONSTANT;
    is_duplex_preference_le_audio =
        android::sysprop::BluetoothProperties::getDefaultDuplexAudioProfile() ==
        LE_AUDIO_PROFILE_CONSTANT;
#else
    is_output_preference_le_audio = true;
    is_duplex_preference_le_audio = true;
#endif
    lex_codec_disabled = std::make_pair(false, false);
    asymmetric_phy_for_unidirectional_cis_supported =
        com::android::bluetooth::flags::asymmetric_phy_for_unidirectional_cis();
  }
  ~LeAudioDeviceGroup(void);

  void AddNode(const std::shared_ptr<LeAudioDevice>& leAudioDevice);
  void RemoveNode(const std::shared_ptr<LeAudioDevice>& leAudioDevice);
  bool IsEmpty(void) const;
  bool IsAnyDeviceConnected(void) const;
  int Size(void) const;
  int DesiredSize(void) const;
  int NumOfConnected() const;
  int NumOfAvailableForDirection(int direction) const;
  bool Activate(types::LeAudioContextType context_type,
                const types::BidirectionalPair<types::AudioContexts>&
                    metadata_context_types,
                types::BidirectionalPair<std::vector<uint8_t>> ccid_lists);
  void Deactivate(void);
  void ClearSinksFromConfiguration(void);
  void ClearSourcesFromConfiguration(void);
  void Cleanup(void);
  LeAudioDevice* GetFirstDevice(void) const;
  LeAudioDevice* GetFirstDeviceWithAvailableContext(
      types::LeAudioContextType context_type) const;
  types::LeAudioConfigurationStrategy GetGroupSinkStrategy(void) const;
  inline void InvalidateGroupStrategy(void) { strategy_ = std::nullopt; }
  int GetAseCount(uint8_t direction) const;
  LeAudioDevice* GetNextDevice(LeAudioDevice* leAudioDevice) const;
  LeAudioDevice* GetNextDeviceWithAvailableContext(
      LeAudioDevice* leAudioDevice,
      types::LeAudioContextType context_type) const;
  LeAudioDevice* GetFirstActiveDevice(void) const;
  LeAudioDevice* GetNextActiveDevice(LeAudioDevice* leAudioDevice) const;
  LeAudioDevice* GetFirstActiveDeviceByCisAndDataPathState(
      types::CisState cis_state, types::DataPathState data_path_state) const;
  LeAudioDevice* GetNextActiveDeviceByCisAndDataPathState(
      LeAudioDevice* leAudioDevice, types::CisState cis_state,
      types::DataPathState data_path_state) const;
  bool IsDeviceInTheGroup(LeAudioDevice* leAudioDevice) const;
  bool HaveAllActiveDevicesAsesTheSameState(types::AseState state) const;
  bool HaveAnyActiveDeviceInUnconfiguredState() const;
  bool IsGroupStreamReady(void) const;
  bool IsGroupReadyToCreateStream(void) const;
  bool IsGroupReadyToSuspendStream(void) const;
  bool IsSeamlessSupported(void);
  void DisableLeXCodec(bool status);
  void PopulateVendorMetadatabyDirection(types::LeAudioContextType context_type,
                                         uint8_t direction, types::LeAudioLtvMap pacs_metadata,
                                         const set_configurations::AseConfiguration& conf) const;
  bool HaveAllCisesDisconnected(void) const;
  void ClearAllCises(void);
  void UpdateCisConfiguration(uint8_t direction);
  void AssignCisConnHandlesToAses(LeAudioDevice* leAudioDevice);
  void AssignCisConnHandlesToAses(void);
  bool Configure(types::LeAudioContextType context_type,
                 const types::BidirectionalPair<types::AudioContexts>&
                     metadata_context_types,
                 types::BidirectionalPair<std::vector<uint8_t>> ccid_lists = {
                     .sink = {}, .source = {}});
  uint32_t GetSduInterval(uint8_t direction) const;
  uint8_t GetSCA(void) const;
  uint8_t GetPacking(void) const;
  uint8_t GetFraming(void) const;
  uint16_t GetMaxTransportLatencyStom(void) const;
  uint16_t GetMaxTransportLatencyMtos(void) const;
  void SetTransportLatency(uint8_t direction, uint32_t transport_latency_us);
  uint8_t GetRtn(uint8_t direction, uint8_t cis_id) const;
  uint16_t GetMaxSduSize(uint8_t direction, uint8_t cis_id) const;
  uint8_t GetPhyBitmask(uint8_t direction) const;
  uint8_t GetTargetPhy(uint8_t direction) const;
  bool GetPresentationDelay(uint32_t* delay, uint8_t direction) const;
  uint16_t GetRemoteDelay(uint8_t direction) const;
  bool UpdateAudioContextAvailability(void);
  bool UpdateAudioSetConfigurationCache(
      types::LeAudioContextType ctx_type) const;
  CodecManager::UnicastConfigurationRequirements
  GetAudioSetConfigurationRequirements(
      types::LeAudioContextType ctx_type) const;
  bool ReloadAudioLocations(void);
  bool ReloadAudioDirections(void);
  std::shared_ptr<const set_configurations::AudioSetConfiguration>
  GetActiveConfiguration(void) const;
  bool IsPendingConfiguration(void) const;
  std::shared_ptr<const set_configurations::AudioSetConfiguration>
  GetConfiguration(types::LeAudioContextType ctx_type) const;
  std::shared_ptr<const set_configurations::AudioSetConfiguration>
  GetCachedConfiguration(types::LeAudioContextType ctx_type) const;
  void InvalidateCachedConfigurations(void);
  void SetPendingConfiguration(void);
  void ClearPendingConfiguration(void);
  void AddToAllowListNotConnectedGroupMembers(int gatt_if);
  void ApplyReconnectionMode(int gatt_if, tBTM_BLE_CONN_TYPE reconnection_mode);
  void Disable(int gatt_if);
  void Enable(int gatt_if, tBTM_BLE_CONN_TYPE reconnection_mode);
  bool IsEnabled(void) const;
  LeAudioCodecConfiguration GetAudioSessionCodecConfigForDirection(
      types::LeAudioContextType group_context_type, uint8_t direction) const;
  bool HasCodecConfigurationForDirection(
      types::LeAudioContextType group_context_type, uint8_t direction) const;
  bool IsAudioSetConfigurationAvailable(
      types::LeAudioContextType group_context_type);
  bool IsLeXDevice(void) const ;
  bool IsMetadataChanged(
      const types::BidirectionalPair<types::AudioContexts>& context_types,
      const types::BidirectionalPair<std::vector<uint8_t>>& ccid_lists) const;
  bool IsConfiguredForContext(types::LeAudioContextType context_type) const;
  void RemoveCisFromStreamIfNeeded(LeAudioDevice* leAudioDevice,
                                   uint16_t cis_conn_hdl);
  bool IsSuspendedForReconfiguration(void) const;
  void SetSuspendedForReconfiguration(void);
  void ClearSuspendedForReconfiguration(void);

  bool IsReconfigStartPendingDir(uint8_t direction) const;
  void SetReconfigStartPendingDirs(uint8_t directions);
  void ClearReconfigStartPendingDirs(uint8_t directions);

  inline types::AseState GetState(void) const {
    log::info("current_state_: {}", bluetooth::common::ToString(current_state_));
    return current_state_;
  }
  void SetState(types::AseState state) {
    log::info("current state: {}, new state {}, in_transition_ {}",
              bluetooth::common::ToString(current_state_),
              bluetooth::common::ToString(state), in_transition_);
    LeAudioLogHistory::Get()->AddLogHistory(
        kLogStateMachineTag, group_id_, RawAddress::kEmpty, kLogStateChangedOp,
        bluetooth::common::ToString(current_state_) + "->" +
            bluetooth::common::ToString(state));
    current_state_ = state;

    if (target_state_ == current_state_) {
      in_transition_ = false;
      log::info("In transition flag cleared");
    }
  }

  inline types::AseState GetTargetState(void) const {
    log::info("target_state_: {}", bluetooth::common::ToString(target_state_));
    return target_state_;
  }
  inline void SetNotifyStreamingWhenCisesAreReadyFlag(bool value) {
    notify_streaming_when_cises_are_ready_ = value;
  }
  inline bool GetNotifyStreamingWhenCisesAreReadyFlag(void) {
    return notify_streaming_when_cises_are_ready_;
  }
  void SetTargetState(types::AseState state) {
    log::info("target state: {}, new target state: {}, in_transition_ {}",
              bluetooth::common::ToString(target_state_),
              bluetooth::common::ToString(state), in_transition_);
    LeAudioLogHistory::Get()->AddLogHistory(
        kLogStateMachineTag, group_id_, RawAddress::kEmpty,
        kLogTargetStateChangedOp,
        bluetooth::common::ToString(target_state_) + "->" +
            bluetooth::common::ToString(state));

    target_state_ = state;

    in_transition_ = target_state_ != current_state_;
    log::info("In transition flag  = {}", in_transition_);
  }

  /* Returns context types for which support was recently added or removed */
  inline types::AudioContexts GetPendingAvailableContextsChange() const {
    return pending_group_available_contexts_change_;
  }

  /* Set which context types were recently added or removed */
  inline void SetPendingAvailableContextsChange(
      types::AudioContexts audio_contexts) {
    pending_group_available_contexts_change_ = audio_contexts;
  }

  inline void ClearPendingAvailableContextsChange() {
    pending_group_available_contexts_change_.clear();
  }

  inline void SetConfigurationContextType(
      types::LeAudioContextType context_type) {
    log::debug("context_type = {}.", bluetooth::common::ToString(context_type));
    configuration_context_type_ = context_type;
  }

  inline types::LeAudioContextType GetConfigurationContextType(void) const {
    log::debug("configuration_context_type_= {}.",
                     bluetooth::common::ToString(configuration_context_type_));
    return configuration_context_type_;
  }

  inline types::BidirectionalPair<types::AudioContexts> GetMetadataContexts()
      const {
    return metadata_context_type_;
  }

  inline std::vector<uint8_t> GetCodecVendorMetadata(uint8_t direction,
    types::LeAudioContextType context_type) {
   return (direction == le_audio::types::kLeAudioDirectionSink) ?
    sink_context_to_vendor_metadata_map[context_type] :
    source_context_to_vendor_metadata_map[context_type];
  }

  inline void SetAvailableContexts(
      types::BidirectionalPair<types::AudioContexts> new_contexts) {
    group_available_contexts_ = new_contexts;
    log::debug(
        "group id: {}, available contexts sink: {}, available contexts source: "
        "{}",
        group_id_, group_available_contexts_.sink.to_string(),
        group_available_contexts_.source.to_string());
  }

  types::AudioContexts GetAvailableContexts(
      int direction = types::kLeAudioDirectionBoth) const {
    log::assert_that(direction <= (types::kLeAudioDirectionBoth),
                     "Invalid direction used.");
    if (direction < types::kLeAudioDirectionBoth) {
      log::debug(
          "group id: {}, available contexts sink: {}, available contexts "
          "source: {}",
          group_id_, group_available_contexts_.sink.to_string(),
          group_available_contexts_.source.to_string());
      return group_available_contexts_.get(direction);
    } else {
      return types::get_bidirectional(group_available_contexts_);
    }
  }

  inline void SetAllowedContextMask(
      types::BidirectionalPair<types::AudioContexts>& context_types) {
    group_user_allowed_context_mask_ = context_types;
    log::debug(
        "group id: {}, allowed contexts sink: {}, allowed contexts source: "
        "{}",
        group_id_, group_user_allowed_context_mask_.sink.to_string(),
        group_user_allowed_context_mask_.source.to_string());
  }

  types::AudioContexts GetAllowedContextMask(
      int direction = types::kLeAudioDirectionBoth) const {
    log::assert_that(direction <= (types::kLeAudioDirectionBoth),
                     "Invalid direction used.");
    if (direction < types::kLeAudioDirectionBoth) {
      log::debug(
          "group id: {}, allowed contexts sink: {}, allowed contexts "
          "source: {}",
          group_id_, group_user_allowed_context_mask_.sink.to_string(),
          group_user_allowed_context_mask_.source.to_string());
      return group_user_allowed_context_mask_.get(direction);
    } else {
      return types::get_bidirectional(group_user_allowed_context_mask_);
    }
  }

  types::AudioContexts GetSupportedContexts(
      int direction = types::kLeAudioDirectionBoth) const;

  DsaModes GetAllowedDsaModes() {
    if (!com::android::bluetooth::flags::leaudio_dynamic_spatial_audio()) {
      return {DsaMode::DISABLED};
    }

    DsaModes dsa_modes{};
    std::set<DsaMode> dsa_mode_set{};

    for (auto leAudioDevice : leAudioDevices_) {
      if (leAudioDevice.expired()) continue;

      auto device_dsa_modes = leAudioDevice.lock()->GetDsaModes();

      dsa_mode_set.insert(device_dsa_modes.begin(), device_dsa_modes.end());
    }

    dsa_modes.assign(dsa_mode_set.begin(), dsa_mode_set.end());

    return dsa_modes;
  }

  std::vector<DsaModes> GetAllowedDsaModesList() {
    std::vector<DsaModes> dsa_modes_list = {};
    for (auto leAudioDevice : leAudioDevices_) {
      DsaModes dsa_modes = {};

      if (!leAudioDevice.expired()) {
        dsa_modes = leAudioDevice.lock()->GetDsaModes();
      }
      dsa_modes_list.push_back(dsa_modes);
    }
    return dsa_modes_list;
  }

  types::BidirectionalPair<types::AudioContexts> GetLatestAvailableContexts(
      void) const;

  bool IsInTransition(void) const;
  bool IsStreaming(void) const;
  bool IsReleasingOrIdle(void) const;

  void PrintDebugState(void) const;
  void Dump(int fd, int active_group_id) const;

  /* Codec configuration matcher supporting the legacy configuration provider
   * mechanism for the non-vendor and software codecs. Only if the codec
   * parameters are using the common LTV data format, the BT stack can verify
   * them against the remote device capabilities and find the best possible
   * configurations. This will not be used for finding best possible vendor
   * codec configuration.
   */
  const set_configurations::AudioSetConfiguration*
  FindFirstSupportedConfiguration(
      const CodecManager::UnicastConfigurationRequirements& requirements,
      const set_configurations::AudioSetConfigurations* confs) const;

 private:
  bool is_enabled_;

  uint32_t transport_latency_mtos_us_;
  uint32_t transport_latency_stom_us_;

  bool ConfigureAses(
      const set_configurations::AudioSetConfiguration* audio_set_conf,
      types::LeAudioContextType context_type,
      const types::BidirectionalPair<types::AudioContexts>&
          metadata_context_types,
      const types::BidirectionalPair<std::vector<uint8_t>>& ccid_lists);
  bool IsAudioSetConfigurationSupported(
      const CodecManager::UnicastConfigurationRequirements& requirements,
      const set_configurations::AudioSetConfiguration* audio_set_configuration)
      const;
  uint32_t GetTransportLatencyUs(uint8_t direction) const;
  bool IsCisPartOfCurrentStream(uint16_t cis_conn_hdl) const;

  /* Current configuration and metadata context types */
  types::LeAudioContextType configuration_context_type_;
  types::BidirectionalPair<types::AudioContexts> metadata_context_type_;

  /* Mask of contexts that the whole group can handle at its current state
   * It's being updated each time group members connect, disconnect or their
   * individual available audio contexts are changed.
   */
  types::BidirectionalPair<types::AudioContexts> group_available_contexts_;

  /* A temporary mask for bits which were either added or removed when the
   * group available context type changes. It usually means we should refresh
   * our group configuration capabilities to clear this.
   */
  types::AudioContexts pending_group_available_contexts_change_;

  /* Current source metadata context types to vendor metadata map */
  mutable std::map<types::LeAudioContextType,
                std::vector<uint8_t>> source_context_to_vendor_metadata_map;

      /* Current sink metadata context types to vendor metadata map */
  mutable std::map<types::LeAudioContextType,
                  std::vector<uint8_t>> sink_context_to_vendor_metadata_map;


  /* Mask of currently allowed context types. Not set a value not set will
   * result in streaming rejection.
   */
  types::BidirectionalPair<types::AudioContexts>
      group_user_allowed_context_mask_;

  /* Possible configuration cache - refreshed on each group context availability
   * change. Stored as a pair of (is_valid_cache, configuration*). `pair.first`
   * being `false` means that the cached value should be refreshed.
   */
  mutable std::map<
      types::LeAudioContextType,
      std::pair<bool, const std::shared_ptr<
                          set_configurations::AudioSetConfiguration>>>
      context_to_configuration_cache_map;

  types::AseState target_state_;
  types::AseState current_state_;
  bool in_transition_;
  std::vector<std::weak_ptr<LeAudioDevice>> leAudioDevices_;
  bool suspended_for_reconfig_;
  uint8_t reconfig_start_pending_directions_;
};

/* LeAudioDeviceGroup class represents a wraper helper over all device groups in
 * le audio implementation. It allows to operate on device group from a list
 * (vector container) using determinants like id.
 */
class LeAudioDeviceGroups {
 public:
  LeAudioDeviceGroup* Add(int group_id);
  void Remove(const int group_id);
  LeAudioDeviceGroup* FindById(int group_id) const;
  std::vector<int> GetGroupsIds(void) const;
  size_t Size() const;
  bool IsAnyInTransition() const;
  void Cleanup(void);
  void Dump(int fd, int active_group_id) const;

 private:
  std::vector<std::unique_ptr<LeAudioDeviceGroup>> groups_;
};

}  // namespace bluetooth::le_audio
