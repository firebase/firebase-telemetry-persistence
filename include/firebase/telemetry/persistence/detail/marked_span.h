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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_MARKED_SPAN_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_MARKED_SPAN_H__

#include <cstdint>

#include "firebase/telemetry/persistence/detail/mmap_config.h"

namespace firebase::telemetry::persistence::detail {

// The raw attribute key value pair present in each span
struct __attribute__((packed)) RawAttribute {
  char key[64];
  char value[128];
};

// Represents a raw span with N attributes
template <MmapSize Size>
struct __attribute__((packed)) RawSpan {
  std::uint64_t trace_id[2];
  std::uint64_t span_id;
  std::uint64_t parent_span_id;
  std::uint64_t start_time;
  std::uint64_t end_time;
  char name[64];
  std::uint64_t attr_count;

  RawAttribute attributes[mmap_config<Size>.max_attributes];
};

// Represents a marked span, the raw data unit in file-backed mmap data
template <MmapSize Size>
struct __attribute__((packed)) MarkedSpan {
  std::uint64_t mark;
  RawSpan<Size> span;
};

static_assert(sizeof (RawAttribute) % 8 == 0,
              "Size of struct `Attribute` is not word-aligned");
static_assert(sizeof (RawSpan<MmapSize::Small>) % 8 == 0,
              "Size of struct `RawSpan` is not word-aligned");
static_assert(
    offsetof(RawSpan<MmapSize::Small>, attr_count) % 8 == 0,
    "Member variable `attr_count` of struct `RawSpan` is not word-aligned");
static_assert(
    offsetof(MarkedSpan<MmapSize::Small>, span) % 8 == 0,
    "Member variable `span` of struct `MarkedSpan` is not word-aligned");
static_assert(sizeof (MarkedSpan<MmapSize::Small>) % 8 == 0,
              "Size of struct `MarkedSpan` is not word-aligned");

}  // namespace firebase::telemetry::persistence::detail

#endif  // __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_MARKED_SPAN_H__
