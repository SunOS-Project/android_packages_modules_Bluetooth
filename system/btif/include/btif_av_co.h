/******************************************************************************
 *
 *  Copyright 2009-2012 Broadcom Corporation
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

#ifndef BTIF_AV_CO_H
#define BTIF_AV_CO_H

#include "btif/include/btif_a2dp_source.h"
#include "stack/include/a2dp_codec_api.h"
#include "types/raw_address.h"

// Sets the active peer to |peer_addr|.
// Returns true on success, otherwise false.
bool bta_av_co_set_active_peer(const RawAddress& peer_addr);

/**
 * Sets the active peer within the sink profile of the bta av co instance.
 * @param peer_address peer address of the remote device.
 * @return true on success, otherwise false.
 */
bool bta_av_co_set_active_sink_peer(const RawAddress& peer_address);

/**
 * Sets the active peer within the sink profile of the bta av co instance.
 * @param peer_address peer address of the remote device.
 * @return true on success, otherwise false.
 */
bool bta_av_co_set_active_source_peer(const RawAddress& peer_address);

void bta_av_co_save_codec(const uint8_t* new_codec_config);

// Gets the A2DP peer parameters that are used to initialize the encoder.
// The peer address is |peer_addr|.
// The parameters are stored in |p_peer_params|.
// |p_peer_params| cannot be null.
void bta_av_co_get_peer_params(const RawAddress& peer_addr,
                               tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params);

// Gets the current A2DP encoder interface that can be used to encode and
// prepare A2DP packets for transmission - see |tA2DP_ENCODER_INTERFACE|.
// Returns the A2DP encoder interface if the current codec is setup,
// otherwise NULL.
const tA2DP_ENCODER_INTERFACE* bta_av_co_get_encoder_interface(void);

// Sets the user preferred codec configuration.
// The peer address is |peer_addr|.
// |codec_user_config| contains the preferred codec configuration.
// |restart_output| is used to know whether AV is reconfiguring with remote.
// Returns true on success, otherwise false.
bool bta_av_co_set_codec_user_config(
    const RawAddress& peer_addr,
    const btav_a2dp_codec_config_t& codec_user_config, bool* p_restart_output);

// Sets the Audio HAL selected audio feeding parameters.
// Those parameters are applied only to the currently selected codec.
// |codec_audio_config| contains the selected audio feeding configuration.
// Returns true on success, otherwise false.
bool bta_av_co_set_codec_audio_config(
    const btav_a2dp_codec_config_t& codec_audio_config);

// Initializes the control block.
// |codec_priorities| contains the A2DP Source codec priorities to use.
// |supported_codecs| returns the list of supported A2DP Source codecs.
void bta_av_co_init(
    const std::vector<btav_a2dp_codec_config_t>& codec_priorities,
    std::vector<btav_a2dp_codec_info_t>* supported_codecs);

// Checks whether the codec for |codec_index| is supported.
// Returns true if the codec is supported, otherwise false.
bool bta_av_co_is_supported_codec(btav_a2dp_codec_index_t codec_index);

// Gets the current A2DP codec for the active peer.
// Returns a pointer to the current |A2dpCodecConfig| if valid, otherwise
// nullptr.
A2dpCodecConfig* bta_av_get_a2dp_current_codec(void);

// Gets the current A2DP codec for a peer identified by |peer_address|.
// Returns a pointer to the current |A2dpCodecConfig| if valid, otherwise
// nullptr.
A2dpCodecConfig* bta_av_get_a2dp_peer_current_codec(
    const RawAddress& peer_address);

// Gets the A2DP effective frame size from the current encoder.
// Returns the effective frame size if the encoder is configured, otherwise 0.
int bta_av_co_get_encoder_effective_frame_size();

// Dump A2DP codec debug-related information for the A2DP module.
// |fd| is the file descriptor to use for writing the ASCII formatted
// information.
void btif_a2dp_codec_debug_dump(int fd);

// Compares the given BD address family against the interop databse
// and return if AAC can be selected as a codec for streaming or not
//TRUE if AAC is allowed , FALSE otherwise
bool bta_av_co_audio_is_aac_wl_enabled(const RawAddress *remote_bdaddr);

// Set the Offload Support or Aptx Adaptive
// offloading_preference is the MM supported offload preferences
void bta_av_co_aptx_ad_support(std::vector<btav_a2dp_codec_config_t> offloading_preference);


#endif  // BTIF_AV_CO_H
