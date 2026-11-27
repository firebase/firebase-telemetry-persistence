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

using unspecified_context_t = detail::Context;

// Recovers the active spans persisted in the file at the given path
//
// Returns an empty vector if the file does not exist, does not hold a valid
// layout, or has no active spans. The file is only read, never created or
// modified, so recovering it again returns the same spans
std::vector<Span> recover_spans(const std::string& path);

// Initializes the file-backed memory mapped mutable span data for a new session
//
// The file is created if needed, resized to the target size, and emptied. Any
// spans still in it are discarded without being recovered. It must not be in
// use elsewhere, because resizing it breaks any other mapping of it
//
// Returns an opaque context to the initialized environment, or nullptr if the
// file could not be opened, resized, or mapped
unspecified_context_t* initialize_span_data(const std::string& path,
                                            MmapSize target_size);

// Returns the mutable span data, or nullptr if the context is nullptr
std::shared_ptr<MutableSpanData> get_mutable_span_data(
    unspecified_context_t* context);

// Deletes the initialized environment, and sets the given context to nullptr
//
// It is the callers responsibility to clean up after itself
void release_span_data(unspecified_context_t*& context);

}  // namespace firebase::telemetry::persistence

#endif  // __FIREBASE_TELEMETRY_PERSISTENCE_INITIALIZE_H__
