/*
 * Copyright 2022 The Android Open Source Project
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
#define LOG_TAG "BTAudioA2dpAIDL"

#include "a2dp_encoding_aidl.h"

#include <bluetooth/log.h>

#include <vector>

#include "a2dp_provider_info.h"
#include "a2dp_transport.h"
#include "audio_aidl_interfaces.h"
#include "bta/av/bta_av_int.h"
#include "btif/include/btif_common.h"
#include "codec_status_aidl.h"
#include "transport_instance.h"
#include "hardware/audio.h"
#include "a2dp_aac.h"
#include "a2dp_sbc.h"
#include "a2dp_vendor_ldac_constants.h"
#include <a2dp_vendor.h>
#include "stack/include/btm_client_interface.h"
#include "stack/include/btm_vendor_types.h"
#define AAC_SAMPLE_SIZE  1024
#define AAC_LATM_HEADER  12

/*****************************************************************************
 *  Local type definitions
 *****************************************************************************/
/* Context Types */
enum class LeAudioContextType : uint16_t {
  UNINITIALIZED = 0x0000,
  UNSPECIFIED = 0x0001,
  CONVERSATIONAL = 0x0002,
  MEDIA = 0x0004,
  GAME = 0x0008,
  INSTRUCTIONAL = 0x0010,
  VOICEASSISTANTS = 0x0020,
  LIVE = 0x0040,
  SOUNDEFFECTS = 0x0080,
  NOTIFICATIONS = 0x0100,
  RINGTONE = 0x0200,
  ALERTS = 0x0400,
  EMERGENCYALARM = 0x0800,
  RFU = 0x1000,
};

enum Content {
  CONTENT_TYPE_UNINITIALIZED = 0x0000,
  CONTENT_TYPE_UNSPECIFIED = 0x0001,
  CONTENT_TYPE_CONVERSATIONAL = 0x0002,
  CONTENT_TYPE_MEDIA = 0x0004,
  CONTENT_TYPE_GAME = 0x0008,
  CONTENT_TYPE_INSTRUCTIONAL = 0x0010,
  CONTENT_TYPE_VOICEASSISTANTS = 0x0020,
  CONTENT_TYPE_LIVE = 0x0040,
  CONTENT_TYPE_SOUNDEFFECTS = 0x0080,
  CONTENT_TYPE_NOTIFICATIONS = 0x0100,
  CONTENT_TYPE_RINGTONE = 0x0200,
  CONTENT_TYPE_ALERTS = 0x0400,
  CONTENT_TYPE_EMERGENCYALARM = 0x0800,
  CONTENT_TYPE_RFU = 0x1000,
};

uint16_t LeAudioContextToIntContentInAPM(LeAudioContextType context_type);
//LeAudioContextType priority_context = LeAudioContextType::SOUNDEFFECTS;

enum CONTEXT_PRIORITY {
  SONIFICATION = 0,
  MEDIA,
  GAME,
  CONVERSATIONAL
};

enum METADATA_TYPE {
  SOURCE = 0,
  SINK
};

LeAudioContextType AudioContentToLeAudioContextInAPM(
    audio_content_type_t content_type,
    audio_source_t source_type, audio_usage_t usage) {
  LOG(INFO) << __func__ << ": usage: " << usage;
  switch (usage) {
    case AUDIO_USAGE_MEDIA:
      return LeAudioContextType::MEDIA;
    case AUDIO_USAGE_VOICE_COMMUNICATION:
      return LeAudioContextType::CONVERSATIONAL;
    case AUDIO_USAGE_CALL_ASSISTANT:
      return LeAudioContextType::CONVERSATIONAL;
    case AUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING:
      return LeAudioContextType::VOICEASSISTANTS;
    case AUDIO_USAGE_ASSISTANCE_SONIFICATION:
      return LeAudioContextType::SOUNDEFFECTS;
    case AUDIO_USAGE_GAME:
      return LeAudioContextType::GAME;
    case AUDIO_USAGE_NOTIFICATION:
      return LeAudioContextType::NOTIFICATIONS;
    case AUDIO_USAGE_NOTIFICATION_TELEPHONY_RINGTONE:
      return LeAudioContextType::CONVERSATIONAL;
    case AUDIO_USAGE_ALARM:
      return LeAudioContextType::ALERTS;
    case AUDIO_USAGE_EMERGENCY:
      return LeAudioContextType::EMERGENCYALARM;
    case AUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE:
      return LeAudioContextType::INSTRUCTIONAL;
    default:
      break;
  }

  switch (source_type) {
    case AUDIO_SOURCE_MIC:
    case AUDIO_SOURCE_HOTWORD:
    case AUDIO_SOURCE_VOICE_CALL:
    case AUDIO_SOURCE_VOICE_COMMUNICATION:
      return LeAudioContextType::CONVERSATIONAL;
    default:
      break;
  }

  LOG(INFO) << __func__ << ": Return Media when not in call by default.";
  return LeAudioContextType::MEDIA;
}

uint16_t LeAudioContextToIntContentInAPM(LeAudioContextType context_type) {
  switch (context_type) {
    case LeAudioContextType::MEDIA:
      return CONTENT_TYPE_MEDIA;
    case LeAudioContextType::GAME:
      return CONTENT_TYPE_GAME;
    case LeAudioContextType::CONVERSATIONAL: //Fall through
      return CONTENT_TYPE_CONVERSATIONAL;
    case LeAudioContextType::LIVE:
      return CONTENT_TYPE_LIVE;
    case LeAudioContextType::RINGTONE:
      return CONTENT_TYPE_RINGTONE;
    case LeAudioContextType::VOICEASSISTANTS:
      return CONTENT_TYPE_CONVERSATIONAL;
    case LeAudioContextType::SOUNDEFFECTS:
      return CONTENT_TYPE_SOUNDEFFECTS;
    case LeAudioContextType::ALERTS:
      return CONTENT_TYPE_ALERTS;
    case LeAudioContextType::EMERGENCYALARM:
      return CONTENT_TYPE_EMERGENCYALARM;
    default:
      return CONTENT_TYPE_MEDIA;
      break;
  }
  return 0;
}

int getPriority(LeAudioContextType context) {
  LOG(INFO) << __func__ << ": context type = "<<(uint16_t)context;
  switch (context) {
    case LeAudioContextType::MEDIA:
      return CONTEXT_PRIORITY::MEDIA;
    case LeAudioContextType::GAME:
      return CONTEXT_PRIORITY::GAME;
    case LeAudioContextType::CONVERSATIONAL:
      return CONTEXT_PRIORITY::CONVERSATIONAL;
    case LeAudioContextType::SOUNDEFFECTS:
      return CONTEXT_PRIORITY::SONIFICATION ;
    default:
      break;
  }
  return 0;
}

