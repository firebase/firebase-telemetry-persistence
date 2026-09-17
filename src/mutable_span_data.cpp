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

#include "firebase/telemetry/persistence/mutable_span_data.h"

#include <chrono>
#include <cstdint>
#include <string>

#include "firebase/telemetry/persistence/detail/span_data_impl.h"

namespace firebase::telemetry::persistence {

MutableSpanData::MutableSpanData(detail::SpanDataImpl& impl) : impl_(impl) {}

std::uint64_t MutableSpanData::count() const {
  return impl_.count();
}

void MutableSpanData::add(const Span& span) {
  if (span_id_to_index_.find(span.span_id()) == span_id_to_index_.end()) {
    if (impl_.count() == impl_.max_count()) {
      // Find the oldest span's span id
      std::uint64_t oldest_span_id = 0;
      detail::mark_t oldest_mark = UINT64_MAX;
      for (std::uint64_t i = 0; i < impl_.count(); ++i) {
        detail::mark_t mark = impl_.get_mark(i);
        if (mark < oldest_mark) {
          oldest_mark = mark;
          oldest_span_id = impl_.get_span_id(i);
        }
      }
      // Remove the oldest span if needed
      if (oldest_span_id != 0) {
        remove(oldest_span_id);
      }
    }

    detail::index_t raw_index = impl_.add(span);
    span_id_to_index_.insert({span.span_id(), raw_index});
  }
}

void MutableSpanData::set_attribute_on_span(std::uint64_t span_id,
                                            const std::string& key,
                                            const std::string& value) {
  if (span_id_to_index_.find(span_id) != span_id_to_index_.end()) {
    detail::index_t raw_index = span_id_to_index_.at(span_id);
    impl_.set_attribute_on_span(raw_index, key, value);
  }
}

void MutableSpanData::remove(std::uint64_t span_id) {
  auto iterator = span_id_to_index_.find(span_id);
  if (iterator != span_id_to_index_.end()) {
    detail::index_t raw_index = iterator->second;

    // Swap the span with the last element, then update its index
    std::uint64_t moved_span_id = impl_.remove(raw_index);
    if (moved_span_id > 0) {
      span_id_to_index_[moved_span_id] = raw_index;
    }

    span_id_to_index_.erase(span_id);
  }
}

void MutableSpanData::end(std::uint64_t span_id, std::uint64_t end_time) {
  auto iterator = span_id_to_index_.find(span_id);
  if (iterator != span_id_to_index_.end()) {
    detail::index_t raw_index = iterator->second;

    if (end_time == 0) {
      end_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch()
      ).count();
    }

    impl_.set_end_time(raw_index, end_time);
  }
}

}  // namespace firebase::telemetry::persistence
