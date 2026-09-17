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
#include <utility>

#include "firebase/telemetry/persistence/detail/header.h"
#include "firebase/telemetry/persistence/detail/managed_mmap.h"
#include "firebase/telemetry/persistence/detail/raw_span_data.h"
#include "test_utils.h"

using namespace firebase::telemetry::persistence;
using namespace firebase::telemetry::persistence::detail;

TEST(RawSpanDataTest, HeaderValidationAndReset) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  EXPECT_FALSE(is_header_valid(header));

  reset_header(header, MmapSize::Small);
  EXPECT_TRUE(is_header_valid(header));

  RawSpanData<MmapSize::Small> raw_data(std::move(header));
  EXPECT_EQ(0, raw_data.count());
}

TEST(RawSpanDataTest, SetAndGetSpan) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  reset_header(header, MmapSize::Small);
  RawSpanData<MmapSize::Small> raw_data(std::move(header));

  Span src = simple_span(1, "TestSpan", {{"k", "v"}});

  raw_data.add(src);

  Span result = raw_data.get(0);
  EXPECT_EQ("TestSpan", result.name());
  EXPECT_EQ("v", get_attribute(result, "k"));
  EXPECT_GT(raw_data.get_mark(0), 0);
}

TEST(RawSpanDataTest, RemoveSpan) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  reset_header(header, MmapSize::Small);
  RawSpanData<MmapSize::Small> raw_data(std::move(header));

  Span src = simple_span(1, "ToRemove");
  raw_data.add(src);

  EXPECT_EQ("ToRemove", raw_data.get(0).name());

  raw_data.remove(0);
  EXPECT_EQ(0, raw_data.get_mark(0));
}

TEST(RawSpanDataTest, RemoveSpanFirst) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  reset_header(header, MmapSize::Small);
  RawSpanData<MmapSize::Small> raw_data(std::move(header));

  Span span1 = simple_span(100, "Span1");
  Span span2 = simple_span(200, "Span2");
  Span span3 = simple_span(300, "Span3");

  raw_data.add(span1);
  raw_data.add(span2);
  raw_data.add(span3);
  EXPECT_EQ(3, raw_data.count());

  std::uint64_t moved_id = raw_data.remove(0);

  EXPECT_EQ(300, moved_id);
  EXPECT_EQ(2, raw_data.count());

  EXPECT_EQ(300, raw_data.get(0).span_id());
  EXPECT_EQ("Span3", raw_data.get(0).name());

  EXPECT_EQ(0, raw_data.get_mark(2));
}

TEST(RawSpanDataTest, RemoveSpanMiddle) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  reset_header(header, MmapSize::Small);
  RawSpanData<MmapSize::Small> raw_data(std::move(header));

  Span span1 = simple_span(100, "Span1");
  Span span2 = simple_span(200, "Span2");
  Span span3 = simple_span(300, "Span3");

  raw_data.add(span1);
  raw_data.add(span2);
  raw_data.add(span3);
  EXPECT_EQ(3, raw_data.count());

  std::uint64_t moved_id = raw_data.remove(1);

  EXPECT_EQ(300, moved_id);
  EXPECT_EQ(2, raw_data.count());

  EXPECT_EQ(300, raw_data.get(1).span_id());
  EXPECT_EQ("Span3", raw_data.get(1).name());

  EXPECT_EQ(0, raw_data.get_mark(2));
}

TEST(RawSpanDataTest, RemoveSpanLast) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  reset_header(header, MmapSize::Small);
  RawSpanData<MmapSize::Small> raw_data(std::move(header));

  Span span1 = simple_span(100, "Span1");
  Span span2 = simple_span(200, "Span2");
  Span span3 = simple_span(300, "Span3");

  raw_data.add(span1);
  raw_data.add(span2);
  raw_data.add(span3);
  EXPECT_EQ(3, raw_data.count());

  std::uint64_t moved_id = raw_data.remove(2);

  EXPECT_EQ(0, moved_id);
  EXPECT_EQ(2, raw_data.count());

  EXPECT_EQ(200, raw_data.get(1).span_id());
  EXPECT_EQ("Span2", raw_data.get(1).name());

  EXPECT_EQ(0, raw_data.get_mark(2));
}

TEST(RawSpanDataTest, AttributeManipulation) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  reset_header(header, MmapSize::Small);
  RawSpanData<MmapSize::Small> raw_data(std::move(header));

  Span src = simple_span(1, "AttrTest");
  raw_data.add(src);

  std::size_t attr_index = raw_data.set_attribute_on_span(0, "key", "val");
  EXPECT_LT(attr_index, mmap_config<MmapSize::Small>.max_attributes);

  Span span = raw_data.get(0);
  EXPECT_EQ("val", get_attribute(span, "key"));

  raw_data.set_attribute_on_span(0, "key", "updated_val");
  EXPECT_EQ("updated_val", get_attribute(raw_data.get(0), "key"));
}

TEST(RawSpanDataTest, AttributeReuseLimit) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  reset_header(header, MmapSize::Small);
  RawSpanData<MmapSize::Small> raw_data(std::move(header));

  Span src = simple_span(1, "LimitTest");
  raw_data.add(src);

  std::size_t max_attrs = mmap_config<MmapSize::Small>.max_attributes;

  for (std::size_t i = 0; i < max_attrs; ++i) {
    raw_data.set_attribute_on_span(0, "k" + std::to_string(i), "v");
  }

  auto raw1 = raw_data.get_raw_span(0);
  EXPECT_EQ(max_attrs, raw1.attr_count);

  std::size_t fail_idx = raw_data.set_attribute_on_span(0, "too_many", "val");
  EXPECT_EQ(max_attrs, fail_idx);
}

TEST(RawSpanDataDeathTest, AddWhenFullDeathTest) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  reset_header(header, MmapSize::Small);
  RawSpanData<MmapSize::Small> raw_data(std::move(header));

  // Fill the data
  for (int i = 1; i <= raw_data.max_count(); ++i) {
    Span src = simple_span(i);
    raw_data.add(src);
  }
  EXPECT_EQ(raw_data.max_count(), raw_data.count());

  // Then add another one when full
  EXPECT_DEATH(
      { raw_data.add(simple_span(999)); },
      "raw_index < max_count");
}

TEST(RawSpanDataDeathTest, NullHeaderDeathTest) {
  EXPECT_DEATH(
      {
        RawSpanData<MmapSize::Small>(
            ManagedMmap<Header>(nullptr, 0, 0, 0, 0, 0));
      },
      "layout_.is_initialized()");
}

TEST(RawSpanDataDeathTest, WrongSizeDeathTestTooSmall) {
  EXPECT_DEATH(
      {
        RawSpanData<MmapSize::Small> raw_data(make_header(MmapSize::Medium));
        EXPECT_EQ(MmapSize::Small, raw_data.size());
      },
      "layout_->header.mmap_size == Size");
}

TEST(RawSpanDataDeathTest, WrongSizeDeathTestTooLarge) {
  EXPECT_DEATH(
      { RawSpanData<MmapSize::Medium> raw_data(make_header(MmapSize::Small)); },
      "<= required_size");
}
