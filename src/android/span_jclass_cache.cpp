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

#include "firebase/telemetry/persistence/android/detail/span_jclass_cache.h"

namespace firebase::telemetry::persistence::android::detail {
namespace {
void release_global(JNIEnv* env, jobject ref) {
  if (env != nullptr && ref != nullptr) {
    env->DeleteGlobalRef(ref);
  }
}
}  // namespace

SpanJClassCache::SpanJClassCache() : env_(nullptr) {}

SpanJClassCache::SpanJClassCache(JNIEnv* env) : env_(env) {
  if (env_ == nullptr) {
    return;
  }

  jclass span_class =
      env->FindClass("com/google/firebase/crashlytics/telemetry/Span");
  if (span_class == nullptr) {
    return;
  }
  span_class_ = static_cast<jclass>(env->NewGlobalRef(span_class));
  env->DeleteLocalRef(span_class);

  jclass string_class = env->FindClass("java/lang/String");
  if (string_class == nullptr) {
    return;
  }
  string_class_ = static_cast<jclass>(env->NewGlobalRef(string_class));
  env->DeleteLocalRef(string_class);

  span_create_ = env->GetStaticMethodID(
      span_class_, "createRecoveredSpan",
      "(JJJJJLjava/lang/String;[Ljava/lang/String;)L"
      "com/google/firebase/crashlytics/telemetry/Span;");
}

SpanJClassCache::~SpanJClassCache() {
  release_global(env_, span_class_);
  release_global(env_, string_class_);
}

SpanJClassCache::SpanJClassCache(SpanJClassCache&& other)
    : env_(other.env_),
      span_class_(other.span_class_),
      string_class_(other.string_class_),
      span_create_(other.span_create_) {
  other.env_ = nullptr;
  other.span_class_ = nullptr;
  other.string_class_ = nullptr;
  other.span_create_ = nullptr;
}

SpanJClassCache& SpanJClassCache::operator=(SpanJClassCache&& other) {
  if (this == &other) {
    return *this;
  }

  release_global(env_, span_class_);
  release_global(env_, string_class_);

  env_ = other.env_;
  span_class_ = other.span_class_;
  string_class_ = other.string_class_;
  span_create_ = other.span_create_;
  other.env_ = nullptr;
  other.span_class_ = nullptr;
  other.string_class_ = nullptr;
  other.span_create_ = nullptr;
  return *this;
}

bool SpanJClassCache::is_initialized() const {
  return env_ != nullptr &&
         span_class_ != nullptr &&
         string_class_ != nullptr &&
         span_create_ != nullptr;
}

}  // namespace firebase::telemetry::persistence::android::detail
