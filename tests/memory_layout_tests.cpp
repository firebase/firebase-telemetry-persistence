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

#include <cstdint>
#include <gtest/gtest.h>

#include "firebase/telemetry/persistence/mmap_size.h"
#include "firebase/telemetry/persistence/detail/memory_layout.h"

using namespace firebase::telemetry::persistence;

namespace firebase::telemetry::persistence::detail {

extern const std::uint64_t header_version;

}

TEST(MemoryLayoutTest, EnforceTotalSizes) {
  EXPECT_EQ(409632, sizeof (detail::MemoryLayout<MmapSize::Small>));
  EXPECT_EQ(12714016, sizeof (detail::MemoryLayout<MmapSize::Medium>));
  EXPECT_EQ(50593824, sizeof (detail::MemoryLayout<MmapSize::Large>));

  // If the memory layout changes, the header version must change too
  EXPECT_EQ(2, detail::header_version);
}
