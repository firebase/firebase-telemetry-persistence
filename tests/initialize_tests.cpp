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

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "firebase/telemetry/persistence/detail/header.h"
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

bool file_exists(const char* path) {
  struct stat st;
  return stat(path, &st) != -1;
}

void remove_file(const char* path) { std::remove(path); }

void write_file(const char* path, const char* content) {
  std::ofstream(path) << content;
}

Header read_header(const char* path) {
  Header header{};
  std::ifstream in(path, std::ios::binary);
  in.read(reinterpret_cast<char *>(&header), sizeof (header));
  return header;
}

// Overwrites the header in place, leaving the spans after it untouched
void overwrite_header(const char* path, const Header& header) {
  std::fstream out(path, std::ios::binary | std::ios::in | std::ios::out);
  out.write(reinterpret_cast<const char *>(&header), sizeof (header));
}

class InitializeTest : public testing::Test {
protected:
  static constexpr char mmap_file[] = "data.mmap";
  static constexpr char other_mmap_file[] = "other.mmap";

  void SetUp() override { remove_files(); }
  void TearDown() override { remove_files(); }

  static void remove_files() {
    remove_file(mmap_file);
    remove_file(other_mmap_file);
  }

  // Leaves behind a file with three active spans, as a crashed session would
  static void persist_three_spans(MmapSize size) {
    unspecified_context_t* context = initialize_span_data(mmap_file, size);

    std::shared_ptr<MutableSpanData> span_data = get_mutable_span_data(context);
    span_data->add(simple_span(100, "span00"));
    span_data->add(simple_span(101, "span01"));
    span_data->add(simple_span(102, "span02", {{"key", "value"}}));
    EXPECT_EQ(3, span_data->count());

    release_span_data(context);
    EXPECT_EQ(nullptr, context);
  }
};

TEST_F(InitializeTest, InitializeFromNewFile) {
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);

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

  // Verify the header got reset
  EXPECT_EQ(0, get_mutable_span_data(context)->count());

  release_span_data(context);
}

TEST_F(InitializeTest, InitializeFromExistingFile) {
  persist_three_spans(MmapSize::Small);

  // Initialize from the existing file, without recovering it first
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);

  // Verify the header got reset
  std::shared_ptr<MutableSpanData> span_data = get_mutable_span_data(context);
  EXPECT_EQ(0, span_data->count());

  // Verify the span data got cleared, otherwise adding into the slots the old
  // spans were in would fail an assert check
  span_data->add(simple_span(200, "span10"));
  span_data->add(simple_span(201, "span11"));
  span_data->add(simple_span(202, "span12"));

  release_span_data(context);

  // Verify only the new session is left to recover
  std::vector<Span> recovered = recover_spans(mmap_file);
  ASSERT_EQ(3, recovered.size());
  EXPECT_EQ("span10", recovered[0].name());
  EXPECT_EQ("span11", recovered[1].name());
  EXPECT_EQ("span12", recovered[2].name());

  EXPECT_EQ(required_size(MmapSize::Small), file_size(mmap_file));
}

TEST_F(InitializeTest, InitializeFromSmallerSize) {
  persist_three_spans(MmapSize::Small);
  EXPECT_EQ(required_size(MmapSize::Small), file_size(mmap_file));

  // Initialize a medium memory layout from the existing small file
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Medium);

  EXPECT_EQ(0, get_mutable_span_data(context)->count());
  get_mutable_span_data(context)->add(simple_span(200));

  release_span_data(context);

  // Verify the file got expanded to the required size for medium
  EXPECT_EQ(required_size(MmapSize::Medium), file_size(mmap_file));

  // Verify the small spans were not carried over into the medium layout
  EXPECT_EQ(1, recover_spans(mmap_file).size());
}

TEST_F(InitializeTest, InitializeFromLargerSize) {
  persist_three_spans(MmapSize::Medium);
  EXPECT_EQ(required_size(MmapSize::Medium), file_size(mmap_file));

  // Initialize a small memory layout from the existing medium file
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);

  EXPECT_EQ(0, get_mutable_span_data(context)->count());
  get_mutable_span_data(context)->add(simple_span(200));

  release_span_data(context);

  // Verify the file got truncated down to the required size for small
  EXPECT_EQ(required_size(MmapSize::Small), file_size(mmap_file));

  // Verify the medium spans were not carried over into the small layout
  EXPECT_EQ(1, recover_spans(mmap_file).size());
}

TEST_F(InitializeTest, InitializeBadPath) {
  unspecified_context_t* context =
      initialize_span_data("no_such_dir/data.mmap", MmapSize::Medium);

  EXPECT_EQ(nullptr, context);
}

class RecoverSpansTest : public InitializeTest {};

