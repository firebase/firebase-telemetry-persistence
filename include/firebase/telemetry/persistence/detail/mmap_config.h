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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_MMAP_CONFIG_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_MMAP_CONFIG_H__

#include <cstddef>
#include <cstdint>

#include "firebase/telemetry/persistence/mmap_size.h"

namespace firebase::telemetry::persistence::detail {

// Configuration for the memory-mapped file structure
struct MmapConfig {
  std::size_t max_spans;
  std::size_t max_attributes;
};

// Configuration for each supported MmapSize
template <MmapSize Size>
constexpr MmapConfig mmap_config = []() {
  switch (Size) {
    case MmapSize::Large:
      return MmapConfig{2048, 128};
    case MmapSize::Medium:
      return MmapConfig{1024, 64};
    case MmapSize::Small:
      return MmapConfig{128, 16};
  }
}();

static_assert(sizeof (MmapSize) % 8 == 0,
              "Size of enum `MmapSize` is not word-aligned");

}  // namespace firebase::telemetry::persistence::detail

#endif  // __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_MMAP_CONFIG_H__
