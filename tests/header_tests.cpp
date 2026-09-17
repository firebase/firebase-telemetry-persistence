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

#include <gtest/gtest.h>

#include "firebase/telemetry/persistence/detail/header.h"
#include "firebase/telemetry/persistence/mmap_size.h"
#include "test_utils.h"

using namespace firebase::telemetry::persistence;
using namespace detail;

namespace firebase::telemetry::persistence::detail {

extern const std::uint64_t header_signature;

}

TEST(HeaderTest, ValidHeader) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  reset_header(header, MmapSize::Small);
  EXPECT_TRUE(is_header_valid(header));
  EXPECT_TRUE(is_header_version_current(header));

  header->version = 0;  // invalid version
  EXPECT_TRUE(is_header_valid(header));
  EXPECT_FALSE(is_header_version_current(header));

  header->mmap_size = static_cast<MmapSize>(42);  // invalid size
  EXPECT_FALSE(is_header_valid(header));
}

TEST(HeaderTest, ValidationAndReset) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  EXPECT_FALSE(is_header_valid(header));

  reset_header(header, MmapSize::Small);
  EXPECT_TRUE(is_header_valid(header));
}

TEST(HeaderTest, VerifySignature) {
  // Ensure the signature part doesn't accidentally change
  EXPECT_EQ(0x48535243ULL, header_signature);
}
