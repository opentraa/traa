#ifndef TRAA_BASE_THREAD_CALLBACK_H_
#define TRAA_BASE_THREAD_CALLBACK_H_

#include <functional>
#include <memory>

namespace traa {
namespace base {

class anonymous_flag {};

/**
 * @brief A class template for creating weak callbacks.
 *
 * This class template allows creating weak callbacks that can be used to invoke a function or
 * member function on an object, while ensuring that the object is still valid. It is particularly
 * useful in scenarios where the object may be destroyed before the callback is invoked.
 *
 * @tparam T The type of the callback function or member function.
 */
template <typename T> class weak_callback {
public:
  /**
   * @brief Default constructor.
   */
  weak_callback() = default;

  /**
   * @brief Constructs a weak callback with a weak flag and a callback object.
   *
   * @param weak_flag The weak flag that represents the validity of the object.
   * @param t The callback object.
   */
  weak_callback(const std::weak_ptr<anonymous_flag> &weak_flag, const T &t)
      : weak_flag_(weak_flag), t_(t) {}

  /**
   * @brief Constructs a weak callback with a weak flag and a callback object.
   *
   * This constructor takes an rvalue reference to the callback object, allowing move semantics.
   *
   * @param weak_flag The weak flag that represents the validity of the object.
   * @param t The callback object.
   */
  weak_callback(const std::weak_ptr<anonymous_flag> &weak_flag, T &&t)
      : weak_flag_(weak_flag), t_(std::move(t)) {}

  /**
   * @brief Constructs a weak callback from another weak callback.
   *
   * This constructor allows constructing a weak callback from another weak callback object.
   *
   * @tparam _T The type of the other weak callback.
   * @param callback The other weak callback object.
   */
  template <class _T>
  weak_callback(const _T &callback) : weak_flag_(callback.weak_flag_), t_(callback.t_) {}

  /**
   * @brief Invokes the callback with the given arguments.
   *
   * This operator allows invoking the callback with the given arguments. It checks if the weak flag
   * is still valid before invoking the callback. If the weak flag is expired, it returns a
   * default-constructed value.
   *
   * @tparam Args The types of the arguments.
   * @param args The arguments to be passed to the callback.
   * @return The result of invoking the callback with the given arguments, or a default-constructed
   * value if the weak flag is expired.
   */
  template <class... Args> auto operator()(Args &&...args) const {
    if (!weak_flag_.expired()) {
      return t_(std::forward<Args>(args)...);
    }
    return decltype(t_(std::forward<Args>(args)...))();
  }

  /**
   * @brief Checks if the weak flag is expired.
   *
   * @return true if the weak flag is expired, false otherwise.
   */
  bool is_expired() const { return weak_flag_.expired(); }

  std::weak_ptr<anonymous_flag>
      weak_flag_; /**< The weak flag that represents the validity of the object. */
  mutable T t_;   /**< The callback object. */
};

/**
 * @class support_weak_callback
 * @brief A base class for objects that support weak callbacks.
 *
 * The support_weak_callback class provides a mechanism for creating weak callbacks
 * that can be used to invoke member functions on objects without keeping strong
 * references to those objects. It also provides a shared flag that can be used to
 * check if the object has been destroyed.
 */
class support_weak_callback {
public:
  /**
   * Destructor.
   */
  virtual ~support_weak_callback(){};

  /**
   * Converts a closure to a weak callback.
   *
   * @tparam T The type of the closure.
   * @param closure The closure to convert.
   * @return A weak_callback object that wraps the closure.
   */
  template <typename T> auto to_weak_callback(const T &closure) -> weak_callback<T> {
    return weak_callback<T>(get_weak_flags(), closure);
  }

