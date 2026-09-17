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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_TEST_UTILS_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_TEST_UTILS_H__

#include <sys/mman.h>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <string>
#include <string_view>

#include "firebase/telemetry/persistence/detail/header.h"
#include "firebase/telemetry/persistence/detail/managed_mmap.h"
#include "firebase/telemetry/persistence/detail/memory_layout.h"
#include "firebase/telemetry/persistence/detail/page_size.h"
#include "firebase/telemetry/persistence/detail/span_data_impl.h"
#include "firebase/telemetry/persistence/mmap_size.h"
#include "firebase/telemetry/persistence/span.h"

namespace firebase::telemetry::persistence::detail {

inline ManagedMmap<Header> make_header(MmapSize size) {
  std::size_t page_size = get_page_size();
  std::size_t required_size =
      (get_memory_layout_size(size) + page_size - 1) / page_size * page_size;
  ManagedMmap<Header> header(nullptr, required_size,
                             PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  header->mmap_size = size;
  return header;
}

inline SpanDataImpl make_span_data(MmapSize size) {
  ManagedMmap<Header> header = make_header(size);
  reset_header(header, size);
  return SpanDataImpl(std::move(header));
}

inline std::string get_attribute(const Span& span, std::string_view key) {
  auto attrs = span.attributes();
  auto it = std::find_if(attrs.begin(), attrs.end(),
                         [key](const auto& p) { return p.first == key; });
  return (it != attrs.end()) ? it->second : "";
}

inline Span simple_span(
    std::uint64_t span_id = 1, const std::string& name = "",
    const std::vector<std::pair<std::string, std::string>>& attributes = {},
    std::uint64_t start_time = 0, std::uint64_t end_time = 0) {
  return Span({0, 0}, span_id, 0, start_time, end_time, name, attributes);
}

}  // namespace firebase::telemetry::persistence::detail

#endif  //__FIREBASE_TELEMETRY_PERSISTENCE_TEST_UTILS_H__
