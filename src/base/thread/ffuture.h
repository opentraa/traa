#ifndef TRAA_BASE_THREAD_FFUTURE_H_
#define TRAA_BASE_THREAD_FFUTURE_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <type_traits>

/*
ffuture synopsis

namespace traa {
namespace base {

enum class ffuture_status
{
    ready,
    timeout,
    deferred,
    abandoned
};

template <class R>
class fpromise
{
public:
    fpromise();
    template <class Allocator>
        fpromise(allocator_arg_t, const Allocator& a);
    fpromise(fpromise&& rhs) noexcept;
    fpromise(const fpromise& rhs) = delete;
    ~fpromise();

    // assignment
    fpromise& operator=(fpromise&& rhs) noexcept;
    fpromise& operator=(const fpromise& rhs) = delete;
    void swap(fpromise& other) noexcept;

    // retrieving the result
    ffuture<R> get_future();

    // setting the result
    void set_value(const R& r);
    void set_value(R&& r);

    // setting the result with deferred notification
    void set_value_at_thread_exit(const R& r);
    void set_value_at_thread_exit(R&& r);
};

template <class R>
class fpromise<R&>
{
public:
    fpromise();
    template <class Allocator>
        fpromise(allocator_arg_t, const Allocator& a);
    fpromise(fpromise&& rhs) noexcept;
    fpromise(const fpromise& rhs) = delete;
    ~fpromise();

    // assignment
    fpromise& operator=(fpromise&& rhs) noexcept;
    fpromise& operator=(const fpromise& rhs) = delete;
    void swap(fpromise& other) noexcept;

    // retrieving the result
    ffuture<R&> get_future();

    // setting the result
    void set_value(R& r);

    // setting the result with deferred notification
    void set_value_at_thread_exit(R&);
};

template <>
class fpromise<void>
{
public:
    fpromise();
    template <class Allocator>
        fpromise(allocator_arg_t, const Allocator& a);
    fpromise(fpromise&& rhs) noexcept;
    fpromise(const fpromise& rhs) = delete;
    ~fpromise();

    // assignment
    fpromise& operator=(fpromise&& rhs) noexcept;
    fpromise& operator=(const fpromise& rhs) = delete;
    void swap(fpromise& other) noexcept;

    // retrieving the result
    ffuture<void> get_future();

    // setting the result
    void set_value();

    // setting the result with deferred notification
    void set_value_at_thread_exit();
};

template <class R> void swap(fpromise<R>& x, fpromise<R>& y) noexcept;

template <class R, class Alloc>
    struct uses_allocator<fpromise<R>, Alloc> : public true_type {};

template <class R>
class ffuture
{
public:
    ffuture() noexcept;
    ffuture(ffuture&&) noexcept;
    ffuture(const ffuture& rhs) = delete;
    ~ffuture();
    ffuture& operator=(const ffuture& rhs) = delete;
    ffuture& operator=(ffuture&&) noexcept;
    fshared_future<R> share() noexcept;

    // retrieving the value
    R get();

    // functions to check state
    bool valid() const noexcept;

    void wait() const;
    template <class Rep, class Period>
        future_status
        wait_for(const chrono::duration<Rep, Period>& rel_time) const;
    template <class Clock, class Duration>
        future_status
        wait_until(const chrono::time_point<Clock, Duration>& abs_time) const;
};

template <class R>
class ffuture<R&>
{
public:
    ffuture() noexcept;
    ffuture(ffuture&&) noexcept;
    ffuture(const ffuture& rhs) = delete;
    ~ffuture();
    ffuture& operator=(const ffuture& rhs) = delete;
    ffuture& operator=(ffuture&&) noexcept;
    fshared_future<R&> share() noexcept;

    // retrieving the value
    R& get();

    // functions to check state
    bool valid() const noexcept;

    void wait() const;
    template <class Rep, class Period>
        future_status
        wait_for(const chrono::duration<Rep, Period>& rel_time) const;
    template <class Clock, class Duration>
        future_status
        wait_until(const chrono::time_point<Clock, Duration>& abs_time) const;
};

template <>
class ffuture<void>
{
public:
    ffuture() noexcept;
    ffuture(ffuture&&) noexcept;
    ffuture(const ffuture& rhs) = delete;
    ~ffuture();
    ffuture& operator=(const ffuture& rhs) = delete;
    ffuture& operator=(ffuture&&) noexcept;
    fshared_future<void> share() noexcept;

    // retrieving the value
    void get();

    // functions to check state
    bool valid() const noexcept;

    void wait() const;
    template <class Rep, class Period>
        future_status
        wait_for(const chrono::duration<Rep, Period>& rel_time) const;
    template <class Clock, class Duration>
        future_status
        wait_until(const chrono::time_point<Clock, Duration>& abs_time) const;
};

template <class R>
class fshared_future
{
public:
    fshared_future() noexcept;
    fshared_future(const fshared_future& rhs);
    fshared_future(ffuture<R>&&) noexcept;
    fshared_future(fshared_future&& rhs) noexcept;
    ~fshared_future();
    fshared_future& operator=(const fshared_future& rhs);
    fshared_future& operator=(fshared_future&& rhs) noexcept;

    // retrieving the value
    const R& get() const;

    // functions to check state
    bool valid() const noexcept;

    void wait() const;
    template <class Rep, class Period>
        future_status
        wait_for(const chrono::duration<Rep, Period>& rel_time) const;
    template <class Clock, class Duration>
        future_status
        wait_until(const chrono::time_point<Clock, Duration>& abs_time) const;
};

template <class R>
class fshared_future<R&>
{
public:
    fshared_future() noexcept;
    fshared_future(const fshared_future& rhs);
    fshared_future(ffuture<R&>&&) noexcept;
    fshared_future(fshared_future&& rhs) noexcept;
    ~fshared_future();
    fshared_future& operator=(const fshared_future& rhs);
    fshared_future& operator=(fshared_future&& rhs) noexcept;

    // retrieving the value
    R& get() const;

    // functions to check state
    bool valid() const noexcept;

    void wait() const;
    template <class Rep, class Period>
        future_status
        wait_for(const chrono::duration<Rep, Period>& rel_time) const;
    template <class Clock, class Duration>
        future_status
        wait_until(const chrono::time_point<Clock, Duration>& abs_time) const;
};

template <>
class fshared_future<void>
{
public:
    fshared_future() noexcept;
    fshared_future(const fshared_future& rhs);
    fshared_future(ffuture<void>&&) noexcept;
    fshared_future(fshared_future&& rhs) noexcept;
    ~fshared_future();
    fshared_future& operator=(const fshared_future& rhs);
    fshared_future& operator=(fshared_future&& rhs) noexcept;

    // retrieving the value
    void get() const;

    // functions to check state
    bool valid() const noexcept;

    void wait() const;
    template <class Rep, class Period>
        future_status
        wait_for(const chrono::duration<Rep, Period>& rel_time) const;
    template <class Clock, class Duration>
        future_status
        wait_until(const chrono::time_point<Clock, Duration>& abs_time) const;
};

template <class> class fpackaged_task; // undefined

template <class R, class... ArgTypes>
class fpackaged_task<R(ArgTypes...)>
{
public:
    typedef R result_type; // extension

    // construction and destruction
    fpackaged_task() noexcept;
    template <class F>
        explicit fpackaged_task(F&& f);
    template <class F, class Allocator>
        fpackaged_task(allocator_arg_t, const Allocator& a, F&& f);              // removed in C++17
    ~fpackaged_task();

    // no copy
    fpackaged_task(const fpackaged_task&) = delete;
    fpackaged_task& operator=(const fpackaged_task&) = delete;

    // move support
    fpackaged_task(fpackaged_task&& other) noexcept;
    fpackaged_task& operator=(fpackaged_task&& other) noexcept;
    void swap(fpackaged_task& other) noexcept;

    bool valid() const noexcept;

    // result retrieval
    ffuture<R> get_future();

    // execution
    void operator()(ArgTypes... );
    void make_ready_at_thread_exit(ArgTypes...);

    void reset();
};

template <class R>
  void swap(fpackaged_task<R(ArgTypes...)&, fpackaged_task<R(ArgTypes...)>&) noexcept;

template <class R, class Alloc> struct uses_allocator<fpackaged_task<R>, Alloc>; // removed in C++17

} // namespace base
} // namespace traa
*/

