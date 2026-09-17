// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "firebase/telemetry/persistence/detail/header.h"

#include <cstdint>

#include "firebase/telemetry/persistence/detail/managed_mmap.h"

namespace firebase::telemetry::persistence::detail {

// The most recent version of the header
extern const std::uint64_t header_version = 2;

// A signature to identify a valid header
extern const std::uint64_t header_signature = 0x48535243ULL;

namespace {

bool is_supported_size(MmapSize mmap_size) {
  return mmap_size == MmapSize::Small
      || mmap_size == MmapSize::Medium
      || mmap_size == MmapSize::Large;
}

}

bool is_header_valid(const ManagedMmap<Header>& header) {
  return header.is_initialized()
      && header->signature == header_signature
      && is_supported_size(header->mmap_size);
}

bool is_header_version_current(const ManagedMmap<Header>& header) {
  return header.is_initialized()
      && header->version == header_version;
}

void reset_header(ManagedMmap<Header>& header, MmapSize mmap_size) {
  if (header.is_initialized()) {
    header->active_span_count = 0;
    header->mmap_size = mmap_size;
    header->version = header_version;
    header->signature = header_signature;
  }
}

}  // namespace firebase::telemetry::persistence::detail
