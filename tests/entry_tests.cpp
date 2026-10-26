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

#include <cstddef>
#include <cstdint>

#include "firebase/telemetry/persistence/detail/entry.h"

using namespace firebase::telemetry::persistence;
using namespace detail;

TEST(EntryTest, Layout) {
  EXPECT_EQ(8, sizeof (Entry));
  EXPECT_EQ(0, offsetof(Entry, tag));
  EXPECT_EQ(4, offsetof(Entry, size));
}

TEST(EntryTest, TagValues) {
  // These values are persisted on disk, so they must never change
  EXPECT_EQ(0, static_cast<std::uint32_t>(Entry::Tag::Empty));
  EXPECT_EQ(1, static_cast<std::uint32_t>(Entry::Tag::Span));
  EXPECT_EQ(2, static_cast<std::uint32_t>(Entry::Tag::Log));
}

TEST(EntryTest, IsTagValid) {
  EXPECT_TRUE(is_tag_valid(Entry::Tag::Empty));
  EXPECT_TRUE(is_tag_valid(Entry::Tag::Span));
  EXPECT_TRUE(is_tag_valid(Entry::Tag::Log));

  EXPECT_FALSE(is_tag_valid(static_cast<Entry::Tag>(3)));
  EXPECT_FALSE(is_tag_valid(static_cast<Entry::Tag>(UINT32_MAX)));
}

TEST(EntryTest, ReadEntryNullDataReturnsNull) {
  EXPECT_EQ(nullptr, read_entry(nullptr));
}

TEST(EntryTest, ReadEntryReturnsPointerIntoData) {
  std::uint32_t data[] = {3, 20};
  Entry* entry = read_entry(reinterpret_cast<std::byte *>(data));

  EXPECT_EQ(static_cast<void *>(data), static_cast<void *>(entry));
}

TEST(EntryTest, ReadEntryEmpty) {
  std::uint32_t data[] = {0, 0};
  Entry* entry = read_entry(reinterpret_cast<std::byte *>(data));

  EXPECT_EQ(Entry::Tag::Empty, entry->tag);
  EXPECT_EQ(0, entry->size);
}

TEST(EntryTest, ReadEntryEmptyWithSize) {
  std::uint32_t data[] = {0, 48};
  Entry* entry = read_entry(reinterpret_cast<std::byte *>(data));

  EXPECT_EQ(Entry::Tag::Empty, entry->tag);
  EXPECT_EQ(48, entry->size);
}

TEST(EntryTest, ReadEntrySpan) {
  std::uint32_t data[] = {1, 48};
  Entry* entry = read_entry(reinterpret_cast<std::byte *>(data));

  EXPECT_EQ(Entry::Tag::Span, entry->tag);
  EXPECT_EQ(48, entry->size);
  EXPECT_EQ(1, data[0]);  // A valid tag is left alone
}

TEST(EntryTest, ReadEntryLog) {
  std::uint32_t data[] = {2, 1024};
  Entry* entry = read_entry(reinterpret_cast<std::byte *>(data));

  EXPECT_EQ(Entry::Tag::Log, entry->tag);
  EXPECT_EQ(1024, entry->size);
  EXPECT_EQ(2, data[0]);
}

TEST(EntryTest, ReadEntryUnknownTagHealsToEmpty) {
  // 3 is the first unassigned tag. It must read as Empty and heal itself
  std::uint32_t data[] = {3, 1024};
  Entry* entry = read_entry(reinterpret_cast<std::byte *>(data));

  EXPECT_EQ(Entry::Tag::Empty, entry->tag);
  EXPECT_EQ(0, data[0]);
}

TEST(EntryTest, ReadEntryUnknownTagKeepsSize) {
  std::uint32_t data[] = {3, 48};
  Entry* entry = read_entry(reinterpret_cast<std::byte *>(data));

  EXPECT_EQ(48, entry->size);
  EXPECT_EQ(48, data[1]);
}

TEST(EntryTest, ReadEntryAllBitsSet) {
  // Every bit in both words set: tag heals, size is preserved untouched
  std::uint32_t data[] = {UINT32_MAX, UINT32_MAX};
  Entry* entry = read_entry(reinterpret_cast<std::byte *>(data));

  EXPECT_EQ(Entry::Tag::Empty, entry->tag);
  EXPECT_EQ(0, data[0]);
  EXPECT_EQ(UINT32_MAX, entry->size);
  EXPECT_EQ(UINT32_MAX, data[1]);
}

TEST(EntryTest, ReadEntryIsIdempotent) {
  // Reading a healed entry again must not change it further
  std::uint32_t data[] = {3, 48};
  read_entry(reinterpret_cast<std::byte *>(data));
  Entry* entry = read_entry(reinterpret_cast<std::byte *>(data));

  EXPECT_EQ(Entry::Tag::Empty, entry->tag);
  EXPECT_EQ(48, entry->size);
  EXPECT_EQ(0, data[0]);
  EXPECT_EQ(48, data[1]);
}
