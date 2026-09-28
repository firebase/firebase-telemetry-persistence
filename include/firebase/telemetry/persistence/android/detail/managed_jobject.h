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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_ANDROID_DETAIL_MANAGED_JOBJECT_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_ANDROID_DETAIL_MANAGED_JOBJECT_H__

#include <jni.h>

namespace firebase::telemetry::persistence::android::detail {
class ManagedJObject {
public:
  ManagedJObject() : env_(nullptr), value_(nullptr) {}

  // Takes ownership of `value` local ref
  explicit ManagedJObject(JNIEnv* env, jobject value)
      : env_(env), value_(value) {}

  ~ManagedJObject() {
    if (env_ != nullptr && value_ != nullptr) {
      env_->DeleteLocalRef(value_);
    }
  }

  ManagedJObject(const ManagedJObject& other) = delete;
  ManagedJObject& operator=(const ManagedJObject& other) = delete;

  ManagedJObject(ManagedJObject&& other)
      : env_(other.env_), value_(other.value_) {
    other.env_ = nullptr;
    other.value_ = nullptr;
  }

  ManagedJObject& operator=(ManagedJObject&& other) {
    if (this == &other) {
      return *this;
    }

    if (env_ != nullptr && value_ != nullptr) {
      env_->DeleteLocalRef(value_);
    }

    env_ = other.env_;
    value_ = other.value_;
    other.env_ = nullptr;
    other.value_ = nullptr;

    return *this;
  }

  explicit operator bool() const { return value_ != nullptr; }

  operator jobject() const { return value_; }

private:
  JNIEnv* env_;
  jobject value_;
};

}  // namespace firebase::telemetry::persistence::android::detail

#endif  //__FIREBASE_TELEMETRY_PERSISTENCE_ANDROID_DETAIL_MANAGED_JOBJECT_H__
