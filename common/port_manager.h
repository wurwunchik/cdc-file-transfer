/*
 * Copyright 2022 Google LLC
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

#ifndef COMMON_PORT_MANAGER_H_
#define COMMON_PORT_MANAGER_H_

#include <absl/status/statusor.h>

#include <memory>
#include <string>
#include <unordered_set>

#include "common/clock.h"

namespace cdc_ft {

class ProcessFactory;
class RemoteUtil;
class SharedMemory;

// Class for reserving ports globally. Use if there can be multiple processes
// of the same type that might request ports at the same time, e.g. multiple
// cdc_rsync.exe processes running concurrently.
class PortManager {
 public:
  // |unique_name| is a globally unique name used for shared memory to
  // synchronize port reservation. The range of possible ports managed by this
  // instance is [|first_port|, |last_port|]. |process_factory| is a valid
  // pointer to a ProcessFactory instance to run processes locally.
  // |remote_util| is a valid pointer to a RemoteUtil instance to run processes
  // remotely.
  PortManager(std::string unique_name, int first_port, int last_port,
              ProcessFactory* process_factory, RemoteUtil* remote_util,
              SystemClock* system_clock = DefaultSystemClock::GetInstance(),
              SteadyClock* steady_clock = DefaultSteadyClock::GetInstance());
  ~PortManager();

  // Reserves a port in the range passed to the constructor. The port is
  // released automatically upon destruction if ReleasePort() is not called
  // explicitly.
  // |check_remote| determines whether the remote port should be checked as
  // well. If false, the check is skipped and a port might be returned that is
  // still in use remotely.
  // |remote_timeout_sec| is the timeout for finding available ports on the
  // remote instance. Not used if |check_remote| is false.
  // Returns a DeadlineExceeded error if the timeout is exceeded.
  // Returns a ResourceExhausted error if no ports are available.
  absl::StatusOr<int> ReservePort(bool check_remote, int remote_timeout_sec);

  // Releases a reserved port.
  absl::Status ReleasePort(int port);

  //
  // Lower-level interface for finding available ports directly.
  //

  // Finds available ports in the range [first_port, last_port] for port
  // forwarding on the local workstation.
  // |ip| is the IP address to filter by.
  // |process_factory| is used to create a netstat process.
  // |forward_output_to_log| determines whether the stderr of netstat is
  // forwarded to the logs. Returns ResourceExhaustedError if no port is
  // available.
  static absl::StatusOr<std::unordered_set<int>> FindAvailableLocalPorts(
      int first_port, int last_port, const char* ip,
      ProcessFactory* process_factory, bool forward_output_to_log);

  // Finds available ports in the range [first_port, last_port] for port
  // forwarding on the instance.
  // |ip| is the IP address to filter by.
  // |process_factory| is used to create a netstat process.
  // |remote_util| is used to connect to the instance.
  // |timeout_sec| is the connection timeout in seconds.
  // |forward_output_to_log| determines whether the stderr of netstat is
  // forwarded to the logs. Returns a DeadlineExceeded error if the timeout is
  // exceeded. Returns ResourceExhaustedError if no port is available.
  static absl::StatusOr<std::unordered_set<int>> FindAvailableRemotePorts(
      int first_port, int last_port, const char* ip,
      ProcessFactory* process_factory, RemoteUtil* remote_util, int timeout_sec,
      bool forward_output_to_log,
      SteadyClock* steady_clock = DefaultSteadyClock::GetInstance());

 private:
  // Returns a list of available ports in the range [|first_port|, |last_port|]
  // from the given |netstat_output|. |ip| is the IP address to look for, e.g.
  // "127.0.0.1".
  // Returns ResourceExhaustedError if no port is available.
  static absl::StatusOr<std::unordered_set<int>> FindAvailablePorts(
      int first_port, int last_port, const std::string& netstat_output,
      const char* ip);

  int first_port_;
  int last_port_;
  ProcessFactory* process_factory_;
  RemoteUtil* remote_util_;
  SystemClock* system_clock_;
  SteadyClock* steady_clock_;
  std::unique_ptr<SharedMemory> shared_mem_;
  std::unordered_set<int> reserved_ports_;
};

}  // namespace cdc_ft

#endif  // COMMON_PORT_MANAGER_H_
