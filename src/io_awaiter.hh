#pragma once

#include <coroutine>
#include <functional>

namespace cong {

class io_awaiter {
public:
  explicit io_awaiter(auto &&prepare) noexcept
      : prepare_(std::forward<decltype(prepare)>(prepare)) {}

  auto await_ready() const noexcept -> bool { return false; }

  void await_suspend(std::coroutine_handle<> coroutine) const noexcept {
    /// TODO: In the future remove this std::function...
    prepare_(coroutine.address());
  }

  void await_resume() const noexcept {}

private:
  std::function<void(void *)> prepare_;
};

} // namespace cong