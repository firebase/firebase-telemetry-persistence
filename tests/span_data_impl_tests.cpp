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
#include "firebase/telemetry/persistence/detail/span_data_impl.h"
#include "test_utils.h"

using namespace firebase::telemetry::persistence;
using namespace firebase::telemetry::persistence::detail;

TEST(SpanDataImplTest, HeaderValidationAndReset) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  EXPECT_FALSE(is_header_valid(header));

  reset_header(header, MmapSize::Small);
  EXPECT_TRUE(is_header_valid(header));

  SpanDataImpl span_data(std::move(header));
  EXPECT_EQ(0, span_data.count());
}

TEST(SpanDataImplTest, SetAndGetSpan) {
  ManagedMmap<Header> header = make_header(MmapSize::Medium);
  reset_header(header, MmapSize::Medium);
  SpanDataImpl span_data(std::move(header));

  Span src = simple_span(1, "TestSpan", {{"k", "v"}});

  span_data.add(src);

  Span result = span_data.get(0);
  EXPECT_EQ("TestSpan", result.name());
  EXPECT_EQ("v", get_attribute(result, "k"));

  EXPECT_GT(span_data.get_mark(0), 0);
  EXPECT_EQ(1, span_data.get_span_id(0));
}

TEST(SpanDataImplTest, MaxSpanCount) {
  ManagedMmap<Header> header = make_header(MmapSize::Large);
  SpanDataImpl span_data(std::move(header));

  EXPECT_EQ(2048, span_data.max_count());
}

TEST(SpanDataImplTest, ClearSpan) {
  ManagedMmap<Header> header = make_header(MmapSize::Large);
  reset_header(header, MmapSize::Large);
  SpanDataImpl span_data(std::move(header));

  Span src = simple_span(1, "ToClear");
  span_data.add(src);

  EXPECT_EQ("ToClear", span_data.get(0).name());

  span_data.remove(0);
  EXPECT_EQ(0, span_data.get_mark(0));
}

TEST(SpanDataImplTest, AttributeManipulation) {
  ManagedMmap<Header> header = make_header(MmapSize::Small);
  reset_header(header, MmapSize::Small);
  SpanDataImpl span_data(std::move(header));

  Span src = simple_span(1, "AttrTest");
  span_data.add(src);

  std::size_t attr_index = span_data.set_attribute_on_span(0, "key", "val");
  EXPECT_LT(attr_index, mmap_config<MmapSize::Small>.max_attributes);

  Span span = span_data.get(0);
  EXPECT_EQ("val", get_attribute(span, "key"));

  span_data.set_attribute_on_span(0, "key", "updated_val");
  EXPECT_EQ("updated_val", get_attribute(span_data.get(0), "key"));
}