namespace traa {
namespace base {

enum class ffuture_status { ready, timeout, deferred, abandoned };

inline namespace __detail {

template <class _Alloc> class _fallocator_destructor {
  typedef std::allocator_traits<_Alloc> __alloc_traits;

public:
  typedef typename __alloc_traits::pointer pointer;
  typedef typename __alloc_traits::size_type size_type;

private:
  _Alloc &__alloc_;
  size_type __s_;

public:
  _fallocator_destructor(_Alloc &__a, size_type __s) noexcept : __alloc_(__a), __s_(__s) {}

  void operator()(pointer __p) noexcept { __alloc_traits::deallocate(__alloc_, __p, __s_); }
};

// llvm-project/blob/main/libcxx/include/__type_traits/strip_signature.h
#if _LIBCPP_STD_VER >= 17
template <class _Fp> struct _fstrip_signature;

#if defined(__cpp_static_call_operator) && __cpp_static_call_operator >= 202207L

template <class _Rp, class... _Args> struct _fstrip_signature<_Rp (*)(_Args...)> {
  using type = _Rp(_Args...);
};

template <class _Rp, class... _Args> struct _fstrip_signature<_Rp (*)(_Args...) noexcept> {
  using type = _Rp(_Args...);
};

#endif // defined(__cpp_static_call_operator) && __cpp_static_call_operator >= 202207L

// clang-format off
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...)> { using type = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) const> { using type = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) volatile> { using type = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) const volatile> { using type = _Rp(_Ap...); };

template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) &> { using type = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) const &> { using type = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) volatile &> { using type = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) const volatile &> { using type = _Rp(_Ap...); };

template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) noexcept> { using type = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) const noexcept> { using type = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) volatile noexcept> { using type = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) const volatile noexcept> { using type = _Rp(_Ap...); };

template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) & noexcept> { using type = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) const & noexcept> { using type = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) volatile & noexcept> { using type = _Rp(_Ap...); };
template<class _Rp, class _Gp, class ..._Ap>
struct _fstrip_signature<_Rp (_Gp::*) (_Ap...) const volatile & noexcept> { using type = _Rp(_Ap...); };
// clang-format on
#endif // _LIBCPP_STD_VER >= 17

#define TRAA_TEST_ENABLE_FUTURE_COUNT 0
#if TRAA_TEST_ENABLE_FUTURE_COUNT
static std::atomic<long> __call_add_count_{0};
static std::atomic<long> __call_release_count_{0};
#endif // TRAA_TEST_ENABLE_FUTURE_COUNT

class _fshared_count {
  _fshared_count(const _fshared_count &rhs) = delete;
  _fshared_count &operator=(const _fshared_count &rhs) = delete;

protected:
  std::atomic<long> __shared_owners_;
  virtual ~_fshared_count(){};

private:
  virtual void __on_zero_shared() noexcept = 0;

public:
  explicit _fshared_count() noexcept : __shared_owners_(0) {}

  void __add_shared() noexcept {
    __shared_owners_.fetch_add(1, std::memory_order_relaxed);
#if TRAA_TEST_ENABLE_FUTURE_COUNT
    printf("__call_add_count_: %ld\r\n",
           __call_add_count_.fetch_add(1, std::memory_order_relaxed) + 1);
#endif // TRAA_TEST_ENABLE_FUTURE_COUNT
  }

  bool __release_shared() noexcept {
#if TRAA_TEST_ENABLE_FUTURE_COUNT
    printf("__call_release_count_: %ld\r\n",
           __call_release_count_.fetch_add(1, std::memory_order_relaxed) + 1);
#endif // TRAA_TEST_ENABLE_FUTURE_COUNT
    if (__shared_owners_.fetch_add(-1, std::memory_order::memory_order_acq_rel) == 0) {
      __on_zero_shared();
      return true;
    }
    return false;
  }

  long use_count() const noexcept {
    // coz fpromise do not call __add_shared, so we need to add 1 here
    return __shared_owners_.load(std::memory_order_relaxed) + 1;
  }
};

struct _frelease_shared_count {
  void operator()(_fshared_count *__p) { __p->__release_shared(); }
};

class _fassoc_sub_state : public _fshared_count {
protected:
  mutable std::mutex __mut_;
  mutable std::condition_variable __cv_;
  unsigned __state_;

  void __on_zero_shared() noexcept override;
  void __sub_wait(std::unique_lock<std::mutex> &__lk);

public:
  enum { __constructed = 1, __future_attached = 2, ready = 4, deferred = 8, abandoned = 16 };

  _fassoc_sub_state() : __state_(0) {}

  bool __has_value() const { return (__state_ & __constructed) || (__state_ & abandoned); }

  void __attach_future() {
    std::lock_guard<std::mutex> __lk(__mut_);
    bool __has_future_attached = (__state_ & __future_attached) != 0;
    if (__has_future_attached)
      std::abort();
    this->__add_shared();
    __state_ |= __future_attached;
  }

  void __set_deferred() { __state_ |= deferred; }

  void __make_ready();
  bool __is_ready() const { return (__state_ & ready) != 0; }

  void __make_abandoned();
  bool __is_abandoned() const { return (__state_ & abandoned) != 0; }

  void set_value();
#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
  void set_value_at_thread_exit();
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS

  void copy();

  void wait();

  template <class _Rep, class _Period>
  ffuture_status wait_for(const std::chrono::duration<_Rep, _Period> &__rel_time) const;
  template <class _Clock, class _Duration>
  ffuture_status wait_until(const std::chrono::time_point<_Clock, _Duration> &__abs_time) const;

  virtual void __execute();
};

template <class _Clock, class _Duration>
ffuture_status
_fassoc_sub_state::wait_until(const std::chrono::time_point<_Clock, _Duration> &__abs_time) const {
  std::unique_lock<std::mutex> __lk(__mut_);
  if (__state_ & deferred)
    return ffuture_status::deferred;
  while (!((__state_ & ready) || (__state_ & abandoned)) && _Clock::now() < __abs_time)
    __cv_.wait_until(__lk, __abs_time);
  if (__state_ & ready)
    return ffuture_status::ready;
  else if (__state_ & abandoned)
    return ffuture_status::abandoned;
  return ffuture_status::timeout;
}

template <class _Rep, class _Period>
ffuture_status
_fassoc_sub_state::wait_for(const std::chrono::duration<_Rep, _Period> &__rel_time) const {
  return wait_until(std::chrono::steady_clock::now() + __rel_time);
}

template <class _Rp> class _fassoc_state : public _fassoc_sub_state {
  typedef typename std::aligned_storage<sizeof(_Rp), std::alignment_of<_Rp>::value>::type _Up;

protected:
  _Up __value_;

  void __on_zero_shared() noexcept override;

public:
  template <class _Arg> void set_value(_Arg &&__arg);

#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
  template <class _Arg> void set_value_at_thread_exit(_Arg &&__arg);
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS

  _Rp move(_Rp default_value);
  std::add_lvalue_reference_t<_Rp> copy(_Rp &default_value);
  const std::add_lvalue_reference_t<_Rp> copy(const _Rp &default_value);
};

template <class _Rp> void _fassoc_state<_Rp>::__on_zero_shared() noexcept {
  if (this->__state_ & _fassoc_sub_state::__constructed)
    reinterpret_cast<_Rp *>(&__value_)->~_Rp();
  delete this;
}

