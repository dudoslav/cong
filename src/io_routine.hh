#pragma once

#include <coroutine>

namespace cong {

template <typename T> class io_promise;
template <typename T> class io_routine;

template <std::same_as<void> T>
class io_routine<T> : public std::coroutine_handle<io_promise<void>> {
public:
  using promise_type = io_promise<void>;

  constexpr auto await_ready() const noexcept { return this->done(); }

  constexpr void await_suspend(std::coroutine_handle<> coroutine) noexcept;

  constexpr void await_resume() const noexcept {}
};

template <typename T>
class io_routine : public std::coroutine_handle<io_promise<T>> {
public:
  using promise_type = io_promise<T>;
  using value_type = T;

  constexpr auto await_ready() const noexcept { return this->done(); }

  constexpr void await_suspend(std::coroutine_handle<> coroutine) noexcept;

  constexpr value_type await_resume() const noexcept {
    return std::move(this->promise().data_);
  }
};

template <typename Derived> class io_promise_base {
public:
  constexpr auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  constexpr auto final_suspend() const noexcept {
    struct awaiter {
      constexpr bool await_ready() const noexcept { return false; }

      constexpr auto
      await_suspend(std::coroutine_handle<Derived> coroutine) noexcept
          -> std::coroutine_handle<> {
        return coroutine.promise().precursor_;
      }

      constexpr void await_resume() const noexcept {}
    };

    return awaiter{};
  }

  constexpr auto unhandled_exception() const noexcept -> void {}

protected:
  std::coroutine_handle<> precursor_ = std::noop_coroutine();
};

template <> class io_promise<void> : public io_promise_base<io_promise<void>> {
public:
  friend class io_routine<void>;

  constexpr void return_void() const noexcept {}

  constexpr auto get_return_object() noexcept -> io_routine<void> {
    return {io_routine<void>::from_promise(*this)};
  }
};

template <typename T> class io_promise : public io_promise_base<io_promise<T>> {
public:
  friend class io_routine<T>;

  using value_type = T;

  constexpr auto return_value(value_type value) noexcept -> void {
    data_ = std::move(value);
  }

  constexpr auto get_return_object() noexcept -> io_routine<value_type> {
    return {io_routine<value_type>::from_promise(*this)};
  }

private:
  value_type data_;
};

template <std::same_as<void> T>
constexpr void
io_routine<T>::await_suspend(std::coroutine_handle<> coroutine) noexcept {
  /// Initially suspended, we need to resume, does this make sense?
  /// What if the coroutine we want to await is already after initial suspend?
  this->resume();

  this->promise().precursor_ = coroutine;
}

template <typename T>
constexpr void
io_routine<T>::await_suspend(std::coroutine_handle<> coroutine) noexcept {
  /// Initially suspended, we need to resume, does this make sense?
  /// What if the coroutine we want to await is already after initial suspend?
  this->resume();

  this->promise().precursor_ = coroutine;
}

} // namespace cong
