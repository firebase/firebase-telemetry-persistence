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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_ENTRY_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_ENTRY_H__

#include <cstddef>
#include <cstdint>

namespace firebase::telemetry::persistence::detail {

// Represents a single entry in the file-backed mmap data
struct __attribute__((packed)) Entry {
  enum class Tag : std::uint32_t {
    Empty = 0,
    Span = 1,
    Log = 2
  };

  Tag tag;
  std::uint32_t size;
};

static_assert(sizeof (Entry) == 8,
              "Size of struct `Entry` is not 8 bytes");

inline bool is_tag_valid(Entry::Tag tag) {
  // Tag::Log is the largest valid enum value
  return tag <= Entry::Tag::Log;
}

// Returns a view of the entry at `data`, which must point into a writable mmap
// with at least sizeof (Entry) bytes remaining. An unknown tag is healed to
// Empty so the entry is skipped on recovery, and the file matches what was read
inline Entry* read_entry(std::byte* data) {
  if (data == nullptr) {
    return nullptr;
  }
  Entry* entry = reinterpret_cast<Entry *>(data);
  if (!is_tag_valid(entry->tag)) {
    entry->tag = Entry::Tag::Empty;
  }
  return entry;
}

}  // namespace firebase::telemetry::persistence::detail

#endif  // __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_ENTRY_H__
