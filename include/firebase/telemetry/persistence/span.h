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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_SPAN_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_SPAN_H__

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "firebase/telemetry/persistence/detail/copy_string.h"
#include "firebase/telemetry/persistence/detail/marked_span.h"
#include "firebase/telemetry/persistence/mmap_size.h"


namespace firebase::telemetry::persistence {

// This is required for using these in Swift
using AttributesList = std::vector<std::pair<std::string, std::string>>;

// Represents a 128-bit otel trace id, split for performance and interop
struct TraceId {
  // Most significant 64 bits (first 16 hex characters)
  std::uint64_t high;
  // Least significant 64 bits (last 16 hex characters)
  std::uint64_t low;
};

// Represents a Span for passing around and interop with Swift and Kotlin
class Span {
public:
  Span(TraceId trace_id, std::uint64_t span_id, std::uint64_t parent_span_id,
       std::uint64_t start_time, std::uint64_t end_time,
       const std::string& name,
       const std::vector<std::pair<std::string, std::string>>& attributes);

  template <MmapSize Size>
  explicit Span(const detail::RawSpan<Size>& raw_span);

  template <MmapSize Size>
  detail::RawSpan<Size> to_raw_span() const;

  TraceId trace_id() const { return trace_id_; }
  std::uint64_t span_id() const { return span_id_; }
  std::uint64_t parent_span_id() const { return parent_span_id_; }
  std::uint64_t start_time() const { return start_time_; }
  std::uint64_t end_time() const { return end_time_; }

  // These return values instead of references for compatibility with Swift
  std::string name() const { return name_; }
  std::vector<std::pair<std::string, std::string>> attributes() const {
    return attributes_;
  }

private:
  TraceId trace_id_;
  std::uint64_t span_id_;
  std::uint64_t parent_span_id_;
  std::uint64_t start_time_;
  std::uint64_t end_time_;

  std::string name_;
  std::vector<std::pair<std::string, std::string>> attributes_;
};

// Implementation --------------------------------------------------------------

inline Span::Span(
    TraceId trace_id, std::uint64_t span_id, std::uint64_t parent_span_id,
    std::uint64_t start_time, std::uint64_t end_time, const std::string& name,
    const std::vector<std::pair<std::string, std::string>>& attributes)
    : trace_id_(trace_id),
      span_id_(span_id),
      parent_span_id_(parent_span_id),
      start_time_(start_time),
      end_time_(end_time),
      name_(name),
      attributes_(attributes) {
  assert(span_id != 0);  // Valid span ids must have at least one non-zero byte
}

template <MmapSize Size>
Span::Span(const detail::RawSpan<Size>& raw_span)
    : trace_id_({raw_span.trace_id[0], raw_span.trace_id[1]}),
      span_id_(raw_span.span_id),
      parent_span_id_(raw_span.parent_span_id),
      start_time_(raw_span.start_time),
      end_time_(raw_span.end_time),
      name_(raw_span.name) {
  for (std::size_t i = 0; i < raw_span.attr_count; ++i) {
    const detail::RawAttribute& raw_attr = raw_span.attributes[i];
    // Removed attributes might still be in the raw data, so do not copy them
    if (std::string_view attr_value(raw_attr.value); !attr_value.empty()) {
      attributes_.emplace_back(raw_attr.key, attr_value);
    }
  }
}

template <MmapSize Size>
detail::RawSpan<Size> Span::to_raw_span() const {
  detail::RawSpan<Size> raw_span{};
  std::memset(&raw_span, 0, sizeof (raw_span));

  raw_span.trace_id[0] = trace_id_.high;
  raw_span.trace_id[1] = trace_id_.low;
  raw_span.span_id = span_id_;
  raw_span.parent_span_id = parent_span_id_;
  raw_span.start_time = start_time_;
  raw_span.end_time = end_time_;

  detail::copy_string(name_, raw_span.name);

  raw_span.attr_count = 0;
  for (const auto& [key, value] : attributes_) {
    if (raw_span.attr_count >= detail::mmap_config<Size>.max_attributes) {
      break;
    }
    detail::RawAttribute& raw_attr = raw_span.attributes[raw_span.attr_count];
    detail::copy_string2(key, raw_attr.key,
                         value, raw_attr.value);
    ++raw_span.attr_count;
  }

  return raw_span;
}

}  // namespace firebase::telemetry::persistence

#endif  // __FIREBASE_TELEMETRY_PERSISTENCE_SPAN_H__
