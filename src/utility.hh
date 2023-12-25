#pragma once

namespace cong {

[[nodiscard]] auto try_call(auto &&fun, auto &&...args) noexcept
    -> std::expected<int, std::error_code> {
  auto const res = fun(std::forward<decltype(args)>(args)...);

  if (res < 0) {
    return std::unexpected{std::error_code{errno, std::system_category()}};
  }

  return res;
}

[[nodiscard]] auto try_call_neg(auto &&fun, auto &&...args) noexcept
    -> std::expected<int, std::error_code> {
  auto const res = fun(std::forward<decltype(args)>(args)...);

  if (res < 0) {
    return std::unexpected{std::error_code{-res, std::system_category()}};
  }

  return res;
}

} // namespace cong