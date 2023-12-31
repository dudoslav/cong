#pragma once

#include <utility>
#include <optional>

namespace cong {

template <typename T, typename R, R (*Deleter)(T)> class resource {
public:
  using value_type = T;

  explicit resource() noexcept = default;

  explicit resource(value_type handle) noexcept : handle_{handle} {}

  /// Copy
  resource(resource const &other) = delete;
  auto operator=(resource const &other) -> resource & = delete;

  /// Move
  resource(resource &&other) noexcept
      : handle_{std::exchange(other.handle_, std::nullopt)} {}
  auto operator=(resource &&other) noexcept -> resource & {
    handle_ = std::exchange(other.handle_, std::nullopt);

    return *this;
  }

  ~resource() noexcept {
    if (handle_) {
      /// Sadly we can not use the return value
      Deleter(*handle_);
    }
  }

  [[nodiscard]] auto operator*() const noexcept -> value_type {
    /// I never know when to make these getters const/non-const
    return *handle_;
  }

private:
  std::optional<value_type> handle_;
};

using file_descriptor = cong::resource<int, int, close>;

} // namespace cong