template <class _Rp> template <class _Arg> void _fassoc_state<_Rp>::set_value(_Arg &&__arg) {
  std::unique_lock<std::mutex> __lk(this->__mut_);
  if (this->__has_value())
    std::abort();
  ::new ((void *)&__value_) _Rp(std::forward<_Arg>(__arg));
  this->__state_ |= _fassoc_sub_state::__constructed | _fassoc_sub_state::ready;
  __cv_.notify_all();
}

#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
template <class _Rp>
template <class _Arg>
void _fassoc_state<_Rp>::set_value_at_thread_exit(_Arg &&__arg) {
  std::unique_lock<std::mutex> __lk(this->__mut_);
  if (this->__has_value())
    std::abort();
  ::new ((void *)&__value_) _Rp(std::forward<_Arg>(__arg));
  this->__state_ |= _fassoc_sub_state::__constructed;
  std::__thread_local_data()->__make_ready_at_thread_exit(this);
}
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS

template <class _Rp> _Rp _fassoc_state<_Rp>::move(_Rp default_value) {
  std::unique_lock<std::mutex> __lk(this->__mut_);
  this->__sub_wait(__lk);
  if (this->__is_abandoned())
    return default_value;
  return std::move(*reinterpret_cast<_Rp *>(&__value_));
}

template <class _Rp> std::add_lvalue_reference_t<_Rp> _fassoc_state<_Rp>::copy(_Rp &default_value) {
  std::unique_lock<std::mutex> __lk(this->__mut_);
  this->__sub_wait(__lk);
  if (this->__is_abandoned())
    return default_value;
  return *reinterpret_cast<_Rp *>(&__value_);
}

template <class _Rp>
const std::add_lvalue_reference_t<_Rp> _fassoc_state<_Rp>::copy(const _Rp &default_value) {
  std::unique_lock<std::mutex> __lk(this->__mut_);
  this->__sub_wait(__lk);
  if (this->__is_abandoned())
    return const_cast<_Rp &>(default_value);
  return *reinterpret_cast<_Rp *>(&__value_);
}

template <class _Rp> class _fassoc_state<_Rp &> : public _fassoc_sub_state {
  typedef _Rp *_Up;

protected:
  _Up __value_;

  void __on_zero_shared() noexcept override;

public:
  void set_value(_Rp &__arg);

  _Rp &copy(_Rp &default_value);
};

template <class _Rp> void _fassoc_state<_Rp &>::__on_zero_shared() noexcept { delete this; }

template <class _Rp> void _fassoc_state<_Rp &>::set_value(_Rp &__arg) {
  std::unique_lock<std::mutex> __lk(this->__mut_);
  if (this->__has_value())
    std::abort();
  __value_ = std::addressof(__arg);
  this->__state_ |= _fassoc_sub_state::__constructed | _fassoc_sub_state::ready;
  __cv_.notify_all();
}

template <class _Rp> _Rp &_fassoc_state<_Rp &>::copy(_Rp &default_value) {
  std::unique_lock<std::mutex> __lk(this->__mut_);
  this->__sub_wait(__lk);
  if (this->__is_abandoned())
    return default_value;
  return *__value_;
}

template <class _Rp, class _Alloc> class _fassoc_state_alloc : public _fassoc_state<_Rp> {
  _Alloc __alloc_;

  virtual void __on_zero_shared() noexcept;

public:
  explicit _fassoc_state_alloc(const _Alloc &__a) : __alloc_(__a) {}
};

template <class _Rp, class _Alloc>
void _fassoc_state_alloc<_Rp, _Alloc>::__on_zero_shared() noexcept {
  if (this->__state_ & _fassoc_state<_Rp>::__constructed)
    reinterpret_cast<_Rp *>(std::addressof(this->__value_))->~_Rp();
  typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<
      _fassoc_state_alloc<_Rp, _Alloc>>
      _Al;
  typedef std::allocator_traits<_Al> _ATraits;
  typedef std::pointer_traits<typename _ATraits::pointer> _PTraits;
  _Al __a(__alloc_);
  this->~_fassoc_state_alloc();
  __a.deallocate(_PTraits::pointer_to(*this), 1);
}

template <class _Rp, class _Alloc>
class _fassoc_state_alloc<_Rp &, _Alloc> : public _fassoc_state<_Rp &> {
  _Alloc __alloc_;

  virtual void __on_zero_shared() noexcept;

public:
  explicit _fassoc_state_alloc(const _Alloc &__a) : __alloc_(__a) {}
};

template <class _Rp, class _Alloc>
void _fassoc_state_alloc<_Rp &, _Alloc>::__on_zero_shared() noexcept {
  typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<
      _fassoc_state_alloc<_Rp &, _Alloc>>
      _Al;
  typedef std::allocator_traits<_Al> _ATraits;
  typedef std::pointer_traits<typename _ATraits::pointer> _PTraits;
  _Al __a(__alloc_);
  this->~_fassoc_state_alloc();
  __a.deallocate(_PTraits::pointer_to(*this), 1);
}

template <class _Alloc> class _fassoc_sub_state_alloc : public _fassoc_sub_state {
  _Alloc __alloc_;

  void __on_zero_shared() noexcept override;

public:
  explicit _fassoc_sub_state_alloc(const _Alloc &__a) : __alloc_(__a) {}
};

template <class _Alloc> void _fassoc_sub_state_alloc<_Alloc>::__on_zero_shared() noexcept {
  typedef
      typename std::allocator_traits<_Alloc>::template rebind_alloc<_fassoc_sub_state_alloc<_Alloc>>
          _Al;
  typedef std::allocator_traits<_Al> _ATraits;
  typedef std::pointer_traits<typename _ATraits::pointer> _PTraits;
  _Al __a(__alloc_);
  this->~_fassoc_sub_state_alloc();
  __a.deallocate(_PTraits::pointer_to(*this), 1);
}

template <class _Rp, class _Fp> class _fdeferred_assoc_state : public _fassoc_state<_Rp> {
  typedef _fassoc_state<_Rp> base_state;

  _Fp __func_;

public:
  explicit _fdeferred_assoc_state(_Fp &&__f);

  virtual void __execute();
};

template <class _Rp, class _Fp>
inline _fdeferred_assoc_state<_Rp, _Fp>::_fdeferred_assoc_state(_Fp &&__f)
    : __func_(std::forward<_Fp>(__f)) {
  this->__set_deferred();
}

template <class _Rp, class _Fp> void _fdeferred_assoc_state<_Rp, _Fp>::__execute() {
  this->set_value(__func_());
}

template <class _Fp> class _fdeferred_assoc_state<void, _Fp> : public _fassoc_sub_state {
  _Fp __func_;

public:
  explicit _fdeferred_assoc_state(_Fp &&__f);

  void __execute() override;
};

template <class _Fp>
inline _fdeferred_assoc_state<void, _Fp>::_fdeferred_assoc_state(_Fp &&__f)
    : __func_(std::forward<_Fp>(__f)) {
  this->__set_deferred();
}

template <class _Fp> void _fdeferred_assoc_state<void, _Fp>::__execute() {
  __func_();
  this->set_value();
}

template <class _Rp, class _Fp> class _fasync_assoc_state : public _fassoc_state<_Rp> {
  _Fp __func_;

  virtual void __on_zero_shared() noexcept;

public:
  explicit _fasync_assoc_state(_Fp &&__f);

  virtual void __execute();
};

template <class _Rp, class _Fp>
inline _fasync_assoc_state<_Rp, _Fp>::_fasync_assoc_state(_Fp &&__f)
    : __func_(std::forward<_Fp>(__f)) {}

template <class _Rp, class _Fp> void _fasync_assoc_state<_Rp, _Fp>::__execute() {
  this->set_value(__func_());
}

template <class _Rp, class _Fp> void _fasync_assoc_state<_Rp, _Fp>::__on_zero_shared() noexcept {
  this->wait();
  _fassoc_state<_Rp>::__on_zero_shared();
}