  /**
   * Retrieves the shared weak flag.
   *
   * @return A weak_ptr to the shared flag.
   */
  std::weak_ptr<anonymous_flag> get_weak_flags() {
    if (shared_flags_.use_count() == 0) {
      shared_flags_.reset((anonymous_flag *)NULL);
    }
    return shared_flags_;
  }

private:
  /**
   * Converts a strong callback to a weak callback.
   *
   * @tparam R The return type of the callback.
   * @tparam Args The argument types of the callback.
   * @tparam Flag The type of the weak flag.
   * @param callback The strong callback to convert.
   * @param expired_flag The weak flag to check if the object has been destroyed.
   * @return A weak callback that wraps the strong callback.
   */
  template <typename R, typename... Args, typename Flag>
  static std::function<R(Args...)>
  convert_to_weak_callback(const std::function<R(Args...)> &callback,
                           std::weak_ptr<Flag> expired_flag) {
    auto weak_back = [expired_flag, callback](Args... args) {
      if (!expired_flag.expired()) {
        return callback(args...);
      }
      return R();
    };

    return weak_back;
  }

protected:
  std::shared_ptr<anonymous_flag> shared_flags_;
};

/**
 * @class weak_callback_flag
 * @brief A class that represents a weak callback flag.
 *
 * This class is used to manage weak callbacks. It inherits from support_weak_callback
 * and provides functionality to cancel the callback and check if it has been used.
 */
class weak_callback_flag final : public support_weak_callback {
public:
  /**
   * @brief Cancels the callback.
   *
   * This function cancels the callback by resetting the shared flags.
   */
  void cancel() { shared_flags_.reset(); }

  /**
   * @brief Checks if the callback has been used.
   *
   * @return true if the callback has been used, false otherwise.
   */
  bool has_used() { return shared_flags_.use_count() != 0; }
};

/**
 * Creates a weak callback function that can be used to invoke a non-member function or a static
 * member function.
 *
 * @tparam F The type of the function to be called.
 * @tparam Args The types of the arguments to be passed to the function.
 * @param f The function to be called.
 * @param args The arguments to be passed to the function.
 * @return A weak callback function that can be used to invoke the specified function with the given
 * arguments.
 */
template <class F, class... Args,
          class = typename std::enable_if<!std::is_member_function_pointer<F>::value>::type>
auto make_weak_callback(F &&f, Args &&...args) -> decltype(std::bind(f, args...)) {
  return std::bind(f, args...);
}

/**
 * Creates a weak callback object that wraps a const member function pointer and its associated
 * object. The weak callback object can be used to invoke the member function even if the associated
 * object has been destroyed.
 *
 * @tparam R The return type of the member function.
 * @tparam C The class type that the member function belongs to.
 * @tparam DArgs The argument types of the member function.
 * @tparam P The type of the associated object.
 * @tparam Args The argument types for the member function call.
 * @param f The member function pointer.
 * @param p The associated object.
 * @param args The arguments for the member function call.
 * @return A weak callback object that wraps the member function and its associated object.
 */
template <class R, class C, class... DArgs, class P, class... Args>
auto make_weak_callback(R (C::*f)(DArgs...) const, P &&p, Args &&...args)
    -> weak_callback<decltype(std::bind(f, p, args...))> {
  std::weak_ptr<anonymous_flag> weak_flag = ((support_weak_callback *)p)->get_weak_flags();
  auto bind_obj = std::bind(f, p, args...);
  static_assert(std::is_base_of<support_weak_callback, C>::value,
                "support_weak_callback should be base of C");
  weak_callback<decltype(bind_obj)> callback(weak_flag, std::move(bind_obj));
  return callback;
}

/**
 * Creates a weak callback object that wraps a non-const member function pointer and its associated
 * object. The weak callback object can be used to invoke the member function even if the associated
 * object has been destroyed.
 *
 * @tparam R The return type of the member function.
 * @tparam C The class type that the member function belongs to.
 * @tparam DArgs The argument types of the member function.
 * @tparam P The type of the associated object.
 * @tparam Args The argument types for the associated object's member function.
 * @param f A pointer to the member function.
 * @param p The associated object.
 * @param args The arguments for the member function.
 * @return A weak callback object that wraps the member function and associated object.
 */
template <class R, class C, class... DArgs, class P, class... Args>
auto make_weak_callback(R (C::*f)(DArgs...), P &&p, Args &&...args)
    -> weak_callback<decltype(std::bind(f, p, args...))> {
  std::weak_ptr<anonymous_flag> weak_flag = ((support_weak_callback *)p)->get_weak_flags();
  auto bind_obj = std::bind(f, p, args...);
  static_assert(std::is_base_of<support_weak_callback, C>::value,
                "support_weak_callback should be base of C");
  weak_callback<decltype(bind_obj)> callback(weak_flag, std::move(bind_obj));
  return callback;
}

} // namespace base
} // namespace traa

#endif // TRAA_BASE_THREAD_CALLBACK_H_