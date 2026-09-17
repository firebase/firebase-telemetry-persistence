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

#include <cassert>
#include <cstdint>
#include <string_view>
#include <utility>
#include <variant>

#include "firebase/telemetry/persistence/detail/span_data_impl.h"
#include "firebase/telemetry/persistence/detail/header.h"

namespace firebase::telemetry::persistence::detail {

SpanDataImpl::SpanDataImpl(ManagedMmap<Header> header)
    : data_(make_data(std::move(header))) {}

std::uint64_t SpanDataImpl::count() const {
  return std::visit([](auto& d) { return d.count(); }, data_);
}

std::size_t SpanDataImpl::max_count() const {
  return std::visit([](auto& d) { return d.max_count(); }, data_);
}

MmapSize SpanDataImpl::size() const {
  return std::visit([](auto& d) { return d.size(); }, data_);
}

index_t SpanDataImpl::add(const Span& span) {
  return std::visit([&span](auto& d) { return d.add(span); }, data_);
}

index_t SpanDataImpl::set_attribute_on_span(index_t raw_index,
                                            std::string_view key,
                                            std::string_view value) {
  return std::visit(
      [raw_index, key, value](auto& d) {
        return d.set_attribute_on_span(raw_index, key, value);
      },
      data_);
}

void SpanDataImpl::set_end_time(index_t raw_index, std::uint64_t end_time) {
  std::visit(
      [raw_index, end_time](auto& d) { d.set_end_time(raw_index, end_time); },
      data_);
}

std::uint64_t SpanDataImpl::get_span_id(index_t raw_index) const {
  return std::visit(
      [raw_index](auto& d) { return d.get_raw_span(raw_index).span_id; },
      data_);
}

mark_t SpanDataImpl::get_mark(index_t raw_index) const {
  return std::visit([raw_index](auto& d) { return d.get_mark(raw_index); },
                    data_);
}

Span SpanDataImpl::get(index_t raw_index) const {
  return std::visit([raw_index](auto& d) { return d.get(raw_index); }, data_);
}

std::uint64_t SpanDataImpl::remove(index_t raw_index) {
  return std::visit([raw_index](auto& d) { return d.remove(raw_index); },
                    data_);
}

void SpanDataImpl::remove_all() {
  std::visit([](auto& d) { d.remove_all(); }, data_);
}

SpanDataImpl::DataVariant SpanDataImpl::make_data(ManagedMmap<Header> header) {
  assert(header.is_initialized());
  switch (header->mmap_size) {
    case MmapSize::Large:
      return RawSpanData<MmapSize::Large>(std::move(header));
    case MmapSize::Medium:
      return RawSpanData<MmapSize::Medium>(std::move(header));
    case MmapSize::Small:
    default:
      return RawSpanData<MmapSize::Small>(std::move(header));
  }
}

}  // namespace firebase::telemetry::persistence::detail
