#ifndef LIB_RETRY_FINAL_ERROR_HANDLER_HPP
#define LIB_RETRY_FINAL_ERROR_HANDLER_HPP

#include <exception>
#include <concepts>

namespace retry {

template <typename SelfType>
class final_error_handler {
 public:
  template <typename R>
  R handle(std::exception_ptr& error) const noexcept(noexcept(self().template handle<R>(error))) {
    return self().template handle<R>(error);
  }

 private:
  const SelfType& self() const noexcept { return *static_cast<const SelfType*>(this); }
};

class rethrow_last_exception : public final_error_handler<rethrow_last_exception> {
 public:
  template <typename R>
  R handle(std::exception_ptr& error) const noexcept(false) { std::rethrow_exception(error); }
}; 

template <typename R>
class return_default_value : public final_error_handler<return_default_value<R>> {
 public:
  explicit return_default_value(const R& value) : value_(value) {}

  template <typename... Args> requires std::is_constructible_v<R, Args...>
  explicit return_default_value(Args&&... args) : value_(std::forward<Args>(args)...) {}

  template <std::same_as<R> U>
  R handle(std::exception_ptr&) const noexcept { return value_; }

 private:
  R value_;
};

namespace detail_ {

template <typename R, typename Handler, typename... Skip>
R handle_final_error(
    std::exception_ptr& error, const final_error_handler<Handler>& h, const Skip&...) {
  return h.template handle<R>(error);
}

template <typename R, typename Handler, typename... Handlers>
  requires (!std::is_base_of_v<final_error_handler<Handler>, Handler>)
R handle_final_error(std::exception_ptr& error, const Handler& opt, const Handlers&... handlers) {
  return handle_final_error<R>(error, handlers...);
}

}  // namespace detail_

}  // namespace retry

#endif  // LIB_RETRY_FINAL_ERROR_HANDLER_HPP

