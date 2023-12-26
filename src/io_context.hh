#pragma once

#include <expected>
#include <memory>

#include <liburing.h>

#include "io_awaiter.hh"
#include "io_routine.hh"
#include "resource.hh"
#include "utility.hh"

namespace cong {

template <typename T> using io_result = std::expected<T, std::error_code>;

class io_context {
public:
  ~io_context() noexcept;

  [[nodiscard]] static auto make_io_context(std::size_t size,
                                            int flags = 0u) noexcept
      -> io_result<io_context>;

  void spawn(io_routine<auto> work) noexcept {
    enqueue(work.address(), io_uring_prep_nop);
  }

  auto execute() noexcept -> io_result<void>;

  auto sq_count() const noexcept -> std::size_t;

  auto cq_count() const noexcept -> std::size_t;

  /// OPERATIONS

  auto read(file_descriptor const &fd, std::byte *dst, std::size_t count,
            std::size_t offset) noexcept -> io_awaiter;

  auto write(file_descriptor const &fd, std::byte const *src, std::size_t count,
             std::size_t offset) noexcept -> io_awaiter;

private:
  io_uring io_uring_{.ring_fd = -1};

  io_context() noexcept = default;

  io_context(io_context const &) = delete;
  io_context &operator=(io_context const &) = delete;

  io_context(io_context &&other) noexcept;
  io_context &operator=(io_context &&other) noexcept;

  [[nodiscard]] auto get_sqe() noexcept -> io_result<io_uring_sqe *>;

  auto enqueue(auto data, auto &&fn, auto &&...args) noexcept
      -> io_result<void> {
    static_assert(sizeof(data) <= sizeof(void *));

    auto sqe = get_sqe();

    if (not sqe) {
      return std::unexpected(sqe.error());
    }

    fn(*sqe, std::forward<decltype(args)>(args)...);
    io_uring_sqe_set_data(*sqe, data);

    return try_call_neg(io_uring_submit, &io_uring_).transform([](auto) {});
  }
};

} // namespace cong