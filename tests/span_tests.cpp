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

#include <utility>
#include <vector>

#include "firebase/telemetry/persistence/detail/memory_layout.h"
#include "firebase/telemetry/persistence/span.h"
#include "test_utils.h"

using namespace firebase::telemetry::persistence;
using namespace firebase::telemetry::persistence::detail;

TEST(SpanTest, ToRawSpanBasic) {
  Span src({0x1234, 0x5678}, 0xABCD, 0x1111, 123456789, 234567890, "TestSpan",
           {{"key1", "val1"}, {"key2", "val2"}});

  RawSpan<MmapSize::Small> dest = src.to_raw_span<MmapSize::Small>();

  EXPECT_EQ(src.trace_id().high, dest.trace_id[0]);
  EXPECT_EQ(src.trace_id().low, dest.trace_id[1]);
  EXPECT_EQ(src.span_id(), dest.span_id);
  EXPECT_EQ(src.parent_span_id(), dest.parent_span_id);
  EXPECT_EQ(src.start_time(), dest.start_time);
  EXPECT_EQ(src.end_time(), dest.end_time);
  EXPECT_STREQ("TestSpan", dest.name);

  EXPECT_EQ(2, dest.attr_count);

  EXPECT_STREQ("key1", dest.attributes[0].key);
  EXPECT_STREQ("val1", dest.attributes[0].value);
  EXPECT_STREQ("key2", dest.attributes[1].key);
  EXPECT_STREQ("val2", dest.attributes[1].value);
}

TEST(SpanTest, ToRawSpanAttributeOverflow) {
  std::vector<std::pair<std::string, std::string>> attributes;
  for (int i = 0; i < 20; ++i) {
    attributes.emplace_back("key" + std::to_string(i), "val");
  }
  Span src({0, 0}, 1, 0, 0, 0, "OverflowTest", attributes);

  RawSpan<MmapSize::Small> dest = src.to_raw_span<MmapSize::Small>();

  // Should be capped at 16 for Small
  EXPECT_EQ(16, dest.attr_count);
}

TEST(SpanTest, FromRawSpanBasic) {
  RawSpan<MmapSize::Small> src{};
  src.trace_id[0] = 0x1111222233334444ULL;
  src.trace_id[1] = 0x5555666677778888ULL;
  src.span_id = 0x3333;
  src.parent_span_id = 0x4444;
  src.start_time = 5555;
  src.end_time = 6666;
  copy_string("RawName", src.name);
  src.attr_count = 1;
  copy_string("k1", src.attributes[0].key);
  copy_string("v1", src.attributes[0].value);

  Span dest = static_cast<Span>(src);

  EXPECT_EQ(0x1111222233334444ULL, dest.trace_id().high);
  EXPECT_EQ(0x5555666677778888ULL, dest.trace_id().low);
  EXPECT_EQ(src.span_id, dest.span_id());
  EXPECT_EQ(src.parent_span_id, dest.parent_span_id());
  EXPECT_EQ(src.start_time, dest.start_time());
  EXPECT_EQ(src.end_time, dest.end_time());
  EXPECT_EQ("RawName", dest.name());
  EXPECT_EQ(1, dest.attributes().size());
  EXPECT_EQ("v1", get_attribute(dest, "k1"));
}

TEST(SpanTest, RoundTripTest) {
  Span original({0xAAAA, 0xBBBB}, 0xCCCC, 0xDDDD, 888888, 999999, "RoundTrip",
                {{"a", "1"}, {"b", "2"}});

  // Copy original -> intermediate -> result
  auto intermediate = original.to_raw_span<MmapSize::Small>();
  Span result = static_cast<Span>(intermediate);

  EXPECT_EQ(original.trace_id().high, result.trace_id().high);
  EXPECT_EQ(original.trace_id().low, result.trace_id().low);
  EXPECT_EQ(original.span_id(), result.span_id());
  EXPECT_EQ(original.parent_span_id(), result.parent_span_id());
  EXPECT_EQ(original.start_time(), result.start_time());
  EXPECT_EQ(original.end_time(), result.end_time());
  EXPECT_EQ(original.name(), result.name());
  EXPECT_EQ(original.attributes(), result.attributes());
}

TEST(SpanTest, ToRawSpanInSimulatedMmap) {
  // Simulate mmap by allocating the full layout size
  auto* layout = static_cast<MemoryLayout<MmapSize::Small> *>(
      std::malloc(sizeof (MemoryLayout<MmapSize::Small>)));

  Span src({0, 0}, 1, 0, 0, 0, "MmapTest", {{"key", "value"}});

  layout->marked_spans[3].span = src.to_raw_span<MmapSize::Small>();

  EXPECT_STREQ("MmapTest", layout->marked_spans[3].span.name);
  EXPECT_EQ(1, layout->marked_spans[3].span.attr_count);
  EXPECT_STREQ("key", layout->marked_spans[3].span.attributes[0].key);
  EXPECT_STREQ("value", layout->marked_spans[3].span.attributes[0].value);

  std::free(layout);
}
