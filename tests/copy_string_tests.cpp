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

#include <string>

#include "firebase/telemetry/persistence/detail/copy_string.h"

using namespace firebase::telemetry::persistence::detail;

TEST(CopyStringTest, CopyString) {
  char dest[16];
  std::string src = "hello world";

  copy_string(src, dest);
  EXPECT_STREQ("hello world", dest);

  // Verify it's null-terminated and zeroed out
  for (int i = 11; i < 16; ++i) {
    EXPECT_EQ(0, dest[i]);
  }
}

TEST(CopyStringTest, CopyCString) {
  char dest[16];
  const char src[] = "hello world";

  copy_string(src, dest);
  EXPECT_STREQ(src, dest);
}

TEST(CopyStringTest, CopyString2) {
  char dest1[16];
  char dest2[17];

  copy_string2("hello", dest1, "bonjour", dest2);
  EXPECT_STREQ("hello", dest1);
  EXPECT_STREQ("bonjour", dest2);
}

TEST(CopyStringTest, CopyMaxString) {
  char dest[12];
  std::string src = "hello world";

  copy_string(src, dest);
  EXPECT_EQ(src, dest);
}

TEST(CopyStringTest, CopyMaxCString) {
  char dest[12];
  const char src[] = "hello world";

  copy_string(src, dest);
  EXPECT_STREQ(src, dest);
}

TEST(CopyStringTest, CopyMaxString2) {
  char dest1[6];
  char dest2[8];

  copy_string2("hello", dest1, "bonjour", dest2);
  EXPECT_STREQ("hello", dest1);
  EXPECT_STREQ("bonjour", dest2);
}

TEST(CopyStringTest, CopyStringTruncated) {
  char dest[6];
  std::string src = "hello world";

  copy_string(src, dest);
  EXPECT_STREQ("hello", dest);
  EXPECT_TRUE(key_equals(src, dest));
}

TEST(CopyStringTest, CopyCStringTruncated) {
  char dest[6];
  const char src[] = "hello world";

  copy_string(src, dest);
  EXPECT_STREQ("hello", dest);
  EXPECT_TRUE(key_equals(src, dest));
}

TEST(CopyStringTest, CopyString2Truncated) {
  char dest1[6];
  char dest2[8];

  copy_string2("hello world", dest1, "bonjour le monde", dest2);
  EXPECT_STREQ("hello", dest1);
  EXPECT_STREQ("bonjour", dest2);
  EXPECT_TRUE(key_equals("hello world", dest1));
  EXPECT_TRUE(key_equals("bonjour le monde", dest2));
}

TEST(CopyStringTest, CopyStringEmpty) {
  char dest[16];

  copy_string("", dest);
  EXPECT_STREQ("", dest);
}

TEST(CopyStringTest, CopyString2Empty) {
  char dest1[16];
  char dest2[15];

  copy_string2("", dest1, "", dest2);
  EXPECT_STREQ("", dest1);
  EXPECT_STREQ("", dest2);
}

TEST(CopyStringTest, EqualsTruncated) {
  char dest[16] = "hello";

  EXPECT_FALSE(key_equals("hell", dest));
  EXPECT_TRUE(key_equals("hello", dest));
  EXPECT_FALSE(key_equals("hello world", dest));
}

TEST(CopyStringTest, EqualsTruncatedMax) {
  char dest[] = "plane";

  EXPECT_FALSE(key_equals("plan", dest));
  EXPECT_TRUE(key_equals("plane", dest));
  EXPECT_TRUE(key_equals("planet", dest));
  EXPECT_TRUE(key_equals("planets", dest));
  EXPECT_FALSE(key_equals("pluto", dest));
}

TEST(CopyStringTest, TruncateContinuationChar) {
  char dest[6];
  std::string src = "hell\xD0\xB4";

  copy_string(src, dest);
  EXPECT_STREQ("hell", dest);
  EXPECT_EQ(0, dest[4]);
  EXPECT_EQ(0, dest[5]);

  EXPECT_TRUE(key_equals(src, dest));
}

TEST(CopyStringTest, TruncateContinuationChar2) {
  char dest1[6];
  char dest2[8];
  std::string src1 = "hell\xD0\xB4";
  std::string src2 = "hello \xE2\x82\xAC";

  copy_string2(src1, dest1, src2, dest2);
  EXPECT_STREQ("hell", dest1);
  EXPECT_STREQ("hello ", dest2);
  EXPECT_TRUE(key_equals(src1, dest1));
  EXPECT_TRUE(key_equals(src2, dest2));
}
