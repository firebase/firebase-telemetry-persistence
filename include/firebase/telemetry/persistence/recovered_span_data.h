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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_SPAN_DATA_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_SPAN_DATA_H__

#include <cstddef>
#include <cstdint>

#include "firebase/telemetry/persistence/detail/types.h"
#include "firebase/telemetry/persistence/span.h"

namespace firebase::telemetry::persistence {

namespace detail {

class SpanDataImpl;

}  // namespace detail

class __attribute__((visibility("default"))) RecoveredSpanData {
public:
  explicit RecoveredSpanData(const detail::SpanDataImpl& impl);

  std::uint64_t count() const;
  std::size_t max_count() const;

  detail::mark_t get_mark(detail::index_t raw_index) const;
  Span get(detail::index_t raw_index) const;

private:
  const detail::SpanDataImpl& impl_;
};

}  // namespace firebase::telemetry::persistence

#endif  // __FIREBASE_TELEMETRY_PERSISTENCE_SPAN_DATA_H__