template <class _Fp> class _fasync_assoc_state<void, _Fp> : public _fassoc_sub_state {
  _Fp __func_;

  void __on_zero_shared() noexcept override;

public:
  explicit _fasync_assoc_state(_Fp &&__f);

  void __execute() override;
};

template <class _Fp>
inline _fasync_assoc_state<void, _Fp>::_fasync_assoc_state(_Fp &&__f)
    : __func_(std::forward<_Fp>(__f)) {}

template <class _Fp> void _fasync_assoc_state<void, _Fp>::__execute() {
  __func_();
  this->set_value();
}

template <class _Fp> void _fasync_assoc_state<void, _Fp>::__on_zero_shared() noexcept {
  this->wait();
  _fassoc_sub_state::__on_zero_shared();
}

template <class _Fp> class _fpackaged_task_base;

template <class _Rp, class... _ArgTypes> class _fpackaged_task_base<_Rp(_ArgTypes...)> {
public:
  _fpackaged_task_base() {}
  _fpackaged_task_base(const _fpackaged_task_base &) = delete;
  _fpackaged_task_base &operator=(const _fpackaged_task_base &) = delete;

  virtual ~_fpackaged_task_base() {}
  virtual void __move_to(_fpackaged_task_base *) noexcept = 0;
  virtual void destroy() = 0;
  virtual void destroy_deallocate() = 0;
  virtual _Rp operator()(_ArgTypes &&...) = 0;
};

template <class _FD, class _Alloc, class _FB> class _fpackaged_task_func;

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
class _fpackaged_task_func<_Fp, _Alloc, _Rp(_ArgTypes...)>
    : public _fpackaged_task_base<_Rp(_ArgTypes...)> {
  // _LIBCPP_COMPRESSED_PAIR(_Fp, __func_, _Alloc, __alloc_);
  _Fp __func_;
  _Alloc __alloc_;

public:
  explicit _fpackaged_task_func(const _Fp &__f) : __func_(__f) {}
  explicit _fpackaged_task_func(_Fp &&__f) : __func_(std::move(__f)) {}
  _fpackaged_task_func(const _Fp &__f, const _Alloc &__a) : __func_(__f), __alloc_(__a) {}
  _fpackaged_task_func(_Fp &&__f, const _Alloc &__a) : __func_(std::move(__f)), __alloc_(__a) {}
  virtual void __move_to(_fpackaged_task_base<_Rp(_ArgTypes...)> *) noexcept;
  virtual void destroy();
  virtual void destroy_deallocate();
  virtual _Rp operator()(_ArgTypes &&...__args);
};

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
void _fpackaged_task_func<_Fp, _Alloc, _Rp(_ArgTypes...)>::__move_to(
    _fpackaged_task_base<_Rp(_ArgTypes...)> *__p) noexcept {
  ::new ((void *)__p) _fpackaged_task_func(std::move(__func_), std::move(__alloc_));
}

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
void _fpackaged_task_func<_Fp, _Alloc, _Rp(_ArgTypes...)>::destroy() {
  __func_.~_Fp();
  __alloc_.~_Alloc();
}

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
void _fpackaged_task_func<_Fp, _Alloc, _Rp(_ArgTypes...)>::destroy_deallocate() {
  typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<
      _fpackaged_task_func<_Fp, _Alloc, _Rp(_ArgTypes...)>>
      _Ap;
  typedef std::allocator_traits<_Ap> _ATraits;
  typedef std::pointer_traits<typename _ATraits::pointer> _PTraits;
  _Ap __a(__alloc_);
  __func_.~_Fp();
  __alloc_.~_Alloc();
  __a.deallocate(_PTraits::pointer_to(*this), 1);
}

template <class _Fp, class _Alloc, class _Rp, class... _ArgTypes>
_Rp _fpackaged_task_func<_Fp, _Alloc, _Rp(_ArgTypes...)>::operator()(_ArgTypes &&...__arg) {
  return std::invoke(__func_, std::forward<_ArgTypes>(__arg)...);
}

template <class _Callable> class _fpackaged_task_function;

template <class _Rp, class... _ArgTypes> class _fpackaged_task_function<_Rp(_ArgTypes...)> {
  typedef _fpackaged_task_base<_Rp(_ArgTypes...)> __base;

  __base *__get_buf() { return (__base *)&__buf_; }

  typename std::aligned_storage<3 * sizeof(void *)>::type __buf_;

  __base *__f_;

public:
  typedef _Rp result_type;

  // construct/copy/destroy:
  _fpackaged_task_function() noexcept : __f_(nullptr) {}
  template <class _Fp> _fpackaged_task_function(_Fp &&__f);
  template <class _Fp, class _Alloc>
  _fpackaged_task_function(std::allocator_arg_t, const _Alloc &__a, _Fp &&__f);

  _fpackaged_task_function(_fpackaged_task_function &&) noexcept;
  _fpackaged_task_function &operator=(_fpackaged_task_function &&) noexcept;

  _fpackaged_task_function(const _fpackaged_task_function &) = delete;
  _fpackaged_task_function &operator=(const _fpackaged_task_function &) = delete;

  ~_fpackaged_task_function();

  void swap(_fpackaged_task_function &) noexcept;

  _Rp operator()(_ArgTypes...) const;
};

template <class _Rp, class... _ArgTypes>
_fpackaged_task_function<_Rp(_ArgTypes...)>::_fpackaged_task_function(
    _fpackaged_task_function &&__f) noexcept {
  if (__f.__f_ == nullptr)
    __f_ = nullptr;
  else if (__f.__f_ == __f.__get_buf()) {
    __f.__f_->__move_to(__get_buf());
    __f_ = (__base *)&__buf_;
  } else {
    __f_ = __f.__f_;
    __f.__f_ = nullptr;
  }
}

template <class _Rp, class... _ArgTypes>
template <class _Fp>
_fpackaged_task_function<_Rp(_ArgTypes...)>::_fpackaged_task_function(_Fp &&__f) : __f_(nullptr) {
  typedef std::remove_reference_t<std::decay_t<_Fp>> _FR;
  typedef _fpackaged_task_func<_FR, std::allocator<_FR>, _Rp(_ArgTypes...)> _FF;
  // TODO @sylar: on linux has error
  // if (sizeof(_FF) <= sizeof(__buf_)) {
  //   ::new ((void *)&__buf_) _FF(std::forward<_Fp>(__f));
  //   __f_ = (__base *)&__buf_;
  // } else {
    typedef std::allocator<_FF> _Ap;
    _Ap __a;
    typedef _fallocator_destructor<_Ap> _Dp;
    std::unique_ptr<__base, _Dp> __hold(__a.allocate(1), _Dp(__a, 1));
    ::new ((void *)__hold.get()) _FF(std::forward<_Fp>(__f), std::allocator<_FR>(__a));
    __f_ = __hold.release();
  // }
}

template <class _Rp, class... _ArgTypes>
template <class _Fp, class _Alloc>
_fpackaged_task_function<_Rp(_ArgTypes...)>::_fpackaged_task_function(std::allocator_arg_t,
                                                                      const _Alloc &__a0, _Fp &&__f)
    : __f_(nullptr) {
  typedef std::remove_reference_t<std::decay_t<_Fp>> _FR;
  typedef _fpackaged_task_func<_FR, _Alloc, _Rp(_ArgTypes...)> _FF;
  // TODO @sylar: on linux has error
  // if (sizeof(_FF) <= sizeof(__buf_)) {
  //   __f_ = (__base *)&__buf_;
  //   ::new ((void *)__f_) _FF(std::forward<_Fp>(__f));
  // } else {
    typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<_FF> _Ap;
    _Ap __a(__a0);
    typedef _fallocator_destructor<_Ap> _Dp;
    std::unique_ptr<__base, _Dp> __hold(__a.allocate(1), _Dp(__a, 1));
    ::new ((void *)std::addressof(*__hold.get())) _FF(std::forward<_Fp>(__f), _Alloc(__a));
    __f_ = std::addressof(*__hold.release());
  // }
}

