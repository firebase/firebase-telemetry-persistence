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
#include "firebase/telemetry/persistence/detail/managed_mmap.h"
#include "firebase/telemetry/persistence/detail/memory_layout.h"
#include "test_utils.h"

using namespace firebase::telemetry::persistence;
using namespace detail;

TEST(ManagedMmapTest, IsInitalized) {
  ManagedMmap<Header> header = make_header(MmapSize::Medium);

  EXPECT_EQ(MmapSize::Medium, header->mmap_size);
  EXPECT_TRUE(header.is_initialized());
}

TEST(ManagedMmapTest, MapFailed) {
  // This causes mmap to return MAP_FAILED
  ManagedMmap<Header> header(nullptr, 0, 0, 0, 0, 0);

  EXPECT_FALSE(header.is_initialized());
}

TEST(ManagedMmapTest, MoveCtor) {
  ManagedMmap<Header> header = make_header(MmapSize::Large);
  EXPECT_TRUE(header.is_initialized());

  ManagedMmap<Header> moved = std::move(header);

  EXPECT_FALSE(header.is_initialized());
  EXPECT_TRUE(moved.is_initialized());
  EXPECT_EQ(MmapSize::Large, moved->mmap_size);
}

TEST(ManagedMmapTest, MoveOp) {
  ManagedMmap<Header> moved(nullptr, 0, 0, 0, 0, 0);
  EXPECT_FALSE(moved.is_initialized());

  ManagedMmap<Header> header = make_header(MmapSize::Small);
  EXPECT_TRUE(header.is_initialized());

  moved = std::move(header);

  EXPECT_FALSE(header.is_initialized());
  EXPECT_TRUE(moved.is_initialized());
  EXPECT_EQ(MmapSize::Small, moved->mmap_size);
}

TEST(ManagedMmapTest, SelfMoveOp) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  EXPECT_TRUE(header.is_initialized());

  header = std::move(header);

  EXPECT_TRUE(header.is_initialized());
  header->mmap_size = MmapSize::Small;
}

TEST(ManagedMmapTest, ReinterpretMoveCtor) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  EXPECT_TRUE(header.is_initialized());

  ManagedMmap<MemoryLayout<MmapSize::Small>> memory_layout = std::move(header);

  EXPECT_FALSE(header.is_initialized());
  EXPECT_TRUE(memory_layout.is_initialized());
}

TEST(ManagedMmapTest, ReinterpretMoveOp) {
  ManagedMmap<MemoryLayout<MmapSize::Small>> memory_layout(nullptr, 0, 0,
                                                           0, 0, 0);

  ManagedMmap<Header> header = make_header(MmapSize::Medium);

  memory_layout = std::move(header);

  EXPECT_FALSE(header.is_initialized());
  EXPECT_TRUE(memory_layout.is_initialized());
  EXPECT_EQ(MmapSize::Medium, memory_layout->header.mmap_size);
}

TEST(ManagedMmapTest, MappedMoveOp) {
  ManagedMmap<Header> mapped = make_header(MmapSize::Small);
  EXPECT_TRUE(mapped.is_initialized());
  EXPECT_EQ(MmapSize::Small, mapped->mmap_size);

  ManagedMmap<Header> header = make_header(MmapSize::Medium);
  EXPECT_TRUE(header.is_initialized());

  mapped = std::move(header);

  EXPECT_FALSE(header.is_initialized());
  EXPECT_TRUE(mapped.is_initialized());
  EXPECT_EQ(MmapSize::Medium, mapped->mmap_size);
}

TEST(ManagedMmapTest, RequiredSizeTooSmallDeathTest) {
  EXPECT_DEATH(
      {
        ManagedMmap<Header> memory_layout(
            nullptr, /*required_size=*/sizeof (Header) - 1, 0, 0, 0, 0);
      },
      "<= required_size");
}

TEST(ManagedMmapTest, ReinterpretMoveCtorTooSmallDeathTest) {
  EXPECT_DEATH(
      {
        ManagedMmap<MemoryLayout<MmapSize::Large>> memory_layout(nullptr, 0, 0,
                                                                 0, 0, 0);

        // Header for a memory layout that's too small for memory_layout
        ManagedMmap<Header> header = make_header(MmapSize::Small);

        memory_layout = std::move(header);
      },
      "<= required_size");
}

TEST(ManagedMmapTest, ReinterpretMoveOpTooSmallDeathTest) {
  EXPECT_DEATH(
      {
        ManagedMmap<MemoryLayout<MmapSize::Large>> memory_layout =
            make_header(MmapSize::Small);  // Too small for memory_layout
      },
      "<= required_size");
}
