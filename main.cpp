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

#include <iostream>
#include <memory>
#include <vector>

#include "firebase/telemetry/persistence/initialize.h"
#include "firebase/telemetry/persistence/mutable_span_data.h"
#include "firebase/telemetry/persistence/span.h"

using namespace firebase::telemetry::persistence;

int main() {
  Span span{{0, 0}, 271, 0, 0, 0, "", {}};
  Span span2{{0, 0}, 272, 0, 0, 0, "", {}};

  unspecified_context_t* context =
      initialize_span_data("data.mmap", MmapSize::Small);
  std::vector<Span> recovered_spans = get_recovered_spans(context);

  std::cout << "Recovered span count: " << recovered_spans.size() << std::endl;
  for (const Span& recovered_span : recovered_spans) {
    std::cout << "Recovered span id: " << recovered_span.span_id()
              << ", name: " << recovered_span.name() << std::endl;
    for (const auto& [key, val] : recovered_span.attributes()) {
      std::cout << "  attribute -> " << key << ": " << val << std::endl;
    }
  }

  std::shared_ptr<MutableSpanData> mutable_span_data =
      get_mutable_span_data(context);
  if (mutable_span_data == nullptr) {
    std::cerr << "Failed to acquire active MutableSpanData writer" << std::endl;
    return 1;
  }

  std::cout << "Active span count before writing: "
            << mutable_span_data->count() << std::endl;

  mutable_span_data->add(span);
  mutable_span_data->add(span2);

  mutable_span_data->set_attribute_on_span(span2.span_id(), "hello", "world");

  std::cout << "Active span count after writing: " << mutable_span_data->count()
            << std::endl;

  release_span_data(context);
  return 0;
}
