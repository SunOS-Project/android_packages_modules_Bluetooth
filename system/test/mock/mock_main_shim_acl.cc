/*
 * Copyright 2021 The Android Open Source Project
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

#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <string>

#include "hci/acl_manager.h"
#include "hci/acl_manager/classic_acl_connection.h"
#include "hci/acl_manager/le_acl_connection.h"
#include "hci/address.h"
#include "hci/address_with_type.h"
#include "hci/class_of_device.h"
#include "main/shim/acl.h"
#include "main/shim/entry.h"
#include "os/handler.h"
#include "stack/acl/acl.h"
#include "test/common/mock_functions.h"
#include "types/raw_address.h"

using namespace bluetooth;

void DumpsysL2cap(int /* fd */) { inc_func_call_count(__func__); }

void DumpsysAcl(int /* fd */) { inc_func_call_count(__func__); }

void DumpsysBtm(int /* fd */) { inc_func_call_count(__func__); }

void DumpsysRecord(int /* fd */) { inc_func_call_count(__func__); }

void DumpsysNeighbor(int /* fd */) { inc_func_call_count(__func__); }

void shim::legacy::Acl::Dump(int /* fd */) const {
  inc_func_call_count(__func__);
}

shim::legacy::Acl::Acl(os::Handler* /* handler */,
                       const acl_interface_t& acl_interface,
                       uint8_t /* max_acceptlist_size */,
                       uint8_t /* max_address_resolution_size */)
    : acl_interface_(acl_interface) {
  inc_func_call_count(__func__);
}
shim::legacy::Acl::~Acl() { inc_func_call_count(__func__); }

bool shim::legacy::Acl::CheckForOrphanedAclConnections() const {
  inc_func_call_count(__func__);
  return false;
}

void shim::legacy::Acl::on_incoming_acl_credits(uint16_t /* handle */,
                                                uint16_t /* credits */) {
  inc_func_call_count(__func__);
}

using HciHandle = uint16_t;

struct shim::legacy::Acl::impl {};

void shim::legacy::Acl::CreateClassicConnection(
    const hci::Address& /* address */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::CancelClassicConnection(
    const hci::Address& /* address */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::AcceptLeConnectionFrom(
    const hci::AddressWithType& /* address_with_type */, bool /* is_direct */,
    std::promise<bool> /* promise */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::IgnoreLeConnectionFrom(
    const hci::AddressWithType& /* address_with_type */) {
  inc_func_call_count(__func__);
}

void bluetooth::shim::legacy::Acl::OnClassicLinkDisconnected(
    HciHandle /* handle */, hci::ErrorCode /* reason */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::GetConnectionLocalAddress(
    uint16_t /* handle */, bool /* ota_address */,
    std::promise<bluetooth::hci::AddressWithType> /* promise */) {
  inc_func_call_count(__func__);
}
void shim::legacy::Acl::GetConnectionPeerAddress(
    uint16_t /* handle */, bool /* ota_address */,
    std::promise<bluetooth::hci::AddressWithType> /* promise */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::GetAdvertisingSetConnectedTo(
    const RawAddress& /* remote_bda */,
    std::promise<std::optional<uint8_t>> /* promise */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::OnLeLinkDisconnected(HciHandle /* handle */,
                                             hci::ErrorCode /* reason */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::OnConnectSuccess(
    std::unique_ptr<hci::acl_manager::ClassicAclConnection> /* connection */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::OnConnectRequest(hci::Address /* address */,
                                         hci::ClassOfDevice /* cod */) {
  inc_func_call_count(__func__);
}
void shim::legacy::Acl::OnConnectFail(hci::Address /* address */,
                                      hci::ErrorCode /* reason */,
                                      bool /* locally_initiated */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::OnLeConnectSuccess(
    hci::AddressWithType /* address_with_type */,
    std::unique_ptr<hci::acl_manager::LeAclConnection> /* connection */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::OnLeConnectFail(
    hci::AddressWithType /* address_with_type */, hci::ErrorCode /* reason */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::DisconnectClassic(uint16_t /* handle */,
                                          tHCI_STATUS /* reason */,
                                          std::string /* comment */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::DisconnectLe(uint16_t /* handle */,
                                     tHCI_STATUS /* reason */,
                                     std::string /* comment */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::LeSetDefaultSubrate(uint16_t /* subrate_min */,
                                            uint16_t /* subrate_max */,
                                            uint16_t /* max_latency */,
                                            uint16_t /* cont_num */,
                                            uint16_t /* sup_tout */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::LeSubrateRequest(uint16_t /* hci_handle */,
                                         uint16_t /* subrate_min */,
                                         uint16_t /* subrate_max */,
                                         uint16_t /* max_latency */,
                                         uint16_t /* cont_num */,
                                         uint16_t /* sup_tout */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::DumpConnectionHistory(int /* fd */) const {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::DisconnectAllForSuspend() {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::Shutdown() { inc_func_call_count(__func__); }

void shim::legacy::Acl::FinalShutdown() { inc_func_call_count(__func__); }

void shim::legacy::Acl::ClearFilterAcceptList() {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::AddToAddressResolution(
    const hci::AddressWithType& /* address_with_type */,
    const std::array<uint8_t, 16>& /* peer_irk */,
    const std::array<uint8_t, 16>& /* local_irk */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::RemoveFromAddressResolution(
    const hci::AddressWithType& /* address_with_type */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::ClearAddressResolution() {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::SetSystemSuspendState(bool /* suspended */) {
  inc_func_call_count(__func__);
}

void shim::legacy::Acl::UpdateConnectionParameters(uint16_t /* handle */,
                                                   uint16_t /* conn_int_min */,
                                                   uint16_t /* conn_int_max */,
                                                   uint16_t /* conn_latency */,
                                                   uint16_t /* conn_timeout */,
                                                   uint16_t /* min_ce_len */,
                                                   uint16_t /* max_ce_len */) {
  inc_func_call_count(__func__);
}
