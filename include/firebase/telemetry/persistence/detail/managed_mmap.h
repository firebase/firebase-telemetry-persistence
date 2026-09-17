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

#ifndef __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_MANAGED_MMAP_POINTER_H__
#define __FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_MANAGED_MMAP_POINTER_H__

#include <sys/mman.h>

#include <cassert>
#include <cstddef>

namespace firebase::telemetry::persistence::detail {

template <typename T>
class ManagedMmap {
public:
  template <typename U>
  friend class ManagedMmap;

  ManagedMmap(void* addr, std::size_t required_size, int protection,
              int flags, int fd, off_t offset);
  ~ManagedMmap();

  ManagedMmap(const ManagedMmap&) = delete;
  ManagedMmap& operator=(const ManagedMmap&) = delete;

  ManagedMmap(ManagedMmap&& other);
  ManagedMmap& operator=(ManagedMmap&& other);

  template <typename U>
  ManagedMmap(ManagedMmap<U>&& other);

  template <typename U>
  ManagedMmap& operator=(ManagedMmap<U>&& other);

  T* operator->();
  const T* operator->() const;

  bool is_initialized() const;

private:
  T* mmap_ptr_;
  std::size_t required_size_;
};

// Implementation --------------------------------------------------------------

template <typename T>
ManagedMmap<T>::ManagedMmap(void* addr, std::size_t required_size,
                            int protection, int flags, int fd, off_t offset)
    : mmap_ptr_(nullptr), required_size_(0) {
  assert(required_size == 0 || sizeof (T) <= required_size);
  void* raw_mmap_ptr = mmap(addr, required_size, protection, flags, fd, offset);
  if (raw_mmap_ptr != MAP_FAILED) {
    mmap_ptr_ = reinterpret_cast<T *>(raw_mmap_ptr);
    required_size_ = required_size;
  }
}

template <typename T>
ManagedMmap<T>::~ManagedMmap() {
  if (mmap_ptr_ != nullptr && required_size_ > 0) {
    munmap(mmap_ptr_, required_size_);
  }
}

template <typename T>
ManagedMmap<T>::ManagedMmap(ManagedMmap&& other)
    : mmap_ptr_(other.mmap_ptr_), required_size_(other.required_size_) {
  other.mmap_ptr_ = nullptr;
  other.required_size_ = 0;
}

template <typename T>
ManagedMmap<T>& ManagedMmap<T>::operator=(ManagedMmap&& other) {
  if (this != &other) {
    if (mmap_ptr_ != nullptr && required_size_ > 0) {
      munmap(mmap_ptr_, required_size_);
    }
    mmap_ptr_ = other.mmap_ptr_;
    required_size_ = other.required_size_;
    other.mmap_ptr_ = nullptr;
    other.required_size_ = 0;
  }
  return *this;
}

template <typename T>
template <typename U>
ManagedMmap<T>::ManagedMmap(ManagedMmap<U>&& other)
    : mmap_ptr_(reinterpret_cast<T *>(other.mmap_ptr_)),
      required_size_(other.required_size_) {
  assert(required_size_ == 0 || sizeof (T) <= required_size_);
  other.mmap_ptr_ = nullptr;
  other.required_size_ = 0;
}

template <typename T>
template <typename U>
ManagedMmap<T>& ManagedMmap<T>::operator=(ManagedMmap<U>&& other) {
  if (mmap_ptr_ != nullptr && required_size_ > 0) {
    munmap(mmap_ptr_, required_size_);
  }
  mmap_ptr_ = reinterpret_cast<T *>(other.mmap_ptr_);
  required_size_ = other.required_size_;
  assert(required_size_ == 0 || sizeof (T) <= required_size_);
  other.mmap_ptr_ = nullptr;
  other.required_size_ = 0;
  return *this;
}

template <typename T>
T* ManagedMmap<T>::operator->() {
  return mmap_ptr_;
}

template <typename T>
const T* ManagedMmap<T>::operator->() const {
  return mmap_ptr_;
}

template <typename T>
bool ManagedMmap<T>::is_initialized() const {
  return mmap_ptr_ != nullptr && required_size_ > 0;
}

}  // namespace firebase::telemetry::persistence::detail

#endif  //__FIREBASE_TELEMETRY_PERSISTENCE_DETAIL_MANAGED_MMAP_POINTER_H__
