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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_ANDROID_DETAIL_SPAN_JCLASS_CACHE_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_ANDROID_DETAIL_SPAN_JCLASS_CACHE_H__

#include <jni.h>

namespace firebase::telemetry::persistence::android::detail {

class SpanJClassCache {
public:
  SpanJClassCache() = default;
  explicit SpanJClassCache(JNIEnv* env);
  ~SpanJClassCache();

  SpanJClassCache(const SpanJClassCache& other) = delete;
  SpanJClassCache& operator=(const SpanJClassCache& other) = delete;

  SpanJClassCache(SpanJClassCache&& other);
  SpanJClassCache& operator=(SpanJClassCache&& other);

  bool is_initialized() const;

  jclass span_class() const { return span_class_; }
  jclass string_class() const { return string_class_; }
  jmethodID span_create() const { return span_create_; }

private:
  JavaVM* vm_ = nullptr;

  jclass span_class_ = nullptr;
  jclass string_class_ = nullptr;
  jmethodID span_create_ = nullptr;
};

}  // namespace firebase::telemetry::persistence::android::detail

#endif  //__FIREBASE_TELEMETRY_PERSISTENCE_ANDROID_DETAIL_SPAN_JCLASS_CACHE_H__
