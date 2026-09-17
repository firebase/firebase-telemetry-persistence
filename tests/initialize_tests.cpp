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
#include <sys/stat.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "firebase/telemetry/persistence/detail/memory_layout.h"
#include "firebase/telemetry/persistence/detail/mmap_config.h"
#include "firebase/telemetry/persistence/detail/page_size.h"
#include "firebase/telemetry/persistence/initialize.h"
#include "firebase/telemetry/persistence/mmap_size.h"
#include "firebase/telemetry/persistence/span.h"
#include "test_utils.h"

using namespace firebase::telemetry::persistence;
using namespace detail;

std::size_t required_size(MmapSize size) {
  std::size_t page_size = detail::get_page_size();
  std::size_t memory_layout_size = detail::get_memory_layout_size(size);
  return (memory_layout_size + page_size - 1) / page_size * page_size;
}

std::size_t file_size(const char* path) {
  struct stat st;
  if (stat(path, &st) != -1) {
    return st.st_size;
  }
  return 0;
}

void remove_file(const char* path) { std::remove(path); }

namespace firebase::telemetry::persistence::detail {
extern const std::uint64_t header_signature;
}

void write_file(const char* path, const char* content) {
  std::ofstream(path) << content;
}

void write_binary_file(const char* path, const void* data, std::size_t size) {
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(data), size);
}

class InitializeTest : public testing::Test {
protected:
  static constexpr char mmap_file[] = "data.mmap";

  void SetUp() override { remove_file(mmap_file); }
  void TearDown() override { remove_file(mmap_file); }
};

TEST_F(InitializeTest, InitializeFromNewFile) {
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);

  // Expect nullptr because didn't recover yet
  EXPECT_EQ(nullptr, get_mutable_span_data(context));

  // Expect not to recover spans because file is new
  std::vector<Span> recovered = get_recovered_spans(context);
  EXPECT_TRUE(recovered.empty());

  std::shared_ptr<MutableSpanData> mutable_span_data =
      get_mutable_span_data(context);

  // Verify the header got reset
  EXPECT_EQ(0, get_mutable_span_data(context)->count());

  // Verify the get_mutable_span_data returns same instance
  std::shared_ptr<MutableSpanData> mutable_span_data2 =
      get_mutable_span_data(context);
  EXPECT_EQ(mutable_span_data, mutable_span_data2);
  EXPECT_EQ(mutable_span_data.get(), mutable_span_data2.get());

  release_span_data(context);
}

TEST_F(InitializeTest, InitializeFromCorruptFile) {
  write_file(mmap_file, "CRSH and some junk data \01 \02 \03 \04 whatevs");

  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);

  // Expect nullptr because didn't recover yet
  EXPECT_EQ(nullptr, get_mutable_span_data(context));

  // Expect not to recover spans because file was corrupt
  std::vector<Span> recovered = get_recovered_spans(context);
  EXPECT_TRUE(recovered.empty());

  // Verify the header got reset
  EXPECT_EQ(0, get_mutable_span_data(context)->count());

  release_span_data(context);
}

TEST_F(InitializeTest, InitializeFromLargeCorruptFile) {
  std::string data(8192, 'A');
  write_file(mmap_file, data.c_str());

  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);

  EXPECT_EQ(nullptr, get_mutable_span_data(context));
  EXPECT_TRUE(get_recovered_spans(context).empty());
  EXPECT_EQ(0, get_mutable_span_data(context)->count());

  release_span_data(context);
}

TEST_F(InitializeTest, InitializeFromInvalidHeaderSignature) {
  Header bad_header{};
  bad_header.signature = 0xBAD00BADULL;
  bad_header.version = 2;
  bad_header.mmap_size = MmapSize::Small;
  bad_header.active_span_count = 5;
  write_binary_file(mmap_file, &bad_header, sizeof(bad_header));

  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);
  ASSERT_NE(nullptr, context);

  // Recovery should fail gracefully due to invalid signature boundary check
  EXPECT_TRUE(get_recovered_spans(context).empty());
  EXPECT_EQ(0, get_mutable_span_data(context)->count());

  release_span_data(context);
}

