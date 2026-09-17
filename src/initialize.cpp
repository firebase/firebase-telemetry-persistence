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

#include "firebase/telemetry/persistence/initialize.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

#include "firebase/telemetry/persistence/detail/header.h"
#include "firebase/telemetry/persistence/detail/memory_layout.h"
#include "firebase/telemetry/persistence/detail/page_size.h"
#include "firebase/telemetry/persistence/detail/span_data_impl.h"
#include "firebase/telemetry/persistence/detail/types.h"
#include "firebase/telemetry/persistence/mutable_span_data.h"
#include "firebase/telemetry/persistence/recovered_span_data.h"

namespace firebase::telemetry::persistence {

namespace {

std::size_t get_aligned_size(MmapSize size) {
  std::size_t page_size = detail::get_page_size();
  std::size_t layout_size = detail::get_memory_layout_size(size);
  return ((layout_size + page_size - 1) / page_size) * page_size;
}

bool should_do_recovery(const detail::ManagedMmap<detail::Header>& header,
                        std::size_t mapped_size) {
  if (!detail::is_header_valid(header) ||
      !detail::is_header_version_current(header)) {
    return false;
  }

  if (mapped_size < detail::get_memory_layout_size(header->mmap_size)) {
    return false;
  }

  return header->active_span_count > 0;
}

}  // namespace

namespace detail {

class Context {
public:
  explicit Context(SpanDataImpl span_data_impl,
                   std::vector<Span> recovered_spans)
      : span_data_impl_(std::move(span_data_impl)),
        recovered_spans_(std::move(recovered_spans)),
        is_recovered_(false),
        mutable_span_data_(std::make_shared<MutableSpanData>(span_data_impl_)) {
    span_data_impl_.remove_all();
  }

  bool is_recovered() const { return is_recovered_; }

  std::shared_ptr<MutableSpanData> mutable_span_data() {
    return mutable_span_data_;
  }

  std::vector<Span> take_recovered_spans() {
    is_recovered_ = true;
    return std::move(recovered_spans_);
  }

private:
  SpanDataImpl span_data_impl_;

  std::vector<Span> recovered_spans_;
  bool is_recovered_;
  std::shared_ptr<MutableSpanData> mutable_span_data_;
};

}  // namespace detail

unspecified_context_t* initialize_span_data(const std::string& path,
                                            MmapSize target_size) {
  int fd = open(path.c_str(), O_RDWR | O_CREAT, 0600);
  if (fd == -1) {
    return nullptr;
  }

  struct stat st;
  if (fstat(fd, &st) == -1) {
    close(fd);
    return nullptr;
  }
  std::size_t original_size = st.st_size;
  std::size_t required_size = get_aligned_size(target_size);

  std::vector<Span> recovered_spans;
  std::optional<detail::SpanDataImpl> span_data_impl;

  if (original_size >= detail::get_page_size()) {
    detail::ManagedMmap<detail::Header> header(
        nullptr, original_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (!header.is_initialized()) {
      close(fd);
      return nullptr;
    }

    if (should_do_recovery(header, original_size)) {
      detail::SpanDataImpl temp_impl(std::move(header));
      RecoveredSpanData recovered_span_data(temp_impl);
      for (detail::index_t i = 0; i < recovered_span_data.count(); ++i) {
        if (recovered_span_data.get_mark(i)) {
          recovered_spans.push_back(recovered_span_data.get(i));
        }
      }

      // If the layout matches the target size, reuse this mapping
      if (temp_impl.size() == target_size && original_size == required_size) {
        span_data_impl.emplace(std::move(temp_impl));
      }
    }
  }

  if (!span_data_impl.has_value()) {
    if (ftruncate(fd, required_size) == -1) {
      close(fd);
      return nullptr;
    }

    detail::ManagedMmap<detail::Header> header(
        nullptr, required_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (!header.is_initialized()) {
      close(fd);
      return nullptr;
    }

    reset_header(header, target_size);
    span_data_impl.emplace(std::move(header));
  }

  close(fd);

  return new detail::Context(std::move(*span_data_impl),
                             std::move(recovered_spans));
}

std::vector<Span> get_recovered_spans(unspecified_context_t* context) {
  if (context == nullptr || context->is_recovered()) {
    return {};
  }

  return context->take_recovered_spans();
}

std::shared_ptr<MutableSpanData> get_mutable_span_data(
    unspecified_context_t* context) {
  if (context == nullptr || !context->is_recovered()) {
    return nullptr;
  }

  return context->mutable_span_data();
}

void release_span_data(unspecified_context_t*& context) {
  if (context != nullptr) {
    delete context;
    // Set the given context to nullptr
    context = nullptr;
  }
}

}  // namespace firebase::telemetry::persistence