template <class _Rp, class... _ArgTypes>
_fpackaged_task_function<_Rp(_ArgTypes...)> &
_fpackaged_task_function<_Rp(_ArgTypes...)>::operator=(_fpackaged_task_function &&__f) noexcept {
  if (__f_ == __get_buf())
    __f_->destroy();
  else if (__f_)
    __f_->destroy_deallocate();
  __f_ = nullptr;
  if (__f.__f_ == nullptr)
    __f_ = nullptr;
  else if (__f.__f_ == __f.__get_buf()) {
    __f.__f_->__move_to(__get_buf());
    __f_ = __get_buf();
  } else {
    __f_ = __f.__f_;
    __f.__f_ = nullptr;
  }
  return *this;
}

template <class _Rp, class... _ArgTypes>
_fpackaged_task_function<_Rp(_ArgTypes...)>::~_fpackaged_task_function() {
  if (__f_ == __get_buf())
    __f_->destroy();
  else if (__f_)
    __f_->destroy_deallocate();
}

template <class _Rp, class... _ArgTypes>
void _fpackaged_task_function<_Rp(_ArgTypes...)>::swap(_fpackaged_task_function &__f) noexcept {
  if (__f_ == (__base *)&__buf_ && __f.__f_ == (__base *)&__f.__buf_) {
    typename std::aligned_storage<sizeof(__buf_)>::type __tempbuf;
    __base *__t = (__base *)&__tempbuf;
    __f_->__move_to(__t);
    __f_->destroy();
    __f_ = nullptr;
    __f.__f_->__move_to((__base *)&__buf_);
    __f.__f_->destroy();
    __f.__f_ = nullptr;
    __f_ = (__base *)&__buf_;
    __t->__move_to((__base *)&__f.__buf_);
    __t->destroy();
    __f.__f_ = (__base *)&__f.__buf_;
  } else if (__f_ == (__base *)&__buf_) {
    __f_->__move_to((__base *)&__f.__buf_);
    __f_->destroy();
    __f_ = __f.__f_;
    __f.__f_ = (__base *)&__f.__buf_;
  } else if (__f.__f_ == (__base *)&__f.__buf_) {
    __f.__f_->__move_to((__base *)&__buf_);
    __f.__f_->destroy();
    __f.__f_ = __f_;
    __f_ = (__base *)&__buf_;
  } else
    std::swap(__f_, __f.__f_);
}

template <class _Rp, class... _ArgTypes>
inline _Rp _fpackaged_task_function<_Rp(_ArgTypes...)>::operator()(_ArgTypes... __arg) const {
  return (*__f_)(std::forward<_ArgTypes>(__arg)...);
}

} // namespace __detail

template <class _Rp> class fpromise;
template <class _Rp> class fshared_future;

// ffuture

template <class _Rp> class ffuture;

template <class _Rp, class _Fp> ffuture<_Rp> __make_fdeferred_assoc_state(_Fp &&__f);

template <class _Rp, class _Fp> ffuture<_Rp> __make_fasync_assoc_state(_Fp &&__f);

template <class _Rp> class ffuture {
  _fassoc_state<_Rp> *__state_;

  explicit ffuture(_fassoc_state<_Rp> *__state);

  template <class> friend class fpromise;
  template <class> friend class fshared_future;

  template <class _R1, class _Fp> friend ffuture<_R1> __make_fdeferred_assoc_state(_Fp &&__f);
  template <class _R1, class _Fp> friend ffuture<_R1> __make_fasync_assoc_state(_Fp &&__f);

public:
  ffuture() noexcept : __state_(nullptr) {}
  ffuture(ffuture &&__rhs) noexcept : __state_(__rhs.__state_) { __rhs.__state_ = nullptr; }
  ffuture(const ffuture &) = delete;
  ffuture &operator=(const ffuture &) = delete;

  ffuture &operator=(ffuture &&__rhs) noexcept {
    ffuture(std::move(__rhs)).swap(*this);
    return *this;
  }

  ~ffuture();

  fshared_future<_Rp> share() noexcept;

  // retrieving the value
  _Rp get(_Rp default_value);

  void swap(ffuture &__rhs) noexcept { std::swap(__state_, __rhs.__state_); }

  // functions to check state

  bool valid() const noexcept { return __state_ != nullptr && !__state_->__is_abandoned(); }

  void wait() const { __state_->wait(); }
  template <class _Rep, class _Period>
  ffuture_status wait_for(const std::chrono::duration<_Rep, _Period> &__rel_time) const {
    return __state_->wait_for(__rel_time);
  }
  template <class _Clock, class _Duration>
  ffuture_status wait_until(const std::chrono::time_point<_Clock, _Duration> &__abs_time) const {
    return __state_->wait_until(__abs_time);
  }
};

template <class _Rp> ffuture<_Rp>::ffuture(_fassoc_state<_Rp> *__state) : __state_(__state) {
  __state_->__attach_future();
}

template <class _Rp> ffuture<_Rp>::~ffuture() {
  if (__state_)
    __state_->__release_shared();
}

template <class _Rp> _Rp ffuture<_Rp>::get(_Rp default_value) {
  // to make sure that the __release_shared() is called after the __state_ is set to nullptr, then
  // the destructor of the ffutre will not call __release_shared() again.
  std::unique_ptr<_fshared_count, _frelease_shared_count> __guard(__state_);
  _fassoc_state<_Rp> *__s = __state_;
  __state_ = nullptr;
  return __s->move(default_value);
}

template <class _Rp> class ffuture<_Rp &> {
  _fassoc_state<_Rp &> *__state_;

  explicit ffuture(_fassoc_state<_Rp &> *__state);

  template <class> friend class fpromise;
  template <class> friend class fshared_future;

  template <class _R1, class _Fp> friend ffuture<_R1> __make_fdeferred_assoc_state(_Fp &&__f);
  template <class _R1, class _Fp> friend ffuture<_R1> __make_fasync_assoc_state(_Fp &&__f);

public:
  ffuture() noexcept : __state_(nullptr) {}

  ffuture(ffuture &&__rhs) noexcept : __state_(__rhs.__state_) { __rhs.__state_ = nullptr; }
  ffuture(const ffuture &) = delete;
  ffuture &operator=(const ffuture &) = delete;

  ffuture &operator=(ffuture &&__rhs) noexcept {
    ffuture(std::move(__rhs)).swap(*this);
    return *this;
  }

  ~ffuture();

  fshared_future<_Rp &> share() noexcept;

  // retrieving the value
  _Rp &get(_Rp &default_value);

  void swap(ffuture &__rhs) noexcept { std::swap(__state_, __rhs.__state_); }

  // functions to check state

  bool valid() const noexcept { return __state_ != nullptr && !__state_->__is_abandoned(); }

  void wait() const { __state_->wait(); }
  template <class _Rep, class _Period>
  ffuture_status wait_for(const std::chrono::duration<_Rep, _Period> &__rel_time) const {
    return __state_->wait_for(__rel_time);
  }
  template <class _Clock, class _Duration>
  ffuture_status wait_until(const std::chrono::time_point<_Clock, _Duration> &__abs_time) const {
    return __state_->wait_until(__abs_time);
  }
};

template <class _Rp> ffuture<_Rp &>::ffuture(_fassoc_state<_Rp &> *__state) : __state_(__state) {
  __state_->__attach_future();
}

template <class _Rp> ffuture<_Rp &>::~ffuture() {
  if (__state_)
    __state_->__release_shared();
}