TEST_F(InitializeTest, InitializeFromInvalidHeaderVersion) {
  Header bad_header{};
  bad_header.signature = header_signature;
  bad_header.version = 9999ULL;  // Unsupported future version
  bad_header.mmap_size = MmapSize::Small;
  bad_header.active_span_count = 5;
  write_binary_file(mmap_file, &bad_header, sizeof(bad_header));

  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);
  ASSERT_NE(nullptr, context);

  // Recovery should fail gracefully due to version boundary check
  EXPECT_TRUE(get_recovered_spans(context).empty());
  EXPECT_EQ(0, get_mutable_span_data(context)->count());

  release_span_data(context);
}

TEST_F(InitializeTest, InitializeFromInvalidMmapSizeEnum) {
  Header bad_header{};
  bad_header.signature = header_signature;
  bad_header.version = 2;
  bad_header.mmap_size = static_cast<MmapSize>(255);  // Invalid enum value
  bad_header.active_span_count = 5;
  write_binary_file(mmap_file, &bad_header, sizeof(bad_header));

  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);
  ASSERT_NE(nullptr, context);

  // Recovery should fail gracefully due to size validation check
  EXPECT_TRUE(get_recovered_spans(context).empty());
  EXPECT_EQ(0, get_mutable_span_data(context)->count());

  release_span_data(context);
}

TEST_F(InitializeTest, InitializeFromTruncatedLayoutSize) {
  // Write a valid header for Medium layout, but truncate the file so
  // mapped_size < layout_size
  Header header{};
  header.signature = header_signature;
  header.version = 2;
  header.mmap_size = MmapSize::Medium;
  header.active_span_count = 3;
  write_binary_file(mmap_file, &header, sizeof(header));

  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Medium);
  ASSERT_NE(nullptr, context);

  // Recovery should fail gracefully due to mapped size boundary check
  EXPECT_TRUE(get_recovered_spans(context).empty());
  EXPECT_EQ(0, get_mutable_span_data(context)->count());

  release_span_data(context);
}

TEST_F(InitializeTest, InitializeFromZeroActiveSpans) {
  Header header{};
  header.signature = header_signature;
  header.version = 2;
  header.mmap_size = MmapSize::Small;
  header.active_span_count = 0;
  write_binary_file(mmap_file, &header, sizeof(header));

  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);
  ASSERT_NE(nullptr, context);

  // Recovery should be skipped because active_span_count == 0
  EXPECT_TRUE(get_recovered_spans(context).empty());
  EXPECT_EQ(0, get_mutable_span_data(context)->count());

  release_span_data(context);
}

TEST_F(InitializeTest, InitializeFromExistingFile) {
  // Set up the "existing" file
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);
  EXPECT_TRUE(get_recovered_spans(context).empty());

  std::shared_ptr<MutableSpanData> span_data = get_mutable_span_data(context);
  span_data->add(simple_span(100, "span00"));
  span_data->add(simple_span(101, "span01"));
  span_data->add(simple_span(102, "span02", {{"key", "value"}}));
  EXPECT_EQ(3, span_data->count());

  release_span_data(context);
  EXPECT_EQ(nullptr, context);

  // Initialize from the existing file
  context = initialize_span_data(mmap_file, MmapSize::Small);

  // Get the expected 3 recovered spans
  std::vector<Span> recovered = get_recovered_spans(context);
  EXPECT_EQ(3, recovered.size());
  EXPECT_EQ("span00", recovered[0].name());
  EXPECT_EQ("span01", recovered[1].name());
  EXPECT_EQ("span02", recovered[2].name());
  EXPECT_EQ("value", detail::get_attribute(recovered[2], "key"));

  // Verify the recovered spans have been moved out of the context
  EXPECT_TRUE(get_recovered_spans(context).empty());

  // Verify the header got reset
  EXPECT_EQ(0, get_mutable_span_data(context)->count());

  // Verify the span data got cleared, otherwise this would fail an assert check
  get_mutable_span_data(context)->add(simple_span(100));

  release_span_data(context);

  EXPECT_EQ(required_size(MmapSize::Small), file_size(mmap_file));
}