int context_contention_src(const source_metadata_v7_t& source_metadata) {
  auto track_count = source_metadata.track_count;
  LeAudioContextType current_context = LeAudioContextType::MEDIA;
  auto current_priority = -1;

  LOG(INFO) << __func__ << ": tracks count: " << track_count;
  if(!track_count) {
    return 0;
  }

  for (size_t i = 0; i < track_count; i++) {
    auto context_priority = 0;
    auto track = source_metadata.tracks[i].base;
    if (track.content_type == 0 && track.usage == 0) {
      LOG(INFO) << __func__ << ": tracks count: " << track_count;
      continue;
    }

    LOG(INFO) << __func__
              << ": usage=" << track.usage
              << ", content_type=" << track.content_type
              << ", gain=" << track.gain;
    LeAudioContextType context_type = AudioContentToLeAudioContextInAPM(track.content_type,
                                                    AUDIO_SOURCE_DEFAULT, track.usage);

    context_priority = getPriority(context_type);
    LOG(INFO) << __func__ << ": context_priority: " << context_priority;
    if (context_priority > current_priority)
    {
        current_priority = context_priority;
        current_context = context_type;
    }
  }
  uint16_t ctx = LeAudioContextToIntContentInAPM(current_context);;
  return ctx;
}

namespace fmt {
template <>
struct formatter<tA2DP_CTRL_CMD> : enum_formatter<tA2DP_CTRL_CMD> {};
template <>
struct formatter<audio_usage_t> : enum_formatter<audio_usage_t> {};
template <>
struct formatter<audio_content_type_t> : enum_formatter<audio_content_type_t> {
};
}  // namespace fmt