template <class _Rp> _Rp &ffuture<_Rp &>::get(_Rp &default_value) {
  // to make sure that the __release_shared() is called after the __state_ is set to nullptr, then
  // the destructor of the ffutre will not call __release_shared() again.
  std::unique_ptr<_fshared_count, _frelease_shared_count> __guard(__state_);
  _fassoc_state<_Rp &> *__s = __state_;
  __state_ = nullptr;
  return __s->copy(default_value);
}

template <> class ffuture<void> {
  _fassoc_sub_state *__state_;

  explicit ffuture(_fassoc_sub_state *__state);

  template <class> friend class fpromise;
  template <class> friend class fshared_future;

  template <class _R1, class _Fp> friend ffuture<_R1> __make_fdeferred_assoc_state(_Fp &&__f);
  template <class _R1, class _Fp> friend ffuture<_R1> __make_fasync_assoc_state(_Fp &&__f);

public:
  ffuture() noexcept : __state_(nullptr) {}

  ffuture(ffuture &&__rhs) noexcept : __state_(__rhs.__state_) { __rhs.__state_ = nullptr; }
  ffuture(const ffuture &) = delete;
  ffuture &operator=(const ffuture &) = delete;

  ffuture &operator=(ffuture &&__rhs) noexcept {
    ffuture(std::move(__rhs)).swap(*this);
    return *this;
  }

  ~ffuture();

  fshared_future<void> share() noexcept;

  // retrieving the value
  void get();

  void swap(ffuture &__rhs) noexcept { std::swap(__state_, __rhs.__state_); }

  // functions to check state

  bool valid() const noexcept { return __state_ != nullptr && !__state_->__is_abandoned(); }

  void wait() const { __state_->wait(); }
  template <class _Rep, class _Period>
  ffuture_status wait_for(const std::chrono::duration<_Rep, _Period> &__rel_time) const {
    return __state_->wait_for(__rel_time);
  }
  template <class _Clock, class _Duration>
  ffuture_status wait_until(const std::chrono::time_point<_Clock, _Duration> &__abs_time) const {
    return __state_->wait_until(__abs_time);
  }
};

// fpromise<R>

template <class _Callable> class fpackaged_task;

template <class _Rp> class fpromise {
  _fassoc_state<_Rp> *__state_;

  explicit fpromise(nullptr_t) noexcept : __state_(nullptr) {}

  template <class> friend class fpackaged_task;

public:
  fpromise();
  template <class _Alloc> fpromise(std::allocator_arg_t, const _Alloc &__a);

  fpromise(fpromise &&__rhs) noexcept : __state_(__rhs.__state_) { __rhs.__state_ = nullptr; }
  fpromise(const fpromise &__rhs) = delete;
  ~fpromise();

  // assignment
  fpromise &operator=(fpromise &&__rhs) noexcept {
    fpromise(std::move(__rhs)).swap(*this);
    return *this;
  }
  fpromise &operator=(const fpromise &__rhs) = delete;

  void swap(fpromise &__rhs) noexcept { std::swap(__state_, __rhs.__state_); }

  // retrieving the result
  ffuture<_Rp> get_future();

  // setting the result
  void set_value(const _Rp &__r);
  void set_value(_Rp &&__r);

// setting the result with deferred notification
#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
  void set_value_at_thread_exit(const _Rp &__r);
  void set_value_at_thread_exit(_Rp &&__r);
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS
};

template <class _Rp> fpromise<_Rp>::fpromise() : __state_(new _fassoc_state<_Rp>) {}

template <class _Rp>
template <class _Alloc>
fpromise<_Rp>::fpromise(std::allocator_arg_t, const _Alloc &__a0) {
  typedef _fassoc_state_alloc<_Rp, _Alloc> _State;
  typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<_State> _A2;
  typedef _fallocator_destructor<_A2> _D2;
  _A2 __a(__a0);
  std::unique_ptr<_State, _D2> __hold(__a.allocate(1), _D2(__a, 1));
  ::new ((void *)std::addressof(*__hold.get())) _State(__a0);
  __state_ = std::addressof(*__hold.release());
}

template <class _Rp> fpromise<_Rp>::~fpromise() {
  if (__state_) {
    if (!__state_->__has_value() && __state_->use_count() > 1)
      __state_->__make_abandoned();
    __state_->__release_shared();
  }
}

template <class _Rp> ffuture<_Rp> fpromise<_Rp>::get_future() {
  if (__state_ == nullptr)
    std::abort();
  return ffuture<_Rp>(__state_);
}

template <class _Rp> void fpromise<_Rp>::set_value(const _Rp &__r) {
  if (__state_ == nullptr)
    std::abort();
  __state_->set_value(__r);
}

template <class _Rp> void fpromise<_Rp>::set_value(_Rp &&__r) {
  if (__state_ == nullptr)
    std::abort();
  __state_->set_value(std::move(__r));
}

#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
template <class _Rp> void fpromise<_Rp>::set_value_at_thread_exit(const _Rp &__r) {
  if (__state_ == nullptr)
    std::abort();
  __state_->set_value_at_thread_exit(__r);
}

template <class _Rp> void fpromise<_Rp>::set_value_at_thread_exit(_Rp &&__r) {
  if (__state_ == nullptr)
    std::abort();
  __state_->set_value_at_thread_exit(std::move(__r));
}
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS

// fpromise<R&>

template <class _Rp> class fpromise<_Rp &> {
  _fassoc_state<_Rp &> *__state_;

  explicit fpromise(nullptr_t) noexcept : __state_(nullptr) {}

  template <class> friend class fpackaged_task;

public:
  fpromise();
  template <class _Allocator> fpromise(std::allocator_arg_t, const _Allocator &__a);

  fpromise(fpromise &&__rhs) noexcept : __state_(__rhs.__state_) { __rhs.__state_ = nullptr; }
  fpromise(const fpromise &__rhs) = delete;
  ~fpromise();

  // assignment

  fpromise &operator=(fpromise &&__rhs) noexcept {
    fpromise(std::move(__rhs)).swap(*this);
    return *this;
  }
  fpromise &operator=(const fpromise &__rhs) = delete;

  void swap(fpromise &__rhs) noexcept { std::swap(__state_, __rhs.__state_); }

  // retrieving the result
  ffuture<_Rp &> get_future();

  // setting the result
  void set_value(_Rp &__r);

// setting the result with deferred notification
#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
  void set_value_at_thread_exit(_Rp &);
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS
};

template <class _Rp> fpromise<_Rp &>::fpromise() : __state_(new _fassoc_state<_Rp &>) {}

template <class _Rp>
template <class _Alloc>
fpromise<_Rp &>::fpromise(std::allocator_arg_t, const _Alloc &__a0) {
  typedef _fassoc_state_alloc<_Rp &, _Alloc> _State;
  typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<_State> _A2;
  typedef _fallocator_destructor<_A2> _D2;
  _A2 __a(__a0);
  std::unique_ptr<_State, _D2> __hold(__a.allocate(1), _D2(__a, 1));
  ::new ((void *)std::addressof(*__hold.get())) _State(__a0);
  __state_ = std::addressof(*__hold.release());
}

template <class _Rp> fpromise<_Rp &>::~fpromise() {
  if (__state_) {
    if (!__state_->__has_value() && __state_->use_count() > 1)
      __state_->__make_abandoned();
    __state_->__release_shared();
  }
}

template <class _Rp> ffuture<_Rp &> fpromise<_Rp &>::get_future() {
  if (__state_ == nullptr)
    std::abort();
  return ffuture<_Rp &>(__state_);
}

template <class _Rp> void fpromise<_Rp &>::set_value(_Rp &__r) {
  if (__state_ == nullptr)
    std::abort();
  __state_->set_value(__r);
}

#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
template <class _Rp> void fpromise<_Rp &>::set_value_at_thread_exit(_Rp &__r) {
  if (__state_ == nullptr)
    std::abort();
  __state_->set_value_at_thread_exit(__r);
}
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS

// fpromise<void>