TEST_F(RecoverSpansTest, RecoverFromMissingFile) {
  EXPECT_TRUE(recover_spans(mmap_file).empty());

  // Verify recovering did not create the file
  EXPECT_FALSE(file_exists(mmap_file));
}

TEST_F(RecoverSpansTest, RecoverFromCorruptFile) {
  write_file(mmap_file, "CRSH and some junk data \01 \02 \03 \04 whatevs");

  EXPECT_TRUE(recover_spans(mmap_file).empty());
}

TEST_F(RecoverSpansTest, RecoverFromLargeCorruptFile) {
  // As large as a real file, so it is the header that gets it rejected
  std::string data(required_size(MmapSize::Small), 'A');
  write_file(mmap_file, data.c_str());

  EXPECT_TRUE(recover_spans(mmap_file).empty());

  // Verify recovering did not resize or reset the corrupt file
  EXPECT_EQ(data.size(), file_size(mmap_file));
  EXPECT_EQ(0x4141414141414141ULL, read_header(mmap_file).signature);
}

TEST_F(RecoverSpansTest, RecoverFromInvalidHeaderSignature) {
  persist_three_spans(MmapSize::Small);
  ASSERT_EQ(3, recover_spans(mmap_file).size());

  Header header = read_header(mmap_file);
  header.signature = 0xBAD00BADULL;
  overwrite_header(mmap_file, header);

  EXPECT_TRUE(recover_spans(mmap_file).empty());
}

TEST_F(RecoverSpansTest, RecoverFromInvalidHeaderVersion) {
  persist_three_spans(MmapSize::Small);
  ASSERT_EQ(3, recover_spans(mmap_file).size());

  Header header = read_header(mmap_file);
  header.version = 9999ULL;  // Unsupported future version
  overwrite_header(mmap_file, header);

  EXPECT_TRUE(recover_spans(mmap_file).empty());
}

TEST_F(RecoverSpansTest, RecoverFromInvalidMmapSizeEnum) {
  persist_three_spans(MmapSize::Small);
  ASSERT_EQ(3, recover_spans(mmap_file).size());

  Header header = read_header(mmap_file);
  header.mmap_size = static_cast<MmapSize>(255);  // Invalid enum value
  overwrite_header(mmap_file, header);

  EXPECT_TRUE(recover_spans(mmap_file).empty());
}

TEST_F(RecoverSpansTest, RecoverFromTruncatedLayoutSize) {
  persist_three_spans(MmapSize::Small);
  ASSERT_EQ(3, recover_spans(mmap_file).size());

  // Claim a medium layout in a file only big enough for a small one
  Header header = read_header(mmap_file);
  header.mmap_size = MmapSize::Medium;
  overwrite_header(mmap_file, header);
  ASSERT_LT(file_size(mmap_file), get_memory_layout_size(MmapSize::Medium));

  EXPECT_TRUE(recover_spans(mmap_file).empty());
}

TEST_F(RecoverSpansTest, RecoverFromZeroActiveSpans) {
  persist_three_spans(MmapSize::Small);
  ASSERT_EQ(3, recover_spans(mmap_file).size());

  // Zero the active count while the spans are still marked
  Header header = read_header(mmap_file);
  header.active_span_count = 0;
  overwrite_header(mmap_file, header);

  EXPECT_TRUE(recover_spans(mmap_file).empty());
}

TEST_F(RecoverSpansTest, RecoverFromOversizedActiveSpanCount) {
  persist_three_spans(MmapSize::Small);

  // Claim more active spans than the layout has slots
  Header header = read_header(mmap_file);
  header.active_span_count = UINT64_MAX;
  overwrite_header(mmap_file, header);

  // Verify the marked spans still recover, without reading past the slots
  std::vector<Span> recovered = recover_spans(mmap_file);
  ASSERT_EQ(3, recovered.size());
  EXPECT_EQ("span00", recovered[0].name());
  EXPECT_EQ("span01", recovered[1].name());
  EXPECT_EQ("span02", recovered[2].name());
}

TEST_F(RecoverSpansTest, RecoverFromInitializedFile) {
  // Set up a file that was initialized but never had a span added
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);
  release_span_data(context);

  EXPECT_TRUE(recover_spans(mmap_file).empty());
}

TEST_F(RecoverSpansTest, RecoverFromExistingFile) {
  persist_three_spans(MmapSize::Small);

  // Get the expected 3 recovered spans
  std::vector<Span> recovered = recover_spans(mmap_file);
  ASSERT_EQ(3, recovered.size());
  EXPECT_EQ("span00", recovered[0].name());
  EXPECT_EQ("span01", recovered[1].name());
  EXPECT_EQ("span02", recovered[2].name());
  EXPECT_EQ("value", detail::get_attribute(recovered[2], "key"));

  // Verify recovering left the file as it was, so it can be recovered again
  EXPECT_EQ(3, recover_spans(mmap_file).size());
  EXPECT_EQ(required_size(MmapSize::Small), file_size(mmap_file));
}

