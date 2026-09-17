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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_TYPES_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_TYPES_H__

#include <cstdint>

namespace firebase::telemetry::persistence::detail {

// Represents an index in an array
typedef std::uint64_t index_t;

// Represents a mark on a marked span
typedef std::uint64_t mark_t;

}  // namespace firebase::telemetry::persistence::detail

#endif  //__FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_TYPES_H__