template <> class fpromise<void> {
  _fassoc_sub_state *__state_;

  explicit fpromise(nullptr_t) noexcept : __state_(nullptr) {}

  template <class> friend class fpackaged_task;

public:
  fpromise();
  template <class _Allocator> fpromise(std::allocator_arg_t, const _Allocator &__a);

  fpromise(fpromise &&__rhs) noexcept : __state_(__rhs.__state_) { __rhs.__state_ = nullptr; }
  fpromise(const fpromise &__rhs) = delete;
  ~fpromise();

  // assignment
  fpromise &operator=(fpromise &&__rhs) noexcept {
    fpromise(std::move(__rhs)).swap(*this);
    return *this;
  }
  fpromise &operator=(const fpromise &__rhs) = delete;

  void swap(fpromise &__rhs) noexcept { std::swap(__state_, __rhs.__state_); }

  // retrieving the result
  ffuture<void> get_future();

  // setting the result
  void set_value();

// setting the result with deferred notification
#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
  void set_value_at_thread_exit();
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS
};

template <class _Alloc> fpromise<void>::fpromise(std::allocator_arg_t, const _Alloc &__a0) {
  typedef _fassoc_sub_state_alloc<_Alloc> _State;
  typedef typename std::allocator_traits<_Alloc>::template rebind_alloc<_State> _A2;
  typedef _fallocator_destructor<_A2> _D2;
  _A2 __a(__a0);
  std::unique_ptr<_State, _D2> __hold(__a.allocate(1), _D2(__a, 1));
  ::new ((void *)std::addressof(*__hold.get())) _State(__a0);
  __state_ = std::addressof(*__hold.release());
}

// fpackaged_task

template <class _Rp, class... _ArgTypes> class fpackaged_task<_Rp(_ArgTypes...)> {
public:
  typedef _Rp result_type; // extension

private:
  _fpackaged_task_function<result_type(_ArgTypes...)> __f_;
  fpromise<result_type> __p_;

public:
  // construction and destruction
  fpackaged_task() noexcept : __p_(nullptr) {}

  template <class _Fp,
            std::enable_if_t<!std::is_same<std::remove_cv_t<std::remove_reference_t<_Fp>>,
                                           fpackaged_task>::value,
                             int> = 0>
  explicit fpackaged_task(_Fp &&__f) : __f_(std::forward<_Fp>(__f)) {}

#if _LIBCPP_STD_VER <= 14
  template <class _Fp, class _Allocator,
            std::enable_if_t<!std::is_same<std::remove_cv_t<std::remove_reference_t<_Fp>>,
                                           fpackaged_task>::value,
                             int> = 0>
  fpackaged_task(std::allocator_arg_t, const _Allocator &__a, _Fp &&__f)
      : __f_(std::allocator_arg_t(), __a, std::forward<_Fp>(__f)),
        __p_(std::allocator_arg_t(), __a) {}
#endif
  // ~fpackaged_task() = default;

  // no copy
  fpackaged_task(const fpackaged_task &) = delete;
  fpackaged_task &operator=(const fpackaged_task &) = delete;

  // move support
  fpackaged_task(fpackaged_task &&__other) noexcept
      : __f_(std::move(__other.__f_)), __p_(std::move(__other.__p_)) {}
  fpackaged_task &operator=(fpackaged_task &&__other) noexcept {
    __f_ = std::move(__other.__f_);
    __p_ = std::move(__other.__p_);
    return *this;
  }
  void swap(fpackaged_task &__other) noexcept {
    __f_.swap(__other.__f_);
    __p_.swap(__other.__p_);
  }

  bool valid() const noexcept { return __p_.__state_ != nullptr; }

  // result retrieval
  ffuture<result_type> get_future() { return __p_.get_future(); }

  // execution
  void operator()(_ArgTypes... __args);
#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
  void make_ready_at_thread_exit(_ArgTypes... __args);
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS

  void reset();
};

template <class _Rp, class... _ArgTypes>
void fpackaged_task<_Rp(_ArgTypes...)>::operator()(_ArgTypes... __args) {
  if (__p_.__state_ == nullptr || __p_.__state_->__has_value())
    std::abort();
  __p_.set_value(__f_(std::forward<_ArgTypes>(__args)...));
}

#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
template <class _Rp, class... _ArgTypes>
void fpackaged_task<_Rp(_ArgTypes...)>::make_ready_at_thread_exit(_ArgTypes... __args) {
  if (__p_.__state_ == nullptr || __p_.__state_->__has_value())
    std::abort();
  __p_.set_value_at_thread_exit(__f_(std::forward<_ArgTypes>(__args)...));
}
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS

template <class _Rp, class... _ArgTypes> void fpackaged_task<_Rp(_ArgTypes...)>::reset() {
  if (!valid())
    std::abort();
  __p_ = fpromise<result_type>();
}

template <class... _ArgTypes> class fpackaged_task<void(_ArgTypes...)> {
public:
  typedef void result_type; // extension

private:
  _fpackaged_task_function<result_type(_ArgTypes...)> __f_;
  fpromise<result_type> __p_;

public:
  // construction and destruction
  fpackaged_task() noexcept : __p_(nullptr) {}
  template <class _Fp,
            std::enable_if_t<!std::is_same<std::remove_cv_t<std::remove_reference_t<_Fp>>,
                                           fpackaged_task>::value,
                             int> = 0>
  explicit fpackaged_task(_Fp &&__f) : __f_(std::forward<_Fp>(__f)) {}
#if _LIBCPP_STD_VER <= 14
  template <class _Fp, class _Allocator,
            std::enable_if_t<!std::is_same<std::remove_cv_t<std::remove_reference_t<_Fp>>,
                                           fpackaged_task>::value,
                             int> = 0>
  fpackaged_task(std::allocator_arg_t, const _Allocator &__a, _Fp &&__f)
      : __f_(std::allocator_arg_t(), __a, std::forward<_Fp>(__f)),
        __p_(std::allocator_arg_t(), __a) {}
#endif
  // ~fpackaged_task() = default;

  // no copy
  fpackaged_task(const fpackaged_task &) = delete;
  fpackaged_task &operator=(const fpackaged_task &) = delete;

  // move support
  fpackaged_task(fpackaged_task &&__other) noexcept
      : __f_(std::move(__other.__f_)), __p_(std::move(__other.__p_)) {}
  fpackaged_task &operator=(fpackaged_task &&__other) noexcept {
    __f_ = std::move(__other.__f_);
    __p_ = std::move(__other.__p_);
    return *this;
  }
  void swap(fpackaged_task &__other) noexcept {
    __f_.swap(__other.__f_);
    __p_.swap(__other.__p_);
  }

  bool valid() const noexcept { return __p_.__state_ != nullptr; }

  // result retrieval
  ffuture<result_type> get_future() { return __p_.get_future(); }

  // execution
  void operator()(_ArgTypes... __args);
#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
  void make_ready_at_thread_exit(_ArgTypes... __args);
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS

  void reset();
};

#if _LIBCPP_STD_VER >= 17

template <class _Rp, class... _Args>
fpackaged_task(_Rp (*)(_Args...)) -> fpackaged_task<_Rp(_Args...)>;

template <class _Fp, class _Stripped = typename _fstrip_signature<decltype(&_Fp::operator())>::type>
fpackaged_task(_Fp) -> fpackaged_task<_Stripped>;

#endif

template <class... _ArgTypes>
void fpackaged_task<void(_ArgTypes...)>::operator()(_ArgTypes... __args) {
  if (__p_.__state_ == nullptr || __p_.__state_->__has_value())
    std::abort();
  __f_(std::forward<_ArgTypes>(__args)...);
  __p_.set_value();
}

