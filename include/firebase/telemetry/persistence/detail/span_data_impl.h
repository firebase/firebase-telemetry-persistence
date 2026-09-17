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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_SPAN_DATA_IMPL_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_SPAN_DATA_IMPL_H__

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <variant>

#include "firebase/telemetry/persistence/detail/managed_mmap.h"
#include "firebase/telemetry/persistence/detail/raw_span_data.h"
#include "firebase/telemetry/persistence/mmap_size.h"
#include "firebase/telemetry/persistence/span.h"

namespace firebase::telemetry::persistence::detail {

// Represents the file-backed span data
class SpanDataImpl {
public:
  explicit SpanDataImpl(ManagedMmap<Header> header);

  // Gets the number of active spans
  std::uint64_t count() const;

  // Gets the maximum number of active spans supported
  std::size_t max_count() const;

  // Gets the raw mmap size of the memory layout
  MmapSize size() const;

  // Adds the span and returns its raw index
  index_t add(const Span& span);

  // Set an attribute on the raw span at the given raw index, returns attr index
  // This does a linear scan. Local testing shows it's faster for under 42 attrs
  // Returns `max_attributes` if the attribute could not be set
  index_t set_attribute_on_span(index_t raw_index,
                                std::string_view key,
                                std::string_view value);

  // Set the end time on the raw span at the given raw index
  void set_end_time(index_t raw_index, std::uint64_t end_time);

  // Gets the span id at the raw index
  std::uint64_t get_span_id(index_t raw_index) const;

  // Gets the mark at the raw index
  mark_t get_mark(index_t raw_index) const;

  // Copies and gets the span from the raw_index
  Span get(index_t raw_index) const;

  // Clears the marked span at the raw index by moving the last active span
  // into this slot. Returns the moved span id, or 0 if no span was moved
  std::uint64_t remove(index_t raw_index);

  // Clears all the marked spans
  void remove_all();

private:
  using DataVariant = std::variant<RawSpanData<MmapSize::Small>,
                                   RawSpanData<MmapSize::Medium>,
                                   RawSpanData<MmapSize::Large>>;

  static DataVariant make_data(ManagedMmap<Header> header);

  DataVariant data_;
};

}  // namespace firebase::telemetry::persistence::detail

#endif  //__FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_SPAN_DATA_IMPL_H__
