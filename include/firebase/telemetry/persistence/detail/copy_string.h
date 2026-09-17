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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_COPY_STRING_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_COPY_STRING_H__

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string_view>

namespace firebase::telemetry::persistence::detail {

constexpr bool is_utf8_continuation_byte(char c) {
  // Check if c is an utf8 continuation byte (starts with bits 10)
  return (static_cast<unsigned char>(c) & 0xc0) == 0x80;
}

template <std::size_t N>
constexpr std::size_t get_utf8_length(std::string_view src) {
  static_assert(N > 0);  // Cannot have a size_t of -1
  std::size_t length = std::min(src.size(), N - 1);

  // Don't truncate on a multibyte utf8 char to avoid corrupting the string
  if (length < src.length()) {
    while (length > 0 && is_utf8_continuation_byte(src[length])) {
      --length;
    }
  }
  return length;
}

template <std::size_t N>
constexpr bool key_equals(std::string_view untruncated, char (&buffer)[N]) {
  std::size_t length = get_utf8_length<N>(untruncated);

  return std::string_view(buffer, length) == untruncated.substr(0, length) &&
         buffer[length] == '\0';
}

template <std::size_t N>
void copy_string(std::string_view src, char (&dest)[N]) {
  // Zero out the destination string and copy the source string into the
  // destination string. This ensures the destination string is always
  // null-terminated and recoverable even if a crash occurs within this
  // function
  std::size_t length = get_utf8_length<N>(src);
  std::memset(dest, 0, N);
  std::memcpy(dest, src.data(), length);
}

template <std::size_t N1, std::size_t N2>
void copy_string2(std::string_view src1, char (&dest1)[N1],
                  std::string_view src2, char (&dest2)[N2]) {
  // Zero out both destination strings and then copy the sources into the
  // destinations. This ensures both destination strings are always
  // null-terminated and recoverable even if a crash occurs in this function.
  //
  // The difference between this function and calling [copy_string] twice is
  // this function will remove both old values before the copies start
  std::size_t length1 = get_utf8_length<N1>(src1);
  std::size_t length2 = get_utf8_length<N2>(src2);
  std::memset(dest1, 0, N1);
  std::memset(dest2, 0, N2);
  std::memcpy(dest1, src1.data(), length1);
  std::memcpy(dest2, src2.data(), length2);
}

}  // namespace firebase::telemetry::persistence::detail

#endif  //__FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_COPY_STRING_H__
