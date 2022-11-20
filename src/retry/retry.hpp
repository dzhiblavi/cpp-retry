#ifndef LIB_RETRY_H
#define LIB_RETRY_H

#include <exception>
#include <type_traits>

#include "src/retry/error_handler.hpp"
#include "src/retry/final_error_handler.hpp"

namespace retry {

template <typename F, typename... Options>
std::invoke_result_t<F> run(const F& func, int max_retries, const Options&... options) {
  for (int retry = 0; retry <= max_retries; ++retry) {
    try {
      return func();
    } catch (...) {
      std::exception_ptr exception = std::current_exception();
      if (retry == max_retries) {
        return detail_::handle_final_error<std::invoke_result_t<F>>(exception, options...);
      } else {
        detail_::handle_error(exception, options...);
      }
    }
  }
  __builtin_unreachable();
}

}  // namespace retry

#endif  // LIB_RETRY_H