TEST_F(RecoverSpansTest, RecoverThenInitialize) {
  persist_three_spans(MmapSize::Small);
  EXPECT_EQ(3, recover_spans(mmap_file).size());

  // Reuse the recovered file for a new session
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);

  // Verify initializing emptied the file, so recovering it now is too late
  EXPECT_TRUE(recover_spans(mmap_file).empty());

  release_span_data(context);
}

TEST_F(RecoverSpansTest, RecoverFromSeparateFile) {
  persist_three_spans(MmapSize::Small);

  // Start a new session in a different file from the one left to recover
  unspecified_context_t* context =
      initialize_span_data(other_mmap_file, MmapSize::Small);
  get_mutable_span_data(context)->add(simple_span(200, "span10"));

  // Verify the new session did not disturb the file left to recover
  std::vector<Span> recovered = recover_spans(mmap_file);
  ASSERT_EQ(3, recovered.size());
  EXPECT_EQ("span00", recovered[0].name());
  EXPECT_EQ("span01", recovered[1].name());
  EXPECT_EQ("span02", recovered[2].name());

  // Verify recovering did not disturb the new session
  EXPECT_EQ(1, get_mutable_span_data(context)->count());

  release_span_data(context);
}

TEST_F(RecoverSpansTest, RecoverFromMediumSize) {
  // Set up a medium file with 150 spans, more than a small layout can hold
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Medium);

  std::shared_ptr<MutableSpanData> span_data = get_mutable_span_data(context);
  for (int i = 1; i <= 150; ++i) {
    span_data->add(simple_span(i));
  }

  release_span_data(context);

  // Recover all 150 spans, because the layout comes from the file's header
  EXPECT_EQ(150, recover_spans(mmap_file).size());
}

TEST_F(RecoverSpansTest, RecoverFromSparseData) {
  // Set up the existing file and make it sparse by adding a gap between spans
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);

  std::shared_ptr<MutableSpanData> span_data = get_mutable_span_data(context);
  span_data->add(simple_span(200, "span00"));
  span_data->add(simple_span(201, "span01"));
  span_data->add(simple_span(202, "span02", {{"key", "value"}}));

  // Remove the span in the middle, making a gap
  span_data->remove(201);
  EXPECT_EQ(2, span_data->count());

  release_span_data(context);

  // Get the expected 2 recovered spans, with the middle span removed
  std::vector<Span> recovered = recover_spans(mmap_file);
  ASSERT_EQ(2, recovered.size());
  EXPECT_EQ("span00", recovered[0].name());
  EXPECT_EQ("span02", recovered[1].name());
  EXPECT_EQ("value", detail::get_attribute(recovered[1], "key"));
}

TEST_F(RecoverSpansTest, RecoverFromSaturatedData) {
  // Set up the existing file and saturate it with max spans
  unspecified_context_t* context =
      initialize_span_data(mmap_file, MmapSize::Small);

  std::shared_ptr<MutableSpanData> span_data = get_mutable_span_data(context);
  std::uint64_t max_spans = detail::mmap_config<MmapSize::Small>.max_spans;
  for (int i = 1; i <= max_spans; ++i) {
    span_data->add(simple_span(i));
  }

  release_span_data(context);

  // Recover all max_spans spans
  std::vector<Span> recovered = recover_spans(mmap_file);
  ASSERT_EQ(max_spans, recovered.size());
  EXPECT_EQ(1, recovered[0].span_id());
  EXPECT_EQ(max_spans, recovered[max_spans - 1].span_id());
}

class InitializeDeathTest : public InitializeTest {};

TEST_F(InitializeDeathTest, InitializeTwiceDeathTest) {
  // Setup first initialize on this file
  unspecified_context_t* context1 =
      initialize_span_data(mmap_file, MmapSize::Small);

  std::shared_ptr<MutableSpanData> span_data1 = get_mutable_span_data(context1);
  span_data1->add(simple_span(100));
  span_data1->add(simple_span(101));

  // Verify spans can recover while the first context is still using the file
  EXPECT_EQ(2, recover_spans(mmap_file).size());

  // Setup second initialize on this same file
  unspecified_context_t* context2 =
      initialize_span_data(mmap_file, MmapSize::Small);

  EXPECT_NE(context1, context2);

  // Expected to die because span_data1 tries to remove span 100 from slot 0,
  // but context2 has already cleared slot 0
  EXPECT_DEATH({ span_data1->remove(100); }, "mark > 0");

  release_span_data(context1);
  release_span_data(context2);
}
