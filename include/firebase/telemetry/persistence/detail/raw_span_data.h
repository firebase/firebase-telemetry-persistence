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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_RAW_SPAN_DATA_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_RAW_SPAN_DATA_H__

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "firebase/telemetry/persistence/detail/copy_string.h"
#include "firebase/telemetry/persistence/detail/managed_mmap.h"
#include "firebase/telemetry/persistence/detail/memory_layout.h"
#include "firebase/telemetry/persistence/detail/mmap_config.h"
#include "firebase/telemetry/persistence/detail/types.h"
#include "firebase/telemetry/persistence/span.h"

namespace firebase::telemetry::persistence::detail {

struct Header;

// Represents the raw mmap backed span data
template <MmapSize Size>
class RawSpanData {
public:
  explicit RawSpanData(ManagedMmap<MemoryLayout<Size>> layout);

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

  // Gets the mark at the raw index
  mark_t get_mark(index_t raw_index) const;

  // Gets the raw span from the raw index
  const RawSpan<Size>& get_raw_span(index_t raw_index) const;

  // Copies and gets the span from the raw_index
  Span get(index_t raw_index) const;

  // Clears the marked span at the raw index by moving the last active span
  // into this slot. Returns the moved span id, or 0 if no span was moved
  std::uint64_t remove(index_t raw_index);

  // Clears all the marked spans
  void remove_all();

private:
  ManagedMmap<MemoryLayout<Size>> layout_;
  std::uint64_t mark_counter_ = 0;
};

// Implementation --------------------------------------------------------------

template <MmapSize Size>
RawSpanData<Size>::RawSpanData(ManagedMmap<MemoryLayout<Size>> layout)
    : layout_(std::move(layout)) {
  assert(layout_.is_initialized());
}

template <MmapSize Size>
std::uint64_t RawSpanData<Size>::count() const {
  return layout_->header.active_span_count;
}

template <MmapSize Size>
std::size_t RawSpanData<Size>::max_count() const {
  return mmap_config<Size>.max_spans;
}

template <MmapSize Size>
MmapSize RawSpanData<Size>::size() const {
  assert(layout_->header.mmap_size == Size);
  return Size;
}

template <MmapSize Size>
index_t RawSpanData<Size>::add(const Span& span) {
  index_t raw_index = count();
  assert(raw_index < max_count());
  assert(layout_->marked_spans[raw_index].mark == 0);

  // If a crash occurs here, this span will be lost in the worst case. A
  // recovered span will never be inconsistent because the mark is written only
  // after the entire raw span and the recovery process will skip unmarked spans
  mark_counter_ += 1;
  layout_->marked_spans[raw_index].mark = 0;
  layout_->marked_spans[raw_index].span = span.to_raw_span<Size>();
  layout_->marked_spans[raw_index].mark = mark_counter_;
  layout_->header.active_span_count += 1;

  return raw_index;
}

template <MmapSize Size>
index_t RawSpanData<Size>::set_attribute_on_span(index_t raw_index,
                                                 std::string_view key,
                                                 std::string_view value) {
  RawSpan<Size>& raw_span = layout_->marked_spans[raw_index].span;

  // If a crash occurs here, this value will be lost in the worst case. An
  // attribute will never be inconsistent because the string is zeroed before
  // copying
  for (index_t i = 0; i < raw_span.attr_count; ++i) {
    if (key_equals(key, raw_span.attributes[i].key)) {
      copy_string(value, raw_span.attributes[i].value);
      return i;
    }
  }

  // If a crash occurs here, this attribute will be lost in the worst case. An
  // attribute will never be inconsistent because both strings are zeroed before
  // copying the key and value. If the value is not copied, the attribute will
  // be ignored due to being empty when recovering from a crash
  if (raw_span.attr_count < mmap_config<Size>.max_attributes) {
    index_t i = raw_span.attr_count;
    copy_string2(key, raw_span.attributes[i].key,
                 value, raw_span.attributes[i].value);
    raw_span.attr_count += 1;
    return i;
  }

  return mmap_config<Size>.max_attributes;
}

template <MmapSize Size>
void RawSpanData<Size>::set_end_time(index_t raw_index,
                                     std::uint64_t end_time) {
  assert(raw_index < count());
  assert(layout_->marked_spans[raw_index].mark > 0);
  layout_->marked_spans[raw_index].span.end_time = end_time;
}

template <MmapSize Size>
std::uint64_t RawSpanData<Size>::get_mark(index_t raw_index) const {
  return layout_->marked_spans[raw_index].mark;
}

template <MmapSize Size>
const RawSpan<Size>& RawSpanData<Size>::get_raw_span(index_t raw_index) const {
  assert(layout_->marked_spans[raw_index].mark > 0);
  return layout_->marked_spans[raw_index].span;
}

template <MmapSize Size>
Span RawSpanData<Size>::get(index_t raw_index) const {
  assert(layout_->marked_spans[raw_index].mark > 0);
  return static_cast<Span>(layout_->marked_spans[raw_index].span);
}

template <MmapSize Size>
std::uint64_t RawSpanData<Size>::remove(index_t raw_index) {
  assert(raw_index < max_count());
  assert(layout_->marked_spans[raw_index].mark > 0);
  assert(raw_index < layout_->header.active_span_count);

  std::uint64_t last_index = layout_->header.active_span_count - 1;
  assert(layout_->marked_spans[last_index].mark > 0);
  if (raw_index == last_index) {
    layout_->marked_spans[raw_index].mark = 0;
    layout_->header.active_span_count -= 1;
    return 0;
  }

  // This is safe if a crash occurs anywhere in here. A recovered span will
  // never be lost or inconsistent because the raw index's mark is cleared
  // before the swap, and the swapped mark is written after the entire raw span
  std::uint64_t last_span_id = layout_->marked_spans[last_index].span.span_id;
  std::uint64_t last_mark = layout_->marked_spans[last_index].mark;
  layout_->marked_spans[raw_index].mark = 0;

  layout_->marked_spans[raw_index].span =
      layout_->marked_spans[last_index].span;

  // If a crash occurs here, the last span will be duplicated in the worst case
  // because the stale copy and swapped copy might both be marked
  layout_->marked_spans[raw_index].mark = last_mark;
  layout_->marked_spans[last_index].mark = 0;

  layout_->header.active_span_count -= 1;

  return last_span_id;
}

template <MmapSize Size>
void RawSpanData<Size>::remove_all() {
  layout_->header.active_span_count = 0;

  std::memset(layout_->marked_spans, 0, sizeof (layout_->marked_spans));
}

}  // namespace firebase::telemetry::persistence::detail

#endif  // __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_RAW_SPAN_DATA_H__
