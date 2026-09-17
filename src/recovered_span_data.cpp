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

#include <cstddef>
#include <cstdint>

#include "firebase/telemetry/persistence/detail/span_data_impl.h"
#include "firebase/telemetry/persistence/recovered_span_data.h"

namespace firebase::telemetry::persistence {

RecoveredSpanData::RecoveredSpanData(const detail::SpanDataImpl& impl)
    : impl_(impl) {}

std::uint64_t RecoveredSpanData::count() const {
  return impl_.count();
}

std::size_t RecoveredSpanData::max_count() const {
  return impl_.max_count();
}

detail::mark_t RecoveredSpanData::get_mark(detail::index_t raw_index) const {
  return impl_.get_mark(raw_index);
}

Span RecoveredSpanData::get(detail::index_t raw_index) const {
  return impl_.get(raw_index);
}

}  // namespace firebase::telemetry::persistence
