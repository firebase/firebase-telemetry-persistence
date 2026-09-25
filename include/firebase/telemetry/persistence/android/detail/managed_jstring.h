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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_MANAGED_JSTRING_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_MANAGED_JSTRING_H__

#include <jni.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

#include "firebase/telemetry/persistence/detail/copy_string.h"

namespace firebase::telemetry::persistence::android::detail {
template <std::size_t MaxLen>
jstring create_bounded_jstring(JNIEnv* env, std::string_view str) {
  if (env == nullptr) {
    return nullptr;
  }

  // If string is within safe bounds and null-terminated, pass directly
  if (str.size() < MaxLen && str.data()[str.size()] == '\0') {
    return env->NewStringUTF(str.data());
  }

  // String exceeds MaxLen or lacks null termination; copy & truncate safely
  char buf[MaxLen] = {};
  persistence::detail::copy_string(str, buf);
  return env->NewStringUTF(buf);
}

inline std::string jstring_to_string(JNIEnv* env, jstring src,
                                     std::size_t max_len) {
  if (src == nullptr || env == nullptr) {
    return "";
  }

  const char* utf_chars = env->GetStringUTFChars(src, nullptr);
  if (utf_chars == nullptr) {
    return "";
  }

  jsize len = env->GetStringUTFLength(src);
  std::size_t safe_len = std::min(static_cast<std::size_t>(len), max_len);
  std::string result(utf_chars, safe_len);
  env->ReleaseStringUTFChars(src, utf_chars);
  return result;
}

template <std::size_t MaxLen>
class ManagedJString {
public:
  ManagedJString(JNIEnv* env, std::string_view value)
      : env_(env), value_(nullptr) {
    if (env_ != nullptr) {
      value_ = create_bounded_jstring<MaxLen>(env, value);
    }
  }

  // Takes ownership of `value` local ref
  ManagedJString(JNIEnv* env, jstring value) : env_(env), value_(value) {}

  // Takes ownership of `value`, which must be a jstring local ref
  ManagedJString(JNIEnv* env, jobject value)
      : ManagedJString(env, static_cast<jstring>(value)) {}

  ~ManagedJString() {
    if (env_ != nullptr && value_ != nullptr) {
      env_->DeleteLocalRef(value_);
    }
  }

  ManagedJString(const ManagedJString& other) = delete;
  ManagedJString(ManagedJString&& other) = delete;

  ManagedJString& operator=(const ManagedJString& other) = delete;
  ManagedJString& operator=(ManagedJString&& other) = delete;

  explicit operator bool() const { return value_ != nullptr; }

  operator jstring() const { return value_; }

  operator std::string() const {
    return jstring_to_string(env_, value_, MaxLen);
  }

private:
  JNIEnv* env_;
  jstring value_;
};

}  // namespace firebase::telemetry::persistence::android::detail

#endif  //__FIREBASE_TELEMETRY_PERSISTENCE_MANAGED_JSTRING_H__