#if defined(TRAA_BASE_THREAD_FFUTURE_USE_TLS)
template <class... _ArgTypes>
void fpackaged_task<void(_ArgTypes...)>::make_ready_at_thread_exit(_ArgTypes... __args) {
  if (__p_.__state_ == nullptr || __p_.__state_->__has_value())
    std::abort();
  __f_(std::forward<_ArgTypes>(__args)...);
  __p_.set_value_at_thread_exit();
}
#endif // TRAA_BASE_THREAD_FFUTURE_USE_TLS

template <class... _ArgTypes> void fpackaged_task<void(_ArgTypes...)>::reset() {
  if (!valid())
    std::abort();
  __p_ = fpromise<result_type>();
}

// fshared_future

template <class _Rp> class fshared_future {
  _fassoc_state<_Rp> *__state_;

public:
  fshared_future() noexcept : __state_(nullptr) {}

  fshared_future(const fshared_future &__rhs) noexcept : __state_(__rhs.__state_) {
    if (__state_)
      __state_->__add_shared();
  }

  fshared_future(ffuture<_Rp> &&__f) noexcept : __state_(__f.__state_) { __f.__state_ = nullptr; }

  fshared_future(fshared_future &&__rhs) noexcept : __state_(__rhs.__state_) {
    __rhs.__state_ = nullptr;
  }
  ~fshared_future();
  fshared_future &operator=(const fshared_future &__rhs) noexcept;

  fshared_future &operator=(fshared_future &&__rhs) noexcept {
    fshared_future(std::move(__rhs)).swap(*this);
    return *this;
  }

  // retrieving the value
  const _Rp &get(const _Rp &default_value) const { return __state_->copy(default_value); }

  void swap(fshared_future &__rhs) noexcept { std::swap(__state_, __rhs.__state_); }

  // functions to check state
  bool valid() const noexcept { return __state_ != nullptr && !__state_->__is_abandoned(); }

  void wait() const { __state_->wait(); }

  template <class _Rep, class _Period>
  ffuture_status wait_for(const std::chrono::duration<_Rep, _Period> &__rel_time) const {
    return __state_->wait_for(__rel_time);
  }

  template <class _Clock, class _Duration>
  ffuture_status wait_until(const std::chrono::time_point<_Clock, _Duration> &__abs_time) const {
    return __state_->wait_until(__abs_time);
  }
};

template <class _Rp> fshared_future<_Rp>::~fshared_future() {
  if (__state_)
    __state_->__release_shared();
}

template <class _Rp>
fshared_future<_Rp> &fshared_future<_Rp>::operator=(const fshared_future &__rhs) noexcept {
  if (__rhs.__state_)
    __rhs.__state_->__add_shared();
  if (__state_)
    __state_->__release_shared();
  __state_ = __rhs.__state_;
  return *this;
}

template <class _Rp> class fshared_future<_Rp &> {
  _fassoc_state<_Rp &> *__state_;

public:
  fshared_future() noexcept : __state_(nullptr) {}

  fshared_future(const fshared_future &__rhs) : __state_(__rhs.__state_) {
    if (__state_)
      __state_->__add_shared();
  }

  fshared_future(ffuture<_Rp &> &&__f) noexcept : __state_(__f.__state_) { __f.__state_ = nullptr; }

  fshared_future(fshared_future &&__rhs) noexcept : __state_(__rhs.__state_) {
    __rhs.__state_ = nullptr;
  }
  ~fshared_future();
  fshared_future &operator=(const fshared_future &__rhs);

  fshared_future &operator=(fshared_future &&__rhs) noexcept {
    fshared_future(std::move(__rhs)).swap(*this);
    return *this;
  }

  // retrieving the value
  _Rp &get(_Rp &default_value) const { return __state_->copy(default_value); }

  void swap(fshared_future &__rhs) noexcept { std::swap(__state_, __rhs.__state_); }

  // functions to check state
  bool valid() const noexcept { return __state_ != nullptr && !__state_->__is_abandoned(); }

  void wait() const { __state_->wait(); }
  template <class _Rep, class _Period>

  ffuture_status wait_for(const std::chrono::duration<_Rep, _Period> &__rel_time) const {
    return __state_->wait_for(__rel_time);
  }
  template <class _Clock, class _Duration>

  ffuture_status wait_until(const std::chrono::time_point<_Clock, _Duration> &__abs_time) const {
    return __state_->wait_until(__abs_time);
  }
};

template <class _Rp> fshared_future<_Rp &>::~fshared_future() {
  if (__state_)
    __state_->__release_shared();
}

template <class _Rp>
fshared_future<_Rp &> &fshared_future<_Rp &>::operator=(const fshared_future &__rhs) {
  if (__rhs.__state_)
    __rhs.__state_->__add_shared();
  if (__state_)
    __state_->__release_shared();
  __state_ = __rhs.__state_;
  return *this;
}

template <> class fshared_future<void> {
  _fassoc_sub_state *__state_;

public:
  fshared_future() noexcept : __state_(nullptr) {}

  fshared_future(const fshared_future &__rhs) : __state_(__rhs.__state_) {
    if (__state_)
      __state_->__add_shared();
  }

  fshared_future(ffuture<void> &&__f) noexcept : __state_(__f.__state_) { __f.__state_ = nullptr; }

  fshared_future(fshared_future &&__rhs) noexcept : __state_(__rhs.__state_) {
    __rhs.__state_ = nullptr;
  }
  ~fshared_future();
  fshared_future &operator=(const fshared_future &__rhs);

  fshared_future &operator=(fshared_future &&__rhs) noexcept {
    fshared_future(std::move(__rhs)).swap(*this);
    return *this;
  }

  // retrieving the value
  void get() const { __state_->copy(); }

  void swap(fshared_future &__rhs) noexcept { std::swap(__state_, __rhs.__state_); }

  // functions to check state
  bool valid() const noexcept { return __state_ != nullptr && !__state_->__is_abandoned(); }

  void wait() const { __state_->wait(); }

  template <class _Rep, class _Period>
  ffuture_status wait_for(const std::chrono::duration<_Rep, _Period> &__rel_time) const {
    return __state_->wait_for(__rel_time);
  }

  template <class _Clock, class _Duration>
  ffuture_status wait_until(const std::chrono::time_point<_Clock, _Duration> &__abs_time) const {
    return __state_->wait_until(__abs_time);
  }
};

template <class _Rp> inline void swap(fshared_future<_Rp> &__x, fshared_future<_Rp> &__y) noexcept {
  __x.swap(__y);
}

template <class _Rp> inline fshared_future<_Rp> ffuture<_Rp>::share() noexcept {
  return fshared_future<_Rp>(std::move(*this));
}

template <class _Rp> inline fshared_future<_Rp &> ffuture<_Rp &>::share() noexcept {
  return fshared_future<_Rp &>(std::move(*this));
}

inline fshared_future<void> ffuture<void>::share() noexcept {
  return fshared_future<void>(std::move(*this));
}

} // namespace base
} // namespace traa

namespace std {
template <class _Rp>
inline void swap(traa::base::ffuture<_Rp> &__x, traa::base::ffuture<_Rp> &__y) noexcept {
  __x.swap(__y);
}

template <class _Rp>
inline void swap(traa::base::fpromise<_Rp> &__x, traa::base::fpromise<_Rp> &__y) noexcept {
  __x.swap(__y);
}

template <class _Rp, class _Alloc>
struct uses_allocator<traa::base::fpromise<_Rp>, _Alloc> : public true_type {};

template <class _Rp, class... _ArgTypes>
inline void swap(traa::base::fpackaged_task<_Rp(_ArgTypes...)> &__x,
                 traa::base::fpackaged_task<_Rp(_ArgTypes...)> &__y) noexcept {
  __x.swap(__y);
}

#if _LIBCPP_STD_VER <= 14
template <class _Callable, class _Alloc>
struct uses_allocator<traa::base::fpackaged_task<_Callable>, _Alloc> : public true_type {};
#endif
} // namespace std

#endif // TRAA_BASE_THREAD_FFUTURE_H_