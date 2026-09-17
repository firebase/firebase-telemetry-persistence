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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_MEMORY_LAYOUT_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_MEMORY_LAYOUT_H__

#include <type_traits>

#include "firebase/telemetry/persistence/detail/header.h"
#include "firebase/telemetry/persistence/detail/marked_span.h"
#include "firebase/telemetry/persistence/detail/mmap_config.h"

namespace firebase::telemetry::persistence::detail {

// MemoryLayout defines the full binary structure of the mmap file
template <MmapSize Size>
struct __attribute__((packed)) MemoryLayout {
  Header header;
  MarkedSpan<Size> marked_spans[mmap_config<Size>.max_spans];
};

inline std::size_t get_memory_layout_size(MmapSize mmap_size) {
  switch (mmap_size) {
    case MmapSize::Large:
      return sizeof (MemoryLayout<MmapSize::Large>);
    case MmapSize::Medium:
      return sizeof (MemoryLayout<MmapSize::Medium>);
    case MmapSize::Small:
    default:
      return sizeof (MemoryLayout<MmapSize::Small>);
  }
}

// The header is not affected by the template, so we only need to verify once
static_assert(
    offsetof(MemoryLayout<MmapSize::Small>, marked_spans) % 8 == 0,
    "Array `marked_spans` of struct `MemoryLayout` is not word-aligned");
static_assert(std::is_standard_layout_v<MemoryLayout<MmapSize::Small>>,
              "Struct `MemoryLayout` must have standard layout for mmap");
static_assert(std::is_trivially_copyable_v<MemoryLayout<MmapSize::Small>>,
              "Struct `MemoryLayout` must be trivially copyable");

static_assert(sizeof (MemoryLayout<MmapSize::Small>) % 8 == 0,
              "Size of struct `MemoryLayout<Small>` is not word-aligned");
static_assert(sizeof (MemoryLayout<MmapSize::Medium>) % 8 == 0,
              "Size of struct `MemoryLayout<Medium>` is not word-aligned");
static_assert(sizeof (MemoryLayout<MmapSize::Large>) % 8 == 0,
              "Size of struct `MemoryLayout<Large>` is not word-aligned");

}  // namespace firebase::telemetry::persistence::detail

#endif  // __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_MEMORY_LAYOUT_H__
