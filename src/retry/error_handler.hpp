#ifndef LIB_RETRY_ERROR_HANDLER_HPP
#define LIB_RETRY_ERROR_HANDLER_HPP

#include <exception>
#include <chrono>
#include <concepts>
#include <thread>

#include <iostream>

namespace retry {

template <typename SelfType>
class error_handler {
 public:
  void handle(std::exception_ptr& error) const
    noexcept(noexcept(self().handle(error))) { self().handle(error); }

 private:
  const SelfType& self() const noexcept { return *static_cast<const SelfType*>(this); }
};

template <typename Rep, typename Period>
class wait_interval : public error_handler<wait_interval<Rep, Period>> {
 public:
  explicit wait_interval(const std::chrono::duration<Rep, Period>& dur) : dur_(dur) {}

  void handle(std::exception_ptr&) const noexcept { std::this_thread::sleep_for(dur_); }

 private:
  std::chrono::duration<Rep, Period> dur_;
};

namespace detail_ {

void handle_error(std::exception_ptr&) noexcept;

template <typename Handler> requires (!std::is_base_of_v<error_handler<Handler>, Handler>)
void handle_error(std::exception_ptr& error, const Handler&) {}

template <typename Handler>
void handle_error(std::exception_ptr& error, const error_handler<Handler>& handler) 
    noexcept(noexcept(handler.handle(error))) { handler.handle(error); }

template <typename Handler, typename... Handlers>
void handle_error(
    std::exception_ptr& error, const error_handler<Handler>& handler, const Handlers&... handlers) {
  handle_error<Handler>(error, handler);
  handle_error(error, handlers...);
}

}  // namespace detail_

}  // namespace retry

#endif  // LIB_RETRY_ERROR_HANDLER_HPP

