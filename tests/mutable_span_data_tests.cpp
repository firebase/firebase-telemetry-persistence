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

#include "firebase/telemetry/persistence/detail/span_data_impl.h"
#include "firebase/telemetry/persistence/mutable_span_data.h"
#include "firebase/telemetry/persistence/recovered_span_data.h"
#include "test_utils.h"

using namespace firebase::telemetry::persistence;
using namespace detail;

TEST(MutableSpanDataTest, OverwriteSpan) {
  SpanDataImpl impl = make_span_data(MmapSize::Medium);
  MutableSpanData mutable_span_data(impl);
  mutable_span_data.add(simple_span(100, "Original", {{"a", "b"}}));
  mutable_span_data.add(simple_span(100, "Overwrite", {{"c", "d"}}));

  RecoveredSpanData recovered_span_data(impl);
  EXPECT_EQ(1, recovered_span_data.count());
  EXPECT_GT(recovered_span_data.get_mark(0), 0);
  EXPECT_EQ(0, recovered_span_data.get_mark(1));

  Span recovered = recovered_span_data.get(0);
  EXPECT_EQ("Original", recovered.name());
  EXPECT_EQ("b", get_attribute(recovered, "a"));
}

TEST(MutableSpanDataTest, SwapAndRemove) {
  SpanDataImpl impl = make_span_data(MmapSize::Small);
  MutableSpanData mutable_span_data(impl);

  Span span1 = simple_span(100, "Span1");
  Span span2 = simple_span(200, "Span2");
  Span span3 = simple_span(300, "Span3");

  mutable_span_data.add(span1);
  mutable_span_data.add(span2);
  mutable_span_data.add(span3);
  EXPECT_EQ(3, mutable_span_data.count());

  // Remove span 200, raw data swaps in span 300
  mutable_span_data.remove(200);
  EXPECT_EQ(2, mutable_span_data.count());

  mutable_span_data.set_attribute_on_span(300, "swapped_key", "swapped_val");

  RecoveredSpanData recovered_span_data(impl);
  EXPECT_EQ(2, recovered_span_data.count());

  EXPECT_EQ(100, recovered_span_data.get(0).span_id());

  Span recovered_span3 = recovered_span_data.get(1);
  EXPECT_EQ(300, recovered_span3.span_id());
  EXPECT_EQ("Span3", recovered_span3.name());
  EXPECT_EQ("swapped_val", get_attribute(recovered_span3, "swapped_key"));

  EXPECT_EQ(0, recovered_span_data.get_mark(2));
}

TEST(MutableSpanDataTest, AddWhenFullPrunes) {
  SpanDataImpl impl = make_span_data(MmapSize::Small);
  MutableSpanData mutable_span_data(impl);

  // Fill the data
  for (int i = 1; i <= impl.max_count(); ++i) {
    Span src = simple_span(i, "Span_" + std::to_string(i));
    mutable_span_data.add(src);
  }
  EXPECT_EQ(impl.max_count(), mutable_span_data.count());

  // Then add another one when full
  mutable_span_data.add(simple_span(999, "NewestSpan"));
  EXPECT_EQ(impl.max_count(), mutable_span_data.count());

  mutable_span_data.set_attribute_on_span(1, "key", "should_be_ignored");
  mutable_span_data.set_attribute_on_span(999, "key", "updated_value");

  RecoveredSpanData recovered_span_data(impl);
  EXPECT_EQ(impl.max_count(), recovered_span_data.count());

  bool found_1 = false;
  bool found_999 = false;
  for (index_t i = 0; i < recovered_span_data.count(); ++i) {
    Span span = recovered_span_data.get(i);
    if (span.span_id() == 1) found_1 = true;
    if (span.span_id() == 999) {
      found_999 = true;
      EXPECT_EQ("updated_value", get_attribute(span, "key"));
    }
  }

  EXPECT_FALSE(found_1);   // Span 1 was the oldest and should be pruned
  EXPECT_TRUE(found_999);  // Span 999 was just added and should be present
}

TEST(MutableSpanDataTest, StressTest) {
  SpanDataImpl impl = make_span_data(MmapSize::Small);
  MutableSpanData mutable_span_data(impl);

  // Run 200 steps of adding 2 spans, and removing 1
  for (std::uint64_t i = 1; i <= 200; ++i) {
    mutable_span_data.add(simple_span(2 * i - 1));
    mutable_span_data.add(simple_span(2 * i));
    mutable_span_data.remove(i);
  }

  RecoveredSpanData recovered(impl);
  EXPECT_EQ(128, recovered.count());  // max_spans for Small size is 128

  std::uint64_t min_id = UINT64_MAX;
  std::uint64_t max_id = 0;
  std::uint64_t sum_ids = 0;

  for (index_t i = 0; i < recovered.count(); ++i) {
    std::uint64_t id = recovered.get(i).span_id();
    if (id < min_id) {
      min_id = id;
    }
    if (id > max_id) {
      max_id = id;
    }
    sum_ids += id;
  }

  EXPECT_EQ(273, min_id);
  EXPECT_EQ(400, max_id);
  EXPECT_EQ(43072, sum_ids);  // Expect 128 * (273 + 400) / 2
}

TEST(MutableSpanDataTest, EndSpanSetsEndTime) {
  SpanDataImpl impl = make_span_data(MmapSize::Small);
  MutableSpanData mutable_span_data(impl);

  Span span = simple_span(100, "SpanToEnd", {}, 1000);
  mutable_span_data.add(span);
  EXPECT_EQ(0, span.end_time());

  mutable_span_data.end(100, /*end_time=*/2000);

  RecoveredSpanData recovered(impl);
  ASSERT_EQ(1, recovered.count());

  Span recovered_span = recovered.get(0);
  EXPECT_EQ(100, recovered_span.span_id());
  EXPECT_EQ(2000, recovered_span.end_time());
}
