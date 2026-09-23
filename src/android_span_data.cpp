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

#include <jni.h>

#include <algorithm>
#include <iterator>
#include <string_view>
#include <utility>
#include <vector>

#include "firebase/telemetry/persistence/detail/copy_string.h"
#include "firebase/telemetry/persistence/initialize.h"
#include "firebase/telemetry/persistence/span.h"

namespace firebase::telemetry::persistence {
namespace {

jclass g_span_class = nullptr;
jclass g_string_class = nullptr;
jmethodID g_create_span_mid = nullptr;

constexpr std::size_t max_span_name_len = 64;
constexpr std::size_t max_attribute_key_len = 64;
constexpr std::size_t max_attribute_val_len = 128;
constexpr std::size_t max_file_path_len = 4096;

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
  char buf[MaxLen];
  detail::copy_string(str, buf);
  return env->NewStringUTF(buf);
}

std::string jstring_to_string(JNIEnv* env, jstring src, std::size_t max_len) {
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

jint cache_jni_globals(JNIEnv* env) {
  jclass span_class =
      env->FindClass("com/google/firebase/crashlytics/telemetry/Span");
  if (span_class == nullptr) {
    return JNI_ERR;
  }
  g_span_class = reinterpret_cast<jclass>(env->NewGlobalRef(span_class));
  env->DeleteLocalRef(span_class);

  jclass string_class = env->FindClass("java/lang/String");
  if (string_class == nullptr) {
    return JNI_ERR;
  }
  g_string_class = reinterpret_cast<jclass>(env->NewGlobalRef(string_class));
  env->DeleteLocalRef(string_class);

  g_create_span_mid =
      env->GetStaticMethodID(g_span_class, "createRecoveredSpan",
                             "(JJJJJLjava/lang/String;[Ljava/lang/String;)Lcom/"
                             "google/firebase/crashlytics/telemetry/Span;");
  return g_create_span_mid != nullptr ? JNI_OK : JNI_ERR;
}

void release_jni_globals(JNIEnv* env) {
  if (g_span_class != nullptr) {
    env->DeleteGlobalRef(g_span_class);
    g_span_class = nullptr;
  }
  if (g_string_class != nullptr) {
    env->DeleteGlobalRef(g_string_class);
    g_string_class = nullptr;
  }
}

jobject create_jni_span_object(JNIEnv* env, const Span& span) {
  if (g_span_class == nullptr || g_string_class == nullptr ||
      g_create_span_mid == nullptr) {
    return nullptr;
  }

  jstring name = create_bounded_jstring<max_span_name_len>(env, span.name());
  if (name == nullptr) {
    return nullptr;
  }

  jsize total_elements = static_cast<jsize>(span.attributes().size() * 2);
  jobjectArray attributes =
      env->NewObjectArray(total_elements, g_string_class, nullptr);
  if (attributes == nullptr) {
    env->DeleteLocalRef(name);
    return nullptr;
  }

  jsize index = 0;
  for (const auto& [key, val] : span.attributes()) {
    jstring jkey = create_bounded_jstring<max_attribute_key_len>(env, key);
    jstring jval = create_bounded_jstring<max_attribute_val_len>(env, val);

    if (jkey != nullptr && jval != nullptr) {
      env->SetObjectArrayElement(attributes, index, jkey);
      env->SetObjectArrayElement(attributes, index + 1, jval);
      index += 2;
    }

    if (jkey != nullptr) {
      env->DeleteLocalRef(jkey);
    }
    if (jval != nullptr) {
      env->DeleteLocalRef(jval);
    }
  }

  jobject recovered_span_obj = env->CallStaticObjectMethod(
      g_span_class, g_create_span_mid, span.trace_id().high,
      span.trace_id().low, span.span_id(), span.parent_span_id(),
      span.start_time(), name, attributes);

  env->DeleteLocalRef(name);
  env->DeleteLocalRef(attributes);

  return recovered_span_obj;
}

std::vector<std::pair<std::string, std::string>> parse_jni_attributes(
    JNIEnv* env, jobjectArray attributes) {
  std::vector<std::pair<std::string, std::string>> attrs;
  if (attributes == nullptr) {
    return attrs;
  }

  jsize len = env->GetArrayLength(attributes);
  attrs.reserve(len / 2);

  for (jsize i = 1; i < len; i += 2) {
    jstring jkey =
        static_cast<jstring>(env->GetObjectArrayElement(attributes, i - 1));
    jstring jval =
        static_cast<jstring>(env->GetObjectArrayElement(attributes, i));
    if (jkey != nullptr && jval != nullptr) {
      attrs.emplace_back(jstring_to_string(env, jkey, max_attribute_key_len),
                         jstring_to_string(env, jval, max_attribute_val_len));
    }
    if (jkey != nullptr) {
      env->DeleteLocalRef(jkey);
    }
    if (jval != nullptr) {
      env->DeleteLocalRef(jval);
    }
  }
  return attrs;
}

jobjectArray create_jni_span_objects_array(JNIEnv* env,
                                           const std::vector<Span>& spans) {
  if (g_span_class == nullptr) {
    return nullptr;
  }

  std::vector<jobject> valid_spans;
  valid_spans.reserve(spans.size());

  for (const Span& span : spans) {
    jobject jspan = create_jni_span_object(env, span);
    if (jspan != nullptr) {
      valid_spans.push_back(jspan);
    }
  }

  jsize size = static_cast<jsize>(valid_spans.size());
  jobjectArray spans_array = env->NewObjectArray(size, g_span_class, nullptr);

  if (spans_array == nullptr) {
    return nullptr;
  }

  for (jsize i = 0; i < size; ++i) {
    env->SetObjectArrayElement(spans_array, i, valid_spans[i]);
    env->DeleteLocalRef(valid_spans[i]);
  }

  return spans_array;
}

// Lifecycle & Initialization Native Implementations -------------------------

jlong JNICALL initialize_native(JNIEnv* env, jclass /* clazz */,
                                jstring file_path, jint size_ordinal) {
  if (file_path == nullptr) {
    return 0;
  }

  MmapSize mmap_size = static_cast<MmapSize>(size_ordinal);
  std::string path = jstring_to_string(env, file_path, max_file_path_len);

  unspecified_context_t* context = initialize_span_data(path, mmap_size);

  return reinterpret_cast<jlong>(context);
}

jobjectArray JNICALL recover_spans_native(JNIEnv* env, jclass /* clazz */,
                                          jlong context_ptr) {
  unspecified_context_t* context =
      reinterpret_cast<unspecified_context_t *>(context_ptr);
  std::vector<Span> recovered_spans = get_recovered_spans(context);

  return create_jni_span_objects_array(env, recovered_spans);
}

void JNICALL shutdown_native(JNIEnv* /* env */, jclass /* clazz */,
                             jlong context_ptr) {
  unspecified_context_t* context =
      reinterpret_cast<unspecified_context_t *>(context_ptr);
  release_span_data(context);
}

// Mutable Context Native Implementations -------------------------------------

void JNICALL add_span(JNIEnv* env, jobject /* thiz */, jlong context_ptr,
                      jlong trace_id_high, jlong trace_id_low, jlong span_id,
                      jlong parent_span_id, jlong start_time, jstring name,
                      jobjectArray attributes) {
  unspecified_context_t* context =
      reinterpret_cast<unspecified_context_t *>(context_ptr);
  MutableSpanData* mutable_span_data = get_mutable_span_data(context).get();
  if (mutable_span_data == nullptr) {
    return;
  }

  mutable_span_data->add(
      Span(TraceId{static_cast<std::uint64_t>(trace_id_high),
                   static_cast<std::uint64_t>(trace_id_low)},
           static_cast<std::uint64_t>(span_id),
           static_cast<std::uint64_t>(parent_span_id),
           static_cast<std::uint64_t>(start_time), 0,
           jstring_to_string(env, name, max_span_name_len),
           parse_jni_attributes(env, attributes)));
}

void JNICALL end_span(JNIEnv* /* env */, jobject /* thiz */, jlong context_ptr,
                      jlong span_id) {
  unspecified_context_t* context =
      reinterpret_cast<unspecified_context_t *>(context_ptr);
  MutableSpanData* mutable_span_data = get_mutable_span_data(context).get();
  if (mutable_span_data == nullptr) {
    return;
  }

  mutable_span_data->end(static_cast<std::uint64_t>(span_id));
}

void JNICALL set_attribute_on_span(JNIEnv* env, jobject /* thiz */,
                                   jlong context_ptr, jlong span_id,
                                   jstring key, jstring value) {
  unspecified_context_t* context =
      reinterpret_cast<unspecified_context_t *>(context_ptr);
  MutableSpanData* mutable_span_data = get_mutable_span_data(context).get();
  if (mutable_span_data == nullptr) {
    return;
  }

  std::uint64_t native_span_id = static_cast<std::uint64_t>(span_id);
  mutable_span_data->set_attribute_on_span(
      native_span_id, jstring_to_string(env, key, max_attribute_key_len),
      jstring_to_string(env, value, max_attribute_val_len));
}

jlong JNICALL count_mutable_spans(JNIEnv* /* env */, jobject /* thiz */,
                                  jlong context_ptr) {
  unspecified_context_t* context =
      reinterpret_cast<unspecified_context_t *>(context_ptr);
  MutableSpanData* mutable_span_data = get_mutable_span_data(context).get();
  if (mutable_span_data == nullptr) {
    return 0;
  }

  return static_cast<jlong>(mutable_span_data->count());
}

const JNINativeMethod telemetry_methods[] = {
    {"initializeNative", "(Ljava/lang/String;I)J",
     reinterpret_cast<void *>(initialize_native)},
    {"recoverSpansNative",
     "(J)[Lcom/google/firebase/crashlytics/telemetry/Span;",
     reinterpret_cast<void *>(recover_spans_native)},
    {"shutdownNative", "(J)V", reinterpret_cast<void *>(shutdown_native)},
};

const JNINativeMethod mutable_methods[] = {
    {"addSpanNative", "(JJJJJJLjava/lang/String;[Ljava/lang/String;)V",
     reinterpret_cast<void *>(add_span)},
    {"endSpanNative", "(JJ)V", reinterpret_cast<void *>(end_span)},
    {"setAttributeOnSpanNative", "(JJLjava/lang/String;Ljava/lang/String;)V",
     reinterpret_cast<void *>(set_attribute_on_span)},
    {"countSpansNative", "(J)J", reinterpret_cast<void *>(count_mutable_spans)},
};

jint register_natives(JNIEnv* env) {
  if (cache_jni_globals(env) != JNI_OK) {
    return JNI_ERR;
  }

  // Native Method Table Registrations for TelemetryContext
  jclass telemetry_clazz = env->FindClass(
      "com/google/firebase/crashlytics/telemetry/TelemetryContext");
  if (telemetry_clazz == nullptr) {
    return JNI_ERR;
  }
  if (env->RegisterNatives(telemetry_clazz, telemetry_methods,
                           std::size(telemetry_methods)) != JNI_OK) {
    return JNI_ERR;
  }

  // Native Method Table Registrations for MutationContext
  jclass mutable_clazz = env->FindClass(
      "com/google/firebase/crashlytics/telemetry/MutationContext");
  if (mutable_clazz == nullptr) {
    return JNI_ERR;
  }
  if (env->RegisterNatives(mutable_clazz, mutable_methods,
                           std::size(mutable_methods)) != JNI_OK) {
    return JNI_ERR;
  }

  return JNI_OK;
}

}  // namespace
}  // namespace firebase::telemetry::persistence

// Explicit JNI Lifecycle & Cache Initializations -----------------------------

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /* reserved */) {
  JNIEnv* env = nullptr;
  if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
    return JNI_ERR;
  }

  return firebase::telemetry::persistence::register_natives(env) == JNI_OK
             ? JNI_VERSION_1_6
             : JNI_ERR;
}

extern "C" JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm,
                                               void* /* reserved */) {
  JNIEnv* env = nullptr;
  if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_OK) {
    firebase::telemetry::persistence::release_jni_globals(env);
  }
}
