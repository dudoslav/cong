#pragma once

#include <expected>
#include <memory>

#include "io_awaiter.hh"
#include "io_routine.hh"

#include <resource.hh>

namespace cong {

class io_context {
public:
  ~io_context() noexcept;

  [[nodiscard]] static auto make_io_context(std::size_t size,
                                            int flags = 0u) noexcept
      -> std::expected<io_context, std::error_code>;

  void spawn(io_routine work) noexcept;

  auto execute() noexcept -> std::expected<void, std::error_code>;

  auto sq_count() const noexcept -> std::size_t;

  auto cq_count() const noexcept -> std::size_t;

  /// OPERATIONS

  auto read(file_descriptor const &fd, std::byte *dst, std::size_t count,
            std::size_t offset) noexcept -> io_awaiter;

  auto write(file_descriptor const &fd, std::byte const *src, std::size_t count,
            std::size_t offset) noexcept -> io_awaiter;

private:
  class impl;

  std::unique_ptr<impl> impl_;

  io_context() noexcept;

  io_context(io_context const &) = delete;
  io_context &operator=(io_context const &) = delete;

  io_context(io_context &&other) noexcept = default;
  io_context &operator=(io_context &&other) noexcept = default;
};

} // namespace cong