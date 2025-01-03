/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#ifndef TRAA_BASE_SDK_OBJC_HELPERS_SCOPED_CFTYPEREF_H_
#define TRAA_BASE_SDK_OBJC_HELPERS_SCOPED_CFTYPEREF_H_

#include "base/checks.h"

#include <CoreFoundation/CoreFoundation.h>

namespace traa {
namespace base {

// retain: scoped_type_ref should retain the object when it takes
// ownership.
// assume: Assume the object already has already been retained.
// scoped_type_ref takes over ownership.
enum class retain_policy { retain, assume };

namespace internal {
template <typename T> struct cftype_ref_traits {
  static T invalid_value() { return nullptr; }
  static void release(T ref) { CFRelease(ref); }
  static T retain(T ref) {
    CFRetain(ref);
    return ref;
  }
};

template <typename T, typename Traits> class scoped_type_ref {
public:
  scoped_type_ref() : ptr_(Traits::invalid_value()) {}
  explicit scoped_type_ref(T ptr) : ptr_(ptr) {}
  scoped_type_ref(T ptr, retain_policy policy) : scoped_type_ref(ptr) {
    if (ptr_ && policy == retain_policy::retain)
      Traits::retain(ptr_);
  }

  scoped_type_ref(const scoped_type_ref<T, Traits> &rhs) : ptr_(rhs.ptr_) {
    if (ptr_)
      ptr_ = Traits::retain(ptr_);
  }

  ~scoped_type_ref() {
    if (ptr_) {
      Traits::release(ptr_);
    }
  }

  T get() const { return ptr_; }
  T operator->() const { return ptr_; }
  explicit operator bool() const { return ptr_; }

  bool operator!() const { return !ptr_; }

  scoped_type_ref &operator=(const T &rhs) {
    if (ptr_)
      Traits::release(ptr_);
    ptr_ = rhs;
    return *this;
  }

  scoped_type_ref &operator=(const scoped_type_ref<T, Traits> &rhs) {
    reset(rhs.get(), retain_policy::retain);
    return *this;
  }

  // This is intended to take ownership of objects that are
  // created by pass-by-pointer initializers.
  T *initialize_info() {
    TRAA_DCHECK(!ptr_);
    return &ptr_;
  }

  void reset(T ptr, retain_policy policy = retain_policy::assume) {
    if (ptr && policy == retain_policy::retain)
      Traits::retain(ptr);
    if (ptr_)
      Traits::release(ptr_);
    ptr_ = ptr;
  }

  T release() {
    T temp = ptr_;
    ptr_ = Traits::invalid_value();
    return temp;
  }

private:
  T ptr_;
};
} // namespace internal

template <typename T>
using scoped_cf_type_ref = internal::scoped_type_ref<T, internal::cftype_ref_traits<T>>;

template <typename T> static scoped_cf_type_ref<T> adopt_cf(T cftype) {
  return scoped_cf_type_ref<T>(cftype, retain_policy::retain);
}

template <typename T> static scoped_cf_type_ref<T> scoped_cf(T cftype) {
  return scoped_cf_type_ref<T>(cftype);
}

} // namespace base
} // namespace traa

#endif // TRAA_BASE_SDK_OBJC_HELPERS_SCOPED_CFTYPEREF_H_
