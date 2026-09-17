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
using namespace firebase::telemetry::persistence::detail;

TEST(RecoveredSpanDataTest, Recover) {
  SpanDataImpl impl = make_span_data(MmapSize::Small);
  impl.add(Span({1, 2}, 3, 4, 5, 0, "RecoverMe", {{"a", "b"}}));

  RecoveredSpanData recovered_span_data(impl);
  EXPECT_EQ(1, recovered_span_data.count());
  EXPECT_GT(recovered_span_data.get_mark(0), 0);
  EXPECT_EQ(128, recovered_span_data.max_count());

  Span recovered = recovered_span_data.get(0);
  EXPECT_EQ("RecoverMe", recovered.name());
  EXPECT_EQ("b", get_attribute(recovered, "a"));
}

TEST(RecoveredSpanDataTest, RecoverMultipleSpans) {
  SpanDataImpl impl = make_span_data(MmapSize::Medium);
  impl.add(simple_span(1, "RecoverMe", {{"a", "b"}}));
  impl.add(simple_span(2, "RecoverMe2", {{"c", "d"}}));
  impl.add(simple_span(3, "RecoverMe3"));

  RecoveredSpanData recovered_span_data(impl);
  EXPECT_EQ(3, recovered_span_data.count());
  EXPECT_GT(recovered_span_data.get_mark(0), 0);
  EXPECT_GT(recovered_span_data.get_mark(1), 0);
  EXPECT_GT(recovered_span_data.get_mark(2), 0);
  EXPECT_EQ(1024, recovered_span_data.max_count());

  EXPECT_EQ(0, recovered_span_data.get_mark(3));

  Span recovered = recovered_span_data.get(0);
  EXPECT_EQ("RecoverMe", recovered.name());
  EXPECT_EQ("b", get_attribute(recovered, "a"));

  Span recovered2 = recovered_span_data.get(1);
  EXPECT_EQ("RecoverMe2", recovered2.name());
  EXPECT_EQ("d", get_attribute(recovered2, "c"));

  Span recovered3 = recovered_span_data.get(2);
  EXPECT_EQ("RecoverMe3", recovered3.name());
  EXPECT_TRUE(recovered3.attributes().empty());
}

TEST(RecoveredSpanDataTest, RecoverAfterMutations) {
  SpanDataImpl impl = make_span_data(MmapSize::Medium);
  MutableSpanData mutable_span_data(impl);
  mutable_span_data.add(simple_span(100, "RecoverMe", {{"a", "b"}}));
  mutable_span_data.add(simple_span(200, "RecoverMe2", {{"c", "d"}}));
  mutable_span_data.add(simple_span(300, "RecoverMe3"));

  mutable_span_data.set_attribute_on_span(100, "new_key", "new_value");
  mutable_span_data.set_attribute_on_span(100, "a", "new_value_for_a");

  // Removing span 200 does a swap_and_remove with span 300
  mutable_span_data.remove(200);

  mutable_span_data.set_attribute_on_span(300, "e", "f");

  RecoveredSpanData recovered_span_data(impl);
  EXPECT_EQ(2, recovered_span_data.count());

  EXPECT_GT(recovered_span_data.get_mark(0), 0);
  Span recovered = recovered_span_data.get(0);
  EXPECT_EQ("RecoverMe", recovered.name());
  EXPECT_EQ("new_value_for_a", get_attribute(recovered, "a"));
  EXPECT_EQ("new_value", get_attribute(recovered, "new_key"));

  EXPECT_GT(recovered_span_data.get_mark(1), 0);

  Span recovered3 = recovered_span_data.get(1);
  EXPECT_EQ("RecoverMe3", recovered3.name());
  EXPECT_EQ("f", get_attribute(recovered3, "e"));
}

TEST(RecoveredSpanDataTest, GetRemovedSpanDeathTest) {
  SpanDataImpl impl = make_span_data(MmapSize::Small);
  MutableSpanData mutable_span_data(impl);
  mutable_span_data.add(simple_span(100, "RecoverMe", {{"a", "b"}}));
  mutable_span_data.add(simple_span(200, "DeleteMe", {{"c", "e"}}));

  mutable_span_data.remove(200);

  RecoveredSpanData recovered_span_data(impl);

  EXPECT_EQ("RecoverMe", recovered_span_data.get(0).name());
  EXPECT_DEATH({ recovered_span_data.get(1); }, "mark > 0");  // DeleteMe
}
