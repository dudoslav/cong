#pragma once

#include <coroutine>

namespace cong {

class io_promise;

class io_routine : public std::coroutine_handle<io_promise> {
public:
  using promise_type = io_promise;

  auto await_ready() const noexcept { return done(); }

  void await_suspend(std::coroutine_handle<> coroutine) noexcept;

  void await_resume() const noexcept {}
};

class io_promise {
public:
  friend class io_routine;

  constexpr auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  constexpr auto final_suspend() const noexcept {
    struct awaiter {
      bool await_ready() const noexcept {
        return false;
      }

      auto await_suspend(std::coroutine_handle<io_promise> coroutine) noexcept -> std::coroutine_handle<> {
        return coroutine.promise().precursor_;
      }

      void await_resume() const noexcept {}
    };

    return awaiter{};
  }

  constexpr auto return_void() const noexcept -> std::suspend_always {
    return {};
  }

  constexpr auto get_return_object() noexcept -> io_routine {
    return {io_routine::from_promise(*this)};
  }

  constexpr auto unhandled_exception() const noexcept -> void {}

private:
  std::coroutine_handle<> precursor_ = std::noop_coroutine();
};

inline void io_routine::await_suspend(std::coroutine_handle<> coroutine) noexcept {
  /// Initially suspended, we need to resume, does this make sense?
  resume();

  promise().precursor_ = coroutine;
}

} // namespace cong