TEST_F(InitializeTest, InitializeFromSmallerSize) {
  // Set up the existing small file
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);
  EXPECT_TRUE(get_recovered_spans(context).empty());

  std::shared_ptr<MutableSpanData> span_data = get_mutable_span_data(context);
  span_data->add(simple_span(200, "span00"));
  span_data->add(simple_span(201, "span01"));
  span_data->add(simple_span(202, "span02", {{"key", "value"}}));
  EXPECT_EQ(3, span_data->count());

  release_span_data(context);
  EXPECT_EQ(nullptr, context);
  EXPECT_EQ(required_size(MmapSize::Small), file_size(mmap_file));

  // Initialize a medium memory layout from the existing small file
  context = initialize_span_data(mmap_file, MmapSize::Medium);

  // Get the expected 3 recovered spans, even though they were small
  std::vector<Span> recovered = get_recovered_spans(context);
  EXPECT_EQ(3, recovered.size());
  EXPECT_EQ("span00", recovered[0].name());
  EXPECT_EQ("span01", recovered[1].name());
  EXPECT_EQ("span02", recovered[2].name());
  EXPECT_EQ("value", detail::get_attribute(recovered[2], "key"));

  EXPECT_EQ(0, get_mutable_span_data(context)->count());
  get_mutable_span_data(context)->add(simple_span(100));

  release_span_data(context);

  // Verify the file got expanded to the required size for medium
  EXPECT_EQ(required_size(MmapSize::Medium), file_size(mmap_file));
}

TEST_F(InitializeTest, InitializeFromLargerSize) {
  // Set up the existing larger file
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Medium);
  EXPECT_TRUE(get_recovered_spans(context).empty());

  std::shared_ptr<MutableSpanData> span_data = get_mutable_span_data(context);
  span_data->add(simple_span(300, "span00"));
  span_data->add(simple_span(301, "span01"));
  span_data->add(simple_span(302, "span02", {{"key", "value"}}));
  EXPECT_EQ(3, span_data->count());

  release_span_data(context);
  EXPECT_EQ(nullptr, context);
  EXPECT_EQ(required_size(MmapSize::Medium), file_size(mmap_file));

  // Initialize a small memory layout from the existing medium file
  context = initialize_span_data(mmap_file, MmapSize::Small);

  // Get the expected 3 recovered spans, even though they were medium
  std::vector<Span> recovered = get_recovered_spans(context);
  EXPECT_EQ(3, recovered.size());
  EXPECT_EQ("span00", recovered[0].name());
  EXPECT_EQ("span01", recovered[1].name());
  EXPECT_EQ("span02", recovered[2].name());
  EXPECT_EQ("value", detail::get_attribute(recovered[2], "key"));

  EXPECT_EQ(0, get_mutable_span_data(context)->count());
  get_mutable_span_data(context)->add(simple_span(100));

  release_span_data(context);

  // Verify the file got truncated down to the required size for small
  EXPECT_EQ(required_size(MmapSize::Small), file_size(mmap_file));
}

TEST_F(InitializeTest, InitializeFromLargerSizeNoSpans) {
  // Set up the existing larger file but don't write any spans to it
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Medium);
  EXPECT_TRUE(get_recovered_spans(context).empty());

  release_span_data(context);
  EXPECT_EQ(nullptr, context);
  EXPECT_EQ(required_size(MmapSize::Medium), file_size(mmap_file));

  // Initialize a small memory layout from the existing medium file
  context = initialize_span_data(mmap_file, MmapSize::Small);
  EXPECT_TRUE(get_recovered_spans(context).empty());

  release_span_data(context);

  // Verify the file got truncated down to the required size for small
  EXPECT_EQ(required_size(MmapSize::Small), file_size(mmap_file));
}

