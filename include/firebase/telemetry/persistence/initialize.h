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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_INITIALIZE_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_INITIALIZE_H__

#include <memory>
#include <string>
#include <vector>

#include "firebase/telemetry/persistence/mmap_size.h"
#include "firebase/telemetry/persistence/mutable_span_data.h"
#include "firebase/telemetry/persistence/span.h"

namespace firebase::telemetry::persistence {

namespace detail {

class Context;

}  // namespace detail

typedef detail::Context unspecified_context_t;

// Initializes the file-backed memory mapped mutable span data
//
// If a valid layout exists on disk, any active spans are recovered
// If the existing layout size matches the requested target size, the layout is
// reused. Otherwise, the file is resized and re-initialized to the target size
//
// Returns an opaque context to the initialized environment
unspecified_context_t* initialize_span_data(const std::string& path,
                                            MmapSize target_size);

// Takes the recovered spans, or returns an empty vector if there are none
//
// Spans can only be recovered from a context once
std::vector<Span> get_recovered_spans(unspecified_context_t* context);

// Returns the initialized mutable span data
//
// If mutable span data has not been initialized yet, returns nullptr
std::shared_ptr<MutableSpanData> get_mutable_span_data(
    unspecified_context_t* context);

// Deletes the initialized environment, and sets the given context to nullptr
//
// It is the callers responsibility to clean up after itself
void release_span_data(unspecified_context_t*& context);

}  // namespace firebase::telemetry::persistence

#endif  // __FIREBASE_TELEMETRY_PERSISTENCE_INITIALIZE_H__