namespace bluetooth {
namespace audio {
namespace aidl {
namespace a2dp {

namespace {

using ::aidl::android::hardware::bluetooth::audio::A2dpStreamConfiguration;
using ::aidl::android::hardware::bluetooth::audio::AudioConfiguration;
using ::aidl::android::hardware::bluetooth::audio::ChannelMode;
using ::aidl::android::hardware::bluetooth::audio::CodecConfiguration;
using ::aidl::android::hardware::bluetooth::audio::PcmConfiguration;
using ::aidl::android::hardware::bluetooth::audio::SessionType;

using ::bluetooth::audio::aidl::BluetoothAudioCtrlAck;
using ::bluetooth::audio::aidl::BluetoothAudioSinkClientInterface;
using ::bluetooth::audio::aidl::codec::A2dpAacToHalConfig;
using ::bluetooth::audio::aidl::codec::A2dpAptxToHalConfig;
using ::bluetooth::audio::aidl::codec::A2dpAptxAdaptiveToHalConfig;
using ::bluetooth::audio::aidl::codec::A2dpCodecToHalBitsPerSample;
using ::bluetooth::audio::aidl::codec::A2dpCodecToHalChannelMode;
using ::bluetooth::audio::aidl::codec::A2dpCodecToHalSampleRate;
using ::bluetooth::audio::aidl::codec::A2dpLdacToHalConfig;
using ::bluetooth::audio::aidl::codec::A2dpOpusToHalConfig;
using ::bluetooth::audio::aidl::codec::A2dpSbcToHalConfig;

/***
 *
 * A2dpTransport functions and variables
 *
 ***/

tA2DP_CTRL_CMD A2dpTransport::a2dp_pending_cmd_ = A2DP_CTRL_CMD_NONE;
uint16_t A2dpTransport::remote_delay_report_ = 0;

A2dpTransport::A2dpTransport(SessionType sessionType)
    : IBluetoothSinkTransportInstance(sessionType, (AudioConfiguration){}),
      total_bytes_read_(0),
      data_position_({}) {
  a2dp_pending_cmd_ = A2DP_CTRL_CMD_NONE;
  remote_delay_report_ = 0;
}

BluetoothAudioCtrlAck A2dpTransport::StartRequest(bool is_low_latency) {
  // Check if a previous request is not finished
  if (a2dp_pending_cmd_ == A2DP_CTRL_CMD_START) {
    log::info("A2DP_CTRL_CMD_START in progress");
    return a2dp_ack_to_bt_audio_ctrl_ack(A2DP_CTRL_ACK_PENDING);
  } else if (a2dp_pending_cmd_ != A2DP_CTRL_CMD_NONE) {
    log::warn("busy in pending_cmd={}", a2dp_pending_cmd_);
    return a2dp_ack_to_bt_audio_ctrl_ack(A2DP_CTRL_ACK_FAILURE);
  }

  // Don't send START request to stack while we are in a call
  if (!bluetooth::headset::IsCallIdle()) {
    log::error("call state is busy");
    return a2dp_ack_to_bt_audio_ctrl_ack(A2DP_CTRL_ACK_INCALL_FAILURE);
  }

  if (btif_av_stream_started_ready(A2dpType::kSource)) {
    // Already started, ACK back immediately.
    return a2dp_ack_to_bt_audio_ctrl_ack(A2DP_CTRL_ACK_SUCCESS);
  }
  if (btif_av_stream_ready(A2dpType::kSource)) {
    // check if codec needs to be switched prior to stream start
    invoke_switch_codec_cb(is_low_latency);
    /*
     * Post start event and wait for audio path to open.
     * If we are the source, the ACK will be sent after the start
     * procedure is completed, othewise send it now.
     */
    a2dp_pending_cmd_ = A2DP_CTRL_CMD_START;
    btif_av_stream_start_with_latency(is_low_latency);
    if (btif_av_get_peer_sep(A2dpType::kSource) != AVDT_TSEP_SRC) {
      log::info("accepted");
      return a2dp_ack_to_bt_audio_ctrl_ack(A2DP_CTRL_ACK_PENDING);
    }
    a2dp_pending_cmd_ = A2DP_CTRL_CMD_NONE;
    return a2dp_ack_to_bt_audio_ctrl_ack(A2DP_CTRL_ACK_SUCCESS);
  }
  log::error("AV stream is not ready to start");
  return a2dp_ack_to_bt_audio_ctrl_ack(A2DP_CTRL_ACK_FAILURE);
}

BluetoothAudioCtrlAck A2dpTransport::SuspendRequest() {
  // Previous request is not finished
  if (a2dp_pending_cmd_ == A2DP_CTRL_CMD_SUSPEND) {
    log::info("A2DP_CTRL_CMD_SUSPEND in progress");
    return a2dp_ack_to_bt_audio_ctrl_ack(A2DP_CTRL_ACK_PENDING);
  } else if (a2dp_pending_cmd_ != A2DP_CTRL_CMD_NONE) {
    log::warn("busy in pending_cmd={}", a2dp_pending_cmd_);
    return a2dp_ack_to_bt_audio_ctrl_ack(A2DP_CTRL_ACK_FAILURE);
  }
  // Local suspend
  if (btif_av_stream_started_ready(A2dpType::kSource)) {
    log::info("accepted");
    a2dp_pending_cmd_ = A2DP_CTRL_CMD_SUSPEND;
    btif_av_stream_suspend();
    return BluetoothAudioCtrlAck::PENDING;
  }
  /* If we are not in started state, just ack back ok and let
   * audioflinger close the channel. This can happen if we are
   * remotely suspended, clear REMOTE SUSPEND flag.
   */
  btif_av_clear_remote_suspend_flag(A2dpType::kSource);
  return a2dp_ack_to_bt_audio_ctrl_ack(A2DP_CTRL_ACK_SUCCESS);
}

void A2dpTransport::StopRequest() {
  if (btif_av_get_peer_sep(A2dpType::kSource) == AVDT_TSEP_SNK &&
      !btif_av_stream_started_ready(A2dpType::kSource)) {
    btif_av_clear_remote_suspend_flag(A2dpType::kSource);
    return;
  }
  log::info("handling");
  a2dp_pending_cmd_ = A2DP_CTRL_CMD_STOP;
  btif_av_stream_stop(RawAddress::kEmpty);
}

void A2dpTransport::SetLatencyMode(LatencyMode latency_mode) {
  bool is_low_latency = latency_mode == LatencyMode::LOW_LATENCY ? true : false;
  btif_av_set_low_latency(is_low_latency);
  btif_av_set_low_latency_spatial_audio(is_low_latency);
}

bool A2dpTransport::GetPresentationPosition(uint64_t* remote_delay_report_ns,
                                            uint64_t* total_bytes_read,
                                            timespec* data_position) {
  *remote_delay_report_ns = remote_delay_report_ * 100000u;
  *total_bytes_read = total_bytes_read_;
  *data_position = data_position_;
  log::verbose("delay={}/10ms, data={} byte(s), timestamp={}.{}s",
               remote_delay_report_, total_bytes_read_, data_position_.tv_sec,
               data_position_.tv_nsec);
  return true;
}

void A2dpTransport::SourceMetadataChanged(
    const source_metadata_v7_t& source_metadata) {
  auto track_count = source_metadata.track_count;
  auto tracks = source_metadata.tracks;
  log::verbose("{} track(s) received", track_count);
  while (track_count) {
    log::verbose("usage={}, content_type={}, gain={}", tracks->base.usage,
                 tracks->base.content_type, tracks->base.gain);
    --track_count;
    ++tracks;
  }

  uint16_t context = context_contention_src(source_metadata);

  if (btif_av_is_dual_mode_enabled() &&
      (context == CONTENT_TYPE_MEDIA ||
      context == CONTENT_TYPE_GAME)) {
    btif_av_metadata_update(context);
  }

  bool is_gaming = (context == CONTENT_TYPE_GAME) ? true : false;
  btif_av_update_source_metadata(is_gaming);
}

void A2dpTransport::SinkMetadataChanged(const sink_metadata_v7_t&) {}

tA2DP_CTRL_CMD A2dpTransport::GetPendingCmd() const {
  return a2dp_pending_cmd_;
}

void A2dpTransport::ResetPendingCmd() {
  log::info("ResetPendingCmd");
  a2dp_pending_cmd_ = A2DP_CTRL_CMD_NONE;
}

void A2dpTransport::ResetPresentationPosition() {
  remote_delay_report_ = 0;
  total_bytes_read_ = 0;
  data_position_ = {};
}

void A2dpTransport::LogBytesRead(size_t bytes_read) {
  if (bytes_read != 0) {
    total_bytes_read_ += bytes_read;
    clock_gettime(CLOCK_MONOTONIC, &data_position_);
  }
}

/***
 *
 * Global functions and variables
 *
 ***/

// delay reports from AVDTP is based on 1/10 ms (100us)
void A2dpTransport::SetRemoteDelay(uint16_t delay_report) {
  remote_delay_report_ = delay_report;
}

// Common interface to call-out into Bluetooth Audio HAL
BluetoothAudioSinkClientInterface* software_hal_interface = nullptr;
BluetoothAudioSinkClientInterface* offloading_hal_interface = nullptr;
BluetoothAudioSinkClientInterface* active_hal_interface = nullptr;

// ProviderInfo for A2DP hardware offload encoding and decoding data paths,
// if supported by the HAL and enabled. nullptr if not supported
// or disabled.
std::unique_ptr<::bluetooth::audio::aidl::a2dp::ProviderInfo> provider_info;

// Save the value if the remote reports its delay before this interface is
// initialized
uint16_t remote_delay = 0;

bool btaudio_a2dp_disabled = false;
bool is_configured = false;
bool is_low_latency_mode_allowed = false;

BluetoothAudioCtrlAck a2dp_ack_to_bt_audio_ctrl_ack(tA2DP_CTRL_ACK ack) {
  switch (ack) {
    case A2DP_CTRL_ACK_SUCCESS:
      return BluetoothAudioCtrlAck::SUCCESS_FINISHED;
    case A2DP_CTRL_ACK_PENDING:
      return BluetoothAudioCtrlAck::PENDING;
    case A2DP_CTRL_ACK_INCALL_FAILURE:
      return BluetoothAudioCtrlAck::FAILURE_BUSY;
    case A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS:
      return BluetoothAudioCtrlAck::FAILURE_DISCONNECTING;
    case A2DP_CTRL_ACK_UNSUPPORTED: /* Offloading but resource failure */
      return BluetoothAudioCtrlAck::FAILURE_UNSUPPORTED;
    case A2DP_CTRL_ACK_FAILURE:
      return BluetoothAudioCtrlAck::FAILURE;
    default:
      return BluetoothAudioCtrlAck::FAILURE;
  }
}

/// Return the MTU for the active peer audio connection.
static uint16_t a2dp_get_peer_mtu(btav_a2dp_codec_index_t codec_index,
                                  uint8_t const* codec_info) {
  RawAddress peer_addr = btif_av_source_active_peer();
  tA2DP_ENCODER_INIT_PEER_PARAMS peer_params;
  bta_av_co_get_peer_params(peer_addr, &peer_params);
  uint16_t peer_mtu = peer_params.peer_mtu;
  uint16_t effective_mtu = bta_av_co_get_encoder_effective_frame_size();

  if (effective_mtu > 0 && effective_mtu < peer_mtu) {
    peer_mtu = effective_mtu;
  }

  // b/188020925
  // When SBC headsets report middle quality bitpool under a larger MTU, we
  // reduce the packet size to prevent the hardware encoder from putting too
  // many frames in one packet.
  if (codec_index == BTAV_A2DP_CODEC_INDEX_SOURCE_SBC &&
      codec_info[2] /* maxBitpool */ <= A2DP_SBC_BITPOOL_MIDDLE_QUALITY) {
    peer_mtu = MAX_2MBPS_AVDTP_MTU;
  }

  // b/177205770
  // Fix the MTU value not to be greater than an AVDTP packet, so the data
  // encoded by A2DP hardware encoder can be fitted into one AVDTP packet
  // without fragmented
  if (peer_mtu > MAX_3MBPS_AVDTP_MTU) {
    peer_mtu = MAX_3MBPS_AVDTP_MTU;
  }

  return peer_mtu;
}

bool a2dp_get_selected_hal_codec_config(CodecConfiguration* codec_config) {
  A2dpCodecConfig* a2dp_config = bta_av_get_a2dp_current_codec();
  uint8_t p_codec_info[AVDT_CODEC_SIZE];
  if (a2dp_config == nullptr) {
    log::warn("failure to get A2DP codec config");
    return false;
  }
  btav_a2dp_codec_config_t current_codec = a2dp_config->getCodecConfig();
  switch (current_codec.codec_type) {
    case BTAV_A2DP_CODEC_INDEX_SOURCE_SBC:
      [[fallthrough]];
    case BTAV_A2DP_CODEC_INDEX_SINK_SBC: {
      if (!A2dpSbcToHalConfig(codec_config, a2dp_config)) {
        return false;
      }
      break;
    }
    case BTAV_A2DP_CODEC_INDEX_SOURCE_AAC:
      [[fallthrough]];
    case BTAV_A2DP_CODEC_INDEX_SINK_AAC: {
      if (!A2dpAacToHalConfig(codec_config, a2dp_config)) {
        return false;
      }
      break;
    }
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX:
      [[fallthrough]];
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_HD: {
      if (!A2dpAptxToHalConfig(codec_config, a2dp_config)) {
        return false;
      }
      break;
    }
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_ADAPTIVE:
      if (!A2dpAptxAdaptiveToHalConfig(codec_config, a2dp_config)) {
        return false;
      }
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_LDAC: {
      if (!A2dpLdacToHalConfig(codec_config, a2dp_config)) {
        return false;
      }
      break;
    }
    case BTAV_A2DP_CODEC_INDEX_SOURCE_OPUS: {
      if (!A2dpOpusToHalConfig(codec_config, a2dp_config)) {
        return false;
      }
      break;
    }
    case BTAV_A2DP_CODEC_INDEX_MAX:
      [[fallthrough]];
    default:
      log::error("Unknown codec_type={}", current_codec.codec_type);
      return false;
  }
#if 0
  codec_config->encodedAudioBitrate = a2dp_config->getTrackBitRate();
  // Obtain the MTU
  RawAddress peer_addr = btif_av_source_active_peer();
  tA2DP_ENCODER_INIT_PEER_PARAMS peer_param;
  bta_av_co_get_peer_params(peer_addr, &peer_param);
  int effectiveMtu = bta_av_co_get_encoder_effective_frame_size();
  if (effectiveMtu > 0 && effectiveMtu < peer_param.peer_mtu) {
    codec_config->peerMtu = effectiveMtu;
  } else {
    codec_config->peerMtu = peer_param.peer_mtu;
  }
  if (current_codec.codec_type == BTAV_A2DP_CODEC_INDEX_SOURCE_SBC &&
      codec_config->config.get<CodecConfiguration::CodecSpecific::sbcConfig>()
              .maxBitpool <= A2DP_SBC_BITPOOL_MIDDLE_QUALITY) {
    codec_config->peerMtu = MAX_2MBPS_AVDTP_MTU;
  } else if (codec_config->peerMtu > MAX_3MBPS_AVDTP_MTU) {
    codec_config->peerMtu = MAX_3MBPS_AVDTP_MTU;
  }
#endif
  RawAddress peer_addr = btif_av_source_active_peer();
  tA2DP_ENCODER_INIT_PEER_PARAMS peer_param;
  bta_av_co_get_peer_params(peer_addr, &peer_param);
  // Obtain the MTU
  memset(p_codec_info, 0, AVDT_CODEC_SIZE);
  if (!a2dp_config->copyOutOtaCodecConfig(p_codec_info))
  {
    log::error("AIDL No valid codec config");
    return false;
  }
  uint8_t codec_type;
  uint32_t bitrate = 0;
  codec_type = A2DP_GetCodecType((const uint8_t*)p_codec_info);
  codec_config->peerMtu = peer_param.peer_mtu - A2DP_HEADER_SIZE;
  if (A2DP_MEDIA_CT_SBC == codec_type) {
    bitrate = A2DP_GetBitrateSbc();
    log::info("AIDL SBC bitrate: {}", bitrate);
    codec_config->encodedAudioBitrate = bitrate;
  }  else if (A2DP_MEDIA_CT_NON_A2DP == codec_type) {
    int samplerate = A2DP_GetTrackSampleRate(p_codec_info);
    if ((A2DP_VendorCodecGetVendorId(p_codec_info)) == A2DP_LDAC_VENDOR_ID) {
      codec_config->encodedAudioBitrate = a2dp_config->getTrackBitRate();
      log::info("AIDL LDAC bitrate: {}", codec_config->encodedAudioBitrate);
    } else {
      /* BR = (Sampl_Rate * PCM_DEPTH * CHNL)/Compression_Ratio */
      int bits_per_sample = 16; // TODO
      codec_config->encodedAudioBitrate = (samplerate * bits_per_sample * 2)/4;
      log::info("AIDL Aptx bitrate: {}", codec_config->encodedAudioBitrate);
    }
  } else if (A2DP_MEDIA_CT_AAC == codec_type) {
    bool is_AAC_frame_ctrl_stack_enable =
                    get_btm_client_interface().vendor.BTM_IsAACFrameCtrlEnabled();
    uint32_t codec_based_bit_rate = 0;
    uint32_t mtu_based_bit_rate = 0;
    log::info("AIDL Stack AAC frame control enabled: {}",
                                                is_AAC_frame_ctrl_stack_enable);
    tA2DP_AAC_CIE aac_cie;
    if(!A2DP_GetAacCIE(p_codec_info, &aac_cie)) {
      log::error("AIDL : Unable to get AAC CIE");
      return false;
    }
    codec_based_bit_rate = aac_cie.bitRate;
    if (is_AAC_frame_ctrl_stack_enable) {
      int sample_rate = A2DP_GetTrackSampleRate(p_codec_info);
      mtu_based_bit_rate = (peer_param.peer_mtu - AAC_LATM_HEADER)
                                          * (8 * sample_rate / AAC_SAMPLE_SIZE);
      log::info("aidl: sample_rate: {}", sample_rate);
      log::info("aidl:  peer_mtu: {}", peer_param.peer_mtu);
      log::info("aidl: codec_bit_rate: {}, MTU bitrate: {}",
                                          codec_based_bit_rate, mtu_based_bit_rate);
      codec_config->encodedAudioBitrate = (codec_based_bit_rate < mtu_based_bit_rate) ?
                                           codec_based_bit_rate:mtu_based_bit_rate;
    } else {
      log::info("aidl: codec_bit_rate: {}", codec_based_bit_rate);
      codec_config->encodedAudioBitrate = codec_based_bit_rate;
    }
  }
  log::info("CodecConfiguration={}", codec_config->toString());
  return true;
}

bool a2dp_get_selected_hal_pcm_config(PcmConfiguration* pcm_config) {
  if (pcm_config == nullptr) return false;
  A2dpCodecConfig* a2dp_codec_configs = bta_av_get_a2dp_current_codec();
  if (a2dp_codec_configs == nullptr) {
    log::warn("failure to get A2DP codec config");
    *pcm_config = BluetoothAudioSinkClientInterface::kInvalidPcmConfiguration;
    return false;
  }

  btav_a2dp_codec_config_t current_codec = a2dp_codec_configs->getCodecConfig();
  pcm_config->sampleRateHz = A2dpCodecToHalSampleRate(current_codec);
  pcm_config->bitsPerSample = A2dpCodecToHalBitsPerSample(current_codec);
  pcm_config->channelMode = A2dpCodecToHalChannelMode(current_codec);
  return (pcm_config->sampleRateHz > 0 && pcm_config->bitsPerSample > 0 &&
          pcm_config->channelMode != ChannelMode::UNKNOWN);
}

// Checking if new bluetooth_audio is supported
bool is_hal_force_disabled() {
  if (!is_configured) {
    btaudio_a2dp_disabled =
        osi_property_get_bool(BLUETOOTH_AUDIO_HAL_PROP_DISABLED, false);
    is_configured = true;
  }
  return btaudio_a2dp_disabled;
}

}  // namespace

bool update_codec_offloading_capabilities(
    const std::vector<btav_a2dp_codec_config_t>& framework_preference,
    bool supports_a2dp_hw_offload_v2) {
  /* Load the provider information if supported by the HAL. */
  provider_info = ::bluetooth::audio::aidl::a2dp::ProviderInfo::GetProviderInfo(
      supports_a2dp_hw_offload_v2);
  return ::bluetooth::audio::aidl::codec::UpdateOffloadingCapabilities(
      framework_preference);
}

// Checking if new bluetooth_audio is enabled
bool is_hal_enabled() { return active_hal_interface != nullptr; }

// Check if new bluetooth_audio is running with offloading encoders
bool is_hal_offloading() {
  if (!is_hal_enabled()) {
    return false;
  }
  return active_hal_interface->GetTransportInstance()->GetSessionType() ==
         SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH;
}

bool is_hal_2_0_offloading_session_unknown() {
    if (!is_hal_enabled()) {
     return false;
   }

  return active_hal_interface->GetTransportInstance()->GetSessionType() ==
         SessionType::UNKNOWN;
}

// Opens the HAL client interface of the specified session type and check
// that is is valid. Returns nullptr if the client interface did not open
// properly.
static BluetoothAudioSinkClientInterface* new_hal_interface(
    SessionType session_type) {
  auto a2dp_transport = new A2dpTransport(session_type);
  auto hal_interface = new BluetoothAudioSinkClientInterface(a2dp_transport);
  if (hal_interface->IsValid()) {
    return hal_interface;
  } else {
    log::error("BluetoothAudio HAL for a2dp is invalid");
    delete a2dp_transport;
    delete hal_interface;
    return nullptr;
  }
}

/// Delete the selected HAL client interface.
static void delete_hal_interface(
    BluetoothAudioSinkClientInterface* hal_interface) {
  if (hal_interface == nullptr) {
    return;
  }
  auto a2dp_transport =
      static_cast<A2dpTransport*>(hal_interface->GetTransportInstance());
  delete a2dp_transport;
  delete hal_interface;
}

// Initialize BluetoothAudio HAL: openProvider
bool init(bluetooth::common::MessageLoopThread* /*message_loop*/) {
  log::info("");

  if (software_hal_interface != nullptr) {
    return true;
  }

  if (is_hal_force_disabled()) {
    log::error("BluetoothAudio HAL is disabled");
    return false;
  }

  if (!BluetoothAudioClientInterface::is_aidl_available()) {
    log::error("BluetoothAudio AIDL implementation does not exist");
    return false;
  }

  software_hal_interface =
      new_hal_interface(SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH);
  if (software_hal_interface == nullptr) {
    return false;
  }

  if (btif_av_is_a2dp_offload_enabled() &&
      offloading_hal_interface == nullptr) {
    offloading_hal_interface =
        new_hal_interface(SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH);
    if (offloading_hal_interface == nullptr) {
      delete_hal_interface(software_hal_interface);
      software_hal_interface = nullptr;
      return false;
    }
  }

  active_hal_interface =
      (offloading_hal_interface != nullptr ? offloading_hal_interface
                                           : software_hal_interface);

  if (remote_delay != 0) {
    log::info("restore DELAY {} ms", static_cast<float>(remote_delay / 10.0));
    static_cast<A2dpTransport*>(active_hal_interface->GetTransportInstance())
        ->SetRemoteDelay(remote_delay);
    remote_delay = 0;
  }
  return true;
}

// Clean up BluetoothAudio HAL
void cleanup() {
  if (!is_hal_enabled()) return;
  end_session();

  auto a2dp_sink = active_hal_interface->GetTransportInstance();
  static_cast<A2dpTransport*>(a2dp_sink)->ResetPendingCmd();
  static_cast<A2dpTransport*>(a2dp_sink)->ResetPresentationPosition();
  active_hal_interface = nullptr;

  a2dp_sink = software_hal_interface->GetTransportInstance();
  delete software_hal_interface;
  software_hal_interface = nullptr;
  delete a2dp_sink;
  if (offloading_hal_interface != nullptr) {
    a2dp_sink = offloading_hal_interface->GetTransportInstance();
    delete offloading_hal_interface;
    offloading_hal_interface = nullptr;
    delete a2dp_sink;
  }

  remote_delay = 0;
}

// Set up the codec into BluetoothAudio HAL
bool setup_codec() {
  if (!is_hal_enabled()) {
    log::error("BluetoothAudio HAL is not enabled");
    return false;
  }

  A2dpCodecConfig* a2dp_config = bta_av_get_a2dp_current_codec();
  if (a2dp_config == nullptr) {
    log::error("the current codec is not configured");
    return false;
  }

  if (provider::supports_codec(a2dp_config->codecIndex())) {
    // The codec is supported in the provider info (AIDL v4).
    // In this case, the codec is offloaded, and the configuration passed
    // as A2dpStreamConfiguration to the UpdateAudioConfig() interface
    // method.
    uint8_t codec_info[AVDT_CODEC_SIZE];
    A2dpStreamConfiguration a2dp_stream_configuration;

    a2dp_config->copyOutOtaCodecConfig(codec_info);
    a2dp_stream_configuration.peerMtu =
        a2dp_get_peer_mtu(a2dp_config->codecIndex(), codec_info);
    a2dp_stream_configuration.codecId =
        provider_info->GetCodec(a2dp_config->codecIndex()).value()->id;

    size_t parameters_start = 0;
    size_t parameters_end = 0;
    switch (a2dp_config->codecIndex()) {
      case BTAV_A2DP_CODEC_INDEX_SOURCE_SBC:
      case BTAV_A2DP_CODEC_INDEX_SOURCE_AAC:
        parameters_start = 3;
        parameters_end = 1 + codec_info[0];
        break;
      default:
        parameters_start = 9;
        parameters_end = 1 + codec_info[0];
        break;
    }

    a2dp_stream_configuration.configuration.insert(
        a2dp_stream_configuration.configuration.end(),
        codec_info + parameters_start, codec_info + parameters_end);

    if (!is_hal_offloading()) {
      log::warn("Switching BluetoothAudio HAL to Hardware");
      end_session();
      active_hal_interface = offloading_hal_interface;
    }

    return active_hal_interface->UpdateAudioConfig(
        AudioConfiguration(a2dp_stream_configuration));
  }

  // Fallback to legacy offloading path.
  CodecConfiguration codec_config{};

  if (!a2dp_get_selected_hal_codec_config(&codec_config)) {
    log::error("Failed to get CodecConfiguration");
    return false;
  }

  bool should_codec_offloading =
      bluetooth::audio::aidl::codec::IsCodecOffloadingEnabled(codec_config);
  if (should_codec_offloading && !is_hal_offloading()) {
    log::warn("Switching BluetoothAudio HAL to Hardware");
    end_session();
    active_hal_interface = offloading_hal_interface;
  } else if (!should_codec_offloading && is_hal_offloading()) {
    log::warn("Switching BluetoothAudio HAL to Software");
    end_session();
    active_hal_interface = software_hal_interface;
  }

  AudioConfiguration audio_config{};
  if (active_hal_interface->GetTransportInstance()->GetSessionType() ==
      SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
    audio_config.set<AudioConfiguration::a2dpConfig>(codec_config);
  } else {
    PcmConfiguration pcm_config{};
    if (!a2dp_get_selected_hal_pcm_config(&pcm_config)) {
      log::error("Failed to get PcmConfiguration");
      return false;
    }
    audio_config.set<AudioConfiguration::pcmConfig>(pcm_config);
  }

  return active_hal_interface->UpdateAudioConfig(audio_config);
}

void start_session() {
  if (!is_hal_enabled()) {
    log::error("BluetoothAudio HAL is not enabled");
    return;
  }
  std::vector<LatencyMode> latency_modes = {LatencyMode::FREE};
  if (is_low_latency_mode_allowed) {
    latency_modes.push_back(LatencyMode::LOW_LATENCY);
  }
  active_hal_interface->SetAllowedLatencyModes(latency_modes);
  active_hal_interface->StartSession();
}

void end_session() {
  if (!is_hal_enabled()) {
    log::error("BluetoothAudio HAL is not enabled");
    return;
  }
  active_hal_interface->EndSession();
  static_cast<A2dpTransport*>(active_hal_interface->GetTransportInstance())
      ->ResetPendingCmd();
  static_cast<A2dpTransport*>(active_hal_interface->GetTransportInstance())
      ->ResetPresentationPosition();
}

void ack_stream_started(const tA2DP_CTRL_ACK& ack) {
  auto ctrl_ack = a2dp_ack_to_bt_audio_ctrl_ack(ack);
  log::info("result={}", ctrl_ack);
  auto a2dp_sink =
      static_cast<A2dpTransport*>(active_hal_interface->GetTransportInstance());
  auto pending_cmd = a2dp_sink->GetPendingCmd();
  if (pending_cmd == A2DP_CTRL_CMD_START) {
    active_hal_interface->StreamStarted(ctrl_ack);
  } else {
    log::warn("pending={} ignore result={}", pending_cmd, ctrl_ack);
    return;
  }
  log::verbose("ctrl_ack={}", ctrl_ack);
  if (ctrl_ack != BluetoothAudioCtrlAck::PENDING) {
    a2dp_sink->ResetPendingCmd();
  }
}

void ack_stream_suspended(const tA2DP_CTRL_ACK& ack) {
  auto ctrl_ack = a2dp_ack_to_bt_audio_ctrl_ack(ack);
  log::info("result={}", ctrl_ack);
  auto a2dp_sink =
      static_cast<A2dpTransport*>(active_hal_interface->GetTransportInstance());
  auto pending_cmd = a2dp_sink->GetPendingCmd();
  if (pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
    active_hal_interface->StreamSuspended(ctrl_ack);
  } else if (pending_cmd == A2DP_CTRL_CMD_STOP) {
    log::info("A2DP_CTRL_CMD_STOP result={}", ctrl_ack);
  } else {
    log::warn("pending={} ignore result={}", pending_cmd, ctrl_ack);
    return;
  }
  log::verbose("ctrl_ack={}", ctrl_ack);
  if (ctrl_ack != BluetoothAudioCtrlAck::PENDING) {
    a2dp_sink->ResetPendingCmd();
  }
}

// Read from the FMQ of BluetoothAudio HAL
size_t read(uint8_t* p_buf, uint32_t len) {
  if (!is_hal_enabled()) {
    log::error("BluetoothAudio HAL is not enabled");
    return 0;
  } else if (is_hal_offloading()) {
    log::error(
        "session_type={} is not A2DP_SOFTWARE_ENCODING_DATAPATH",
        toString(
            active_hal_interface->GetTransportInstance()->GetSessionType()));
    return 0;
  }
  return active_hal_interface->ReadAudioData(p_buf, len);
}

// Update A2DP delay report to BluetoothAudio HAL
void set_remote_delay(uint16_t delay_report) {
  if (!is_hal_enabled()) {
    log::info("not ready for DelayReport {} ms",
              static_cast<float>(delay_report / 10.0));
    remote_delay = delay_report;
    return;
  }
  log::verbose("DELAY {} ms", static_cast<float>(delay_report / 10.0));
  static_cast<A2dpTransport*>(active_hal_interface->GetTransportInstance())
      ->SetRemoteDelay(delay_report);
}

// Set low latency buffer mode allowed or disallowed
void set_low_latency_mode_allowed(bool allowed) {
  LOG(INFO) << __func__;
  is_low_latency_mode_allowed = allowed;
  if (!is_hal_enabled()) {
    log::error("BluetoothAudio HAL is not enabled");
    return;
  }
  std::vector<LatencyMode> latency_modes = {LatencyMode::FREE};
  for (auto latency_mode : latency_modes) {
    LOG(INFO) << __func__ << ": LatencyMode: "
              << ::aidl::android::hardware::bluetooth::audio::toString(latency_mode);
  }
  if (is_low_latency_mode_allowed) {
    latency_modes.push_back(LatencyMode::LOW_LATENCY);
  }
  active_hal_interface->SetAllowedLatencyModes(latency_modes);
}

/***
 * Lookup the codec info in the list of supported offloaded sink codecs.
 ***/
std::optional<btav_a2dp_codec_index_t> provider::sink_codec_index(
    const uint8_t* p_codec_info) {
  return provider_info ? provider_info->SinkCodecIndex(p_codec_info)
                       : std::nullopt;
}

/***
 * Lookup the codec info in the list of supported offloaded source codecs.
 ***/
std::optional<btav_a2dp_codec_index_t> provider::source_codec_index(
    const uint8_t* p_codec_info) {
  return provider_info ? provider_info->SourceCodecIndex(p_codec_info)
                       : std::nullopt;
}

/***
 * Return the name of the codec which is assigned to the input index.
 * The codec index must be in the ranges
 * BTAV_A2DP_CODEC_INDEX_SINK_EXT_MIN..BTAV_A2DP_CODEC_INDEX_SINK_EXT_MAX or
 * BTAV_A2DP_CODEC_INDEX_SOURCE_EXT_MIN..BTAV_A2DP_CODEC_INDEX_SOURCE_EXT_MAX.
 * Returns nullopt if the codec_index is not assigned or codec extensibility
 * is not supported or enabled.
 ***/
std::optional<const char*> provider::codec_index_str(
    btav_a2dp_codec_index_t codec_index) {
  return provider_info ? provider_info->CodecIndexStr(codec_index)
                       : std::nullopt;
}

/***
 * Return true if the codec is supported for the session type
 * A2DP_HARDWARE_ENCODING_DATAPATH or A2DP_HARDWARE_DECODING_DATAPATH.
 ***/
bool provider::supports_codec(btav_a2dp_codec_index_t codec_index) {
  return provider_info ? provider_info->SupportsCodec(codec_index) : false;
}

/***
 * Return the A2DP capabilities for the selected codec.
 ***/
bool provider::codec_info(btav_a2dp_codec_index_t codec_index,
                          uint64_t* codec_id, uint8_t* codec_info,
                          btav_a2dp_codec_config_t* codec_config) {
  return provider_info ? provider_info->CodecCapabilities(
                             codec_index, codec_id, codec_info, codec_config)
                       : false;
}

static btav_a2dp_codec_channel_mode_t convert_channel_mode(
    ChannelMode channel_mode) {
  switch (channel_mode) {
    case ChannelMode::MONO:
      return BTAV_A2DP_CODEC_CHANNEL_MODE_MONO;
    case ChannelMode::STEREO:
      return BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO;
    default:
      log::error("unknown channel mode");
      break;
  }
  return BTAV_A2DP_CODEC_CHANNEL_MODE_NONE;
}

static btav_a2dp_codec_sample_rate_t convert_sampling_frequency_hz(
    int sampling_frequency_hz) {
  switch (sampling_frequency_hz) {
    case 44100:
      return BTAV_A2DP_CODEC_SAMPLE_RATE_44100;
    case 48000:
      return BTAV_A2DP_CODEC_SAMPLE_RATE_48000;
    case 88200:
      return BTAV_A2DP_CODEC_SAMPLE_RATE_88200;
    case 96000:
      return BTAV_A2DP_CODEC_SAMPLE_RATE_96000;
    case 176400:
      return BTAV_A2DP_CODEC_SAMPLE_RATE_176400;
    case 192000:
      return BTAV_A2DP_CODEC_SAMPLE_RATE_192000;
    case 16000:
      return BTAV_A2DP_CODEC_SAMPLE_RATE_16000;
    case 24000:
      return BTAV_A2DP_CODEC_SAMPLE_RATE_24000;
    default:
      log::error("unknown sampling frequency {}", sampling_frequency_hz);
      break;
  }
  return BTAV_A2DP_CODEC_SAMPLE_RATE_NONE;
}

static btav_a2dp_codec_bits_per_sample_t convert_bitdepth(int bitdepth) {
  switch (bitdepth) {
    case 16:
      return BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16;
    case 24:
      return BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24;
    case 32:
      return BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32;
    default:
      log::error("unknown bit depth {}", bitdepth);
      break;
  }
  return BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE;
}

/***
 * Query the codec selection fromt the audio HAL.
 * The HAL is expected to pick the best audio configuration based on the
 * discovered remote SEPs.
 ***/
std::optional<::bluetooth::audio::a2dp::provider::a2dp_configuration>
provider::get_a2dp_configuration(
    RawAddress peer_address,
    std::vector<
        ::bluetooth::audio::a2dp::provider::a2dp_remote_capabilities> const&
        remote_seps,
    btav_a2dp_codec_config_t const& user_preferences) {
  if (provider_info == nullptr) {
    return std::nullopt;
  }

  using ::aidl::android::hardware::bluetooth::audio::A2dpRemoteCapabilities;
  using ::aidl::android::hardware::bluetooth::audio::CodecId;

  // Convert the remote audio capabilities to the exchange format used
  // by the HAL.
  std::vector<A2dpRemoteCapabilities> a2dp_remote_capabilities;
  for (auto const& sep : remote_seps) {
    size_t capabilities_start = 0;
    size_t capabilities_end = 0;
    CodecId id;
    switch (sep.capabilities[2]) {
      case A2DP_MEDIA_CT_SBC:
      case A2DP_MEDIA_CT_AAC: {
        id = CodecId::make<CodecId::a2dp>(
            static_cast<CodecId::A2dp>(sep.capabilities[2]));
        capabilities_start = 3;
        capabilities_end = 1 + sep.capabilities[0];
        break;
      }
      case A2DP_MEDIA_CT_NON_A2DP: {
        uint32_t vendor_id =
            (static_cast<uint32_t>(sep.capabilities[3]) << 0) |
            (static_cast<uint32_t>(sep.capabilities[4]) << 8) |
            (static_cast<uint32_t>(sep.capabilities[5]) << 16) |
            (static_cast<uint32_t>(sep.capabilities[6]) << 24);
        uint16_t codec_id = (static_cast<uint16_t>(sep.capabilities[7]) << 0) |
                            (static_cast<uint16_t>(sep.capabilities[8]) << 8);
        id = CodecId::make<CodecId::vendor>(
            CodecId::Vendor({.id = (int32_t)vendor_id, .codecId = codec_id}));
        capabilities_start = 9;
        capabilities_end = 1 + sep.capabilities[0];
        break;
      }
      default:
        continue;
    }
    A2dpRemoteCapabilities& capabilities =
        a2dp_remote_capabilities.emplace_back();
    capabilities.seid = sep.seid;
    capabilities.id = id;
    capabilities.capabilities.insert(capabilities.capabilities.end(),
                                     sep.capabilities + capabilities_start,
                                     sep.capabilities + capabilities_end);
  }

  // Convert the user preferences into a configuration hint.
  A2dpConfigurationHint hint;
  hint.bdAddr = peer_address.ToArray();
  auto& codecParameters = hint.codecParameters.emplace();
  switch (user_preferences.channel_mode) {
    case BTAV_A2DP_CODEC_CHANNEL_MODE_MONO:
      codecParameters.channelMode = ChannelMode::MONO;
      break;
    case BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO:
      codecParameters.channelMode = ChannelMode::STEREO;
      break;
    default:
      break;
  }
  switch (user_preferences.sample_rate) {
    case BTAV_A2DP_CODEC_SAMPLE_RATE_44100:
      codecParameters.samplingFrequencyHz = 44100;
      break;
    case BTAV_A2DP_CODEC_SAMPLE_RATE_48000:
      codecParameters.samplingFrequencyHz = 48000;
      break;
    case BTAV_A2DP_CODEC_SAMPLE_RATE_88200:
      codecParameters.samplingFrequencyHz = 88200;
      break;
    case BTAV_A2DP_CODEC_SAMPLE_RATE_96000:
      codecParameters.samplingFrequencyHz = 96000;
      break;
    case BTAV_A2DP_CODEC_SAMPLE_RATE_176400:
      codecParameters.samplingFrequencyHz = 176400;
      break;
    case BTAV_A2DP_CODEC_SAMPLE_RATE_192000:
      codecParameters.samplingFrequencyHz = 192000;
      break;
    case BTAV_A2DP_CODEC_SAMPLE_RATE_16000:
      codecParameters.samplingFrequencyHz = 16000;
      break;
    case BTAV_A2DP_CODEC_SAMPLE_RATE_24000:
      codecParameters.samplingFrequencyHz = 24000;
      break;
    default:
      break;
  }
  switch (user_preferences.bits_per_sample) {
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16:
      codecParameters.bitdepth = 16;
      break;
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24:
      codecParameters.bitdepth = 24;
      break;
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32:
      codecParameters.bitdepth = 32;
      break;
    default:
      break;
  }

  log::info("remote capabilities:");
  for (auto const& sep : a2dp_remote_capabilities) {
    log::info("- {}", sep.toString());
  }
  log::info("hint: {}", hint.toString());

  if (offloading_hal_interface == nullptr &&
      (offloading_hal_interface = new_hal_interface(
           SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH)) == nullptr) {
    log::error("the offloading HAL interface cannot be opened");
    return std::nullopt;
  }

  // Invoke the HAL GetAdpCapabilities method with the
  // remote capabilities.
  auto result = offloading_hal_interface->GetA2dpConfiguration(
      a2dp_remote_capabilities, hint);

  // Convert the result configuration back to the stack's format.
  if (!result.has_value()) {
    log::info("provider cannot resolve the a2dp configuration");
    return std::nullopt;
  }

  log::info("provider selected {}", result->toString());

  ::bluetooth::audio::a2dp::provider::a2dp_configuration a2dp_configuration;
  a2dp_configuration.remote_seid = result->remoteSeid;
  a2dp_configuration.vendor_specific_parameters =
      result->parameters.vendorSpecificParameters;
  ProviderInfo::BuildCodecCapabilities(result->id, result->configuration,
                                       a2dp_configuration.codec_config);
  a2dp_configuration.codec_parameters.codec_type =
      provider_info->SourceCodecIndex(result->id).value();
  a2dp_configuration.codec_parameters.channel_mode =
      convert_channel_mode(result->parameters.channelMode);
  a2dp_configuration.codec_parameters.sample_rate =
      convert_sampling_frequency_hz(result->parameters.samplingFrequencyHz);
  a2dp_configuration.codec_parameters.bits_per_sample =
      convert_bitdepth(result->parameters.bitdepth);

  return std::make_optional(a2dp_configuration);
}

/***
 * Query the codec parameters from the audio HAL.
 * The HAL is expected to parse the codec configuration
 * received from the peer and decide whether accept
 * the it or not.
 ***/
tA2DP_STATUS provider::parse_a2dp_configuration(
    btav_a2dp_codec_index_t codec_index, const uint8_t* codec_info,
    btav_a2dp_codec_config_t* codec_parameters,
    std::vector<uint8_t>* vendor_specific_parameters) {
  std::vector<uint8_t> configuration;
  CodecParameters codec_parameters_aidl;

  if (provider_info == nullptr) {
    log::error("provider_info is null");
    return A2DP_FAIL;
  }

  auto codec = provider_info->GetCodec(codec_index);
  if (!codec.has_value()) {
    log::error("codec index not recognized by provider");
    return A2DP_FAIL;
  }

  std::copy(codec_info, codec_info + AVDT_CODEC_SIZE,
            std::back_inserter(configuration));

  auto a2dp_status = offloading_hal_interface->ParseA2dpConfiguration(
      codec.value()->id, configuration, &codec_parameters_aidl);

  if (!a2dp_status.has_value()) {
    log::error("provider failed to parse configuration");
    return A2DP_FAIL;
  }

  if (codec_parameters != nullptr) {
    codec_parameters->channel_mode =
        convert_channel_mode(codec_parameters_aidl.channelMode);
    codec_parameters->sample_rate = convert_sampling_frequency_hz(
        codec_parameters_aidl.samplingFrequencyHz);
    codec_parameters->bits_per_sample =
        convert_bitdepth(codec_parameters_aidl.bitdepth);
  }

  if (vendor_specific_parameters != nullptr) {
    *vendor_specific_parameters =
        codec_parameters_aidl.vendorSpecificParameters;
  }

  return static_cast<tA2DP_STATUS>(a2dp_status.value());
}

}  // namespace a2dp
}  // namespace aidl
}  // namespace audio
}  // namespace bluetooth
