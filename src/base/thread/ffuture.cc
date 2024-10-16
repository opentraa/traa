#include "base/thread/ffuture.h"

namespace traa {
namespace base {
inline namespace __detail {

void _fassoc_sub_state::__on_zero_shared() noexcept { delete this; }

void _fassoc_sub_state::__sub_wait(std::unique_lock<std::mutex> &__lk) {
  if (!__is_ready() && !__is_abandoned()) {
    if (__state_ & static_cast<unsigned>(deferred)) {
      __state_ &= ~static_cast<unsigned>(deferred);
      __lk.unlock();
      __execute();
    } else
      while (!__is_ready() && !__is_abandoned())
        __cv_.wait(__lk);
  }
}

void _fassoc_sub_state::__make_ready() {
  std::unique_lock<std::mutex> __lk(__mut_);
  __state_ |= ready;
  __cv_.notify_all();
}

void _fassoc_sub_state::__make_abandoned() {
  std::unique_lock<std::mutex> __lk(__mut_);
  __state_ |= abandoned;
  __cv_.notify_all();
}

void _fassoc_sub_state::set_value() {
  std::unique_lock<std::mutex> __lk(__mut_);
  if (__has_value())
    std::abort();
  __state_ |= __constructed | ready;
  __cv_.notify_all();
}

#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
void _fassoc_sub_state::set_value_at_thread_exit() {
  std::unique_lock<std::mutex> __lk(__mut_);
  if (__has_value())
    std::abort();
  __state_ |= __constructed;
  std::__thread_local_data()->__make_ready_at_thread_exit(this);
}
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS

void _fassoc_sub_state::copy() {
  std::unique_lock<std::mutex> __lk(__mut_);
  __sub_wait(__lk);
}

void _fassoc_sub_state::wait() {
  std::unique_lock<std::mutex> __lk(__mut_);
  __sub_wait(__lk);
}

void _fassoc_sub_state::__execute() { std::abort(); }

} // namespace __detail

ffuture<void>::ffuture(_fassoc_sub_state *__state) : __state_(__state) {
  __state_->__attach_future();
}

ffuture<void>::~ffuture() {
  if (__state_)
    __state_->__release_shared();
}

void ffuture<void>::get() {
  // to make sure that the __release_shared() is called after the __state_ is set to nullptr, then
  // the destructor of the ffutre will not call __release_shared() again.
  std::unique_ptr<_fshared_count, _frelease_shared_count> __(__state_);
  _fassoc_sub_state *__s = __state_;
  __state_ = nullptr;
  __s->copy();
}

fpromise<void>::fpromise() : __state_(new _fassoc_sub_state) {}

fpromise<void>::~fpromise() {
  if (__state_) {
    if (!__state_->__has_value() && __state_->use_count() > 1)
      __state_->__make_abandoned();
    __state_->__release_shared();
  }
}

ffuture<void> fpromise<void>::get_future() {
  if (__state_ == nullptr)
    std::abort();
  return ffuture<void>(__state_);
}

void fpromise<void>::set_value() {
  if (__state_ == nullptr)
    std::abort();
  __state_->set_value();
}

#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
void fpromise<void>::set_value_at_thread_exit() {
  if (__state_ == nullptr)
    std::abort();
  __state_->set_value_at_thread_exit();
}
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS

fshared_future<void>::~fshared_future() {
  if (__state_)
    __state_->__release_shared();
}

fshared_future<void> &fshared_future<void>::operator=(const fshared_future &__rhs) {
  if (__rhs.__state_)
    __rhs.__state_->__add_shared();
  if (__state_)
    __state_->__release_shared();
  __state_ = __rhs.__state_;
  return *this;
}

} // namespace base
} // namespace traa