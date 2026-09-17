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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_MUTABLE_SPAN_DATA_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_MUTABLE_SPAN_DATA_H__

#include <cstdint>
#include <string>
#include <unordered_map>

#include "firebase/telemetry/persistence/detail/types.h"
#include "firebase/telemetry/persistence/span.h"

namespace firebase::telemetry::persistence {

namespace detail {

class SpanDataImpl;

}  // namespace detail

class __attribute__((visibility("default"))) MutableSpanData {
public:
  explicit MutableSpanData(detail::SpanDataImpl& impl);

  std::uint64_t count() const;

  void add(const Span& span);
  void remove(std::uint64_t span_id);
  void end(std::uint64_t span_id, std::uint64_t end_time = 0);

  void set_attribute_on_span(std::uint64_t span_id,
                             const std::string& key,
                             const std::string& value);

private:
  detail::SpanDataImpl& impl_;
  std::unordered_map<std::uint64_t, detail::index_t> span_id_to_index_;
};

}  // namespace firebase::telemetry::persistence

#endif  //__FIREBASE_TELEMETRY_PERSISTENCE_MUTABLE_SPAN_DATA_H__
