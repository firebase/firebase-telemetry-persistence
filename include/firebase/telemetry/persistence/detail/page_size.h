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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_PAGE_SIZE_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_PAGE_SIZE_H__

#include <cstddef>
#include <unistd.h>

namespace firebase::telemetry::persistence::detail {

// Safely get the system page size
inline std::size_t get_page_size() {
  static long page_size = sysconf(_SC_PAGESIZE);
  static std::size_t adjusted_page_size = page_size <= 0
      ? 4096  // default to 4 KiB
      : static_cast<std::size_t>(page_size);
  return adjusted_page_size;
}

}  // namespace firebase::telemetry::persistence::detail

#endif  // __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_PAGE_SIZE_H__
