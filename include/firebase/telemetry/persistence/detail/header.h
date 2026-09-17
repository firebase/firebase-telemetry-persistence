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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_HEADER_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_HEADER_H__

#include <cstddef>

#include "firebase/telemetry/persistence/detail/managed_mmap.h"
#include "firebase/telemetry/persistence/detail/mmap_config.h"

namespace firebase::telemetry::persistence::detail {

// Represents the header in the file-backed mmap data
struct __attribute__((packed)) Header {
  std::uint64_t signature;
  std::uint64_t version;
  MmapSize mmap_size;
  std::uint64_t active_span_count;
};

// Validates if the header is properly set
bool is_header_valid(const ManagedMmap<Header>& header)
    __attribute__((visibility("default")));

// Returns if the header matches the current data structure version
bool is_header_version_current(const ManagedMmap<Header>& header)
    __attribute__((visibility("default")));

// Resets the header to a clean empty state to prepare for first use
void reset_header(ManagedMmap<Header>& header, MmapSize mmap_size)
    __attribute__((visibility("default")));

static_assert(offsetof(Header, active_span_count) % 8 == 0,
              "Member variable `active_span_count` of struct `Header` is not "
              "word-aligned");
static_assert(
    offsetof(Header, mmap_size) % 8 == 0,
    "Member variable `mmap_size` of struct `Header` is not word-aligned");
static_assert(sizeof (Header) % 8 == 0,
              "Size of struct `Header` is not word-aligned");

}  // namespace firebase::telemetry::persistence::detail

#endif  // __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_HEADER_H__
