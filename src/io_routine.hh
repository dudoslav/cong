#pragma once

#include <coroutine>

namespace cong {

class io_promise;

class io_routine : public std::coroutine_handle<io_promise> {
public:
  using promise_type = io_promise;
};

class io_promise {
public:
  constexpr auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  constexpr auto final_suspend() const noexcept -> std::suspend_never {
    return {};
  }

  constexpr auto return_void() const noexcept -> std::suspend_always {
    return {};
  }

  constexpr auto get_return_object() noexcept -> io_routine { return {io_routine::from_promise(*this)}; }

  constexpr auto unhandled_exception() const noexcept -> void {}
};

} // namespace cong