TEST_F(InitializeTest, InitializeFromSparceData) {
  // Set up the existing file and make it sparce by adding a gap between spans
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);
  EXPECT_TRUE(get_recovered_spans(context).empty());

  std::shared_ptr<MutableSpanData> span_data = get_mutable_span_data(context);
  span_data->add(simple_span(200, "span00"));
  span_data->add(simple_span(201, "span01"));
  span_data->add(simple_span(202, "span02", {{"key", "value"}}));

  // Remove the span in the middle, making a gap
  span_data->remove(201);
  EXPECT_EQ(2, span_data->count());

  release_span_data(context);

  // Initialize a medium memory layout from the existing sparce file
  context = initialize_span_data(mmap_file, MmapSize::Small);

  // Get the expected 2 recovered spans, with the middle span removed
  std::vector<Span> recovered = get_recovered_spans(context);
  EXPECT_EQ(2, recovered.size());
  EXPECT_EQ("span00", recovered[0].name());
  EXPECT_EQ("span02", recovered[1].name());
  EXPECT_EQ("value", detail::get_attribute(recovered[1], "key"));

  release_span_data(context);
}

TEST_F(InitializeTest, InitializeFromSaturatedData) {
  // Set up the existing file and saturate it with max spans
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);
  EXPECT_TRUE(get_recovered_spans(context).empty());

  std::shared_ptr<MutableSpanData> span_data = get_mutable_span_data(context);
  std::uint64_t max_spans = detail::mmap_config<MmapSize::Small>.max_spans;
  for (int i = 1; i <= max_spans; ++i) {
    span_data->add(simple_span(i));
  }

  release_span_data(context);

  // Initialize from the existing saturated file
  context = initialize_span_data(mmap_file, MmapSize::Small);

  // Recover all max_spans spans
  std::vector<Span> recovered = get_recovered_spans(context);
  EXPECT_EQ(max_spans, recovered.size());
  EXPECT_EQ(1, recovered[0].span_id());
  EXPECT_EQ(max_spans, recovered[max_spans - 1].span_id());

  release_span_data(context);
}

TEST_F(InitializeTest, InitializeFromOverSaturatedData) {
  // Set up the existing file and saturate it with 150 spans
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Medium);
  EXPECT_TRUE(get_recovered_spans(context).empty());

  std::shared_ptr<MutableSpanData> span_data = get_mutable_span_data(context);
  for (int i = 1; i <= 150; ++i) {
    span_data->add(simple_span(i));
  }

  release_span_data(context);

  // Initialize a small memory layout from the existing over saturated file
  context = initialize_span_data(mmap_file, MmapSize::Small);

  // Recover all 150 spans, even though medium max spans is 128
  EXPECT_EQ(150, get_recovered_spans(context).size());

  release_span_data(context);
}

TEST_F(InitializeTest, InitializeBadPath) {
  unspecified_context_t* context =
      initialize_span_data("/usr/bin/su", MmapSize::Medium);

  EXPECT_EQ(nullptr, context);
}

TEST_F(InitializeTest, InitializeTwiceNoOp) {
  unspecified_context_t* context1 =
      initialize_span_data(mmap_file, MmapSize::Small);
  unspecified_context_t* context2 =
      initialize_span_data(mmap_file, MmapSize::Medium);

  EXPECT_NE(context1, context2);

  EXPECT_TRUE(get_recovered_spans(context1).empty());
  EXPECT_TRUE(get_recovered_spans(context2).empty());

  EXPECT_EQ(0, get_mutable_span_data(context1)->count());
  EXPECT_EQ(0, get_mutable_span_data(context2)->count());

  release_span_data(context1);
  release_span_data(context2);
}

class InitializeDeathTest : public InitializeTest {};

TEST_F(InitializeDeathTest, InitializeTwiceRecoveryDeathTest) {
  // Setup first initialize on this file
  unspecified_context_t* context1 =
      initialize_span_data(mmap_file, MmapSize::Small);

  EXPECT_TRUE(get_recovered_spans(context1).empty());

  std::shared_ptr<MutableSpanData> span_data1 = get_mutable_span_data(context1);
  span_data1->add(simple_span(100));
  span_data1->add(simple_span(101));

  // Setup second initialize on this same file
  unspecified_context_t* context2 =
      initialize_span_data(mmap_file, MmapSize::Small);

  EXPECT_NE(context1, context2);

  EXPECT_EQ(2, get_recovered_spans(context2).size());

  // Expected to die because span_data1 tries to remove span 100 from slot 0,
  // but context2 has already cleared slot 0
  EXPECT_DEATH({ span_data1->remove(100); }, "mark > 0");

  release_span_data(context1);
  release_span_data(context2);
}
