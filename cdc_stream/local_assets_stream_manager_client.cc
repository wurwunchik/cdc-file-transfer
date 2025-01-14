// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cdc_stream/local_assets_stream_manager_client.h"

#include "absl/status/status.h"
#include "common/grpc_status.h"

namespace cdc_ft {

using StartSessionRequest = localassetsstreammanager::StartSessionRequest;
using StartSessionResponse = localassetsstreammanager::StartSessionResponse;
using StopSessionRequest = localassetsstreammanager::StopSessionRequest;
using StopSessionResponse = localassetsstreammanager::StopSessionResponse;

LocalAssetsStreamManagerClient::LocalAssetsStreamManagerClient(
    std::shared_ptr<grpc::Channel> channel) {
  stub_ = LocalAssetsStreamManager::NewStub(std::move(channel));
}

LocalAssetsStreamManagerClient::~LocalAssetsStreamManagerClient() = default;

absl::Status LocalAssetsStreamManagerClient::StartSession(
    const std::string& src_dir, const std::string& user_host, uint16_t ssh_port,
    const std::string& mount_dir, const std::string& ssh_command,
    const std::string& scp_command) {
  StartSessionRequest request;
  request.set_workstation_directory(src_dir);
  request.set_user_host(user_host);
  request.set_port(ssh_port);
  request.set_mount_dir(mount_dir);
  request.set_ssh_command(ssh_command);
  request.set_scp_command(scp_command);

  grpc::ClientContext context;
  StartSessionResponse response;
  return ToAbslStatus(stub_->StartSession(&context, request, &response));
}

absl::Status LocalAssetsStreamManagerClient::StopSession(
    const std::string& user_host, const std::string& mount_dir) {
  StopSessionRequest request;
  request.set_user_host(user_host);
  request.set_mount_dir(mount_dir);

  grpc::ClientContext context;
  StopSessionResponse response;
  return ToAbslStatus(stub_->StopSession(&context, request, &response));
}

}  // namespace cdc_ft
