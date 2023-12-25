#include <cassert>

#include <liburing.h>

#include "io_context.hh"
#include "utility.hh"

namespace cong {

class io_context::impl {
public:
  // friend class io_awaiter;

  io_uring io_uring_{.ring_fd = -1};

  ~impl() noexcept {
    if (io_uring_.ring_fd != -1) {
      io_uring_queue_exit(&io_uring_);
    }
  }

  auto execute() noexcept -> std::expected<void, std::error_code> {
    auto *cqe = static_cast<io_uring_cqe *>(nullptr);

    return try_call_neg(io_uring_wait_cqe, &io_uring_, &cqe)
        .and_then([&](auto) -> std::expected<void, std::error_code> {
          auto const status = cqe->res;
          auto const coro =
              std::coroutine_handle<>::from_address(io_uring_cqe_get_data(cqe));
          io_uring_cqe_seen(&io_uring_, cqe);

          if (status < 0) {
            return std::unexpected{
                std::error_code{-status, std::system_category()}};
          }

          coro();

          return {};
        });
  }

  [[nodiscard]] auto get_sqe() noexcept
      -> std::expected<io_uring_sqe *, std::error_code> {
    auto *sqe = io_uring_get_sqe(&io_uring_);

    if (not sqe) {
      /// TODO: Is this error OK?
      return std::unexpected{std::make_error_code(std::errc::io_error)};
    }

    return sqe;
  }

  auto enqueue(auto data, auto &&fn, auto &&...args) noexcept
      -> std::expected<void, std::error_code> {
    static_assert(sizeof(data) <= sizeof(void *));

    auto sqe = get_sqe();

    if (not sqe) {
      return std::unexpected(sqe.error());
    }

    fn(*sqe, std::forward<decltype(args)>(args)...);
    io_uring_sqe_set_data(*sqe, data);

    return try_call_neg(io_uring_submit, &io_uring_).transform([](auto) {});
  }

  auto sq_count() const noexcept -> std::size_t {
    return io_uring_sq_ready(&io_uring_);
  }

  auto cq_count() const noexcept -> std::size_t {
    return io_uring_cq_ready(&io_uring_);
  }
};

io_context::io_context() noexcept : impl_{std::make_unique<impl>()} {}
io_context::~io_context() noexcept = default;

auto io_context::make_io_context(std::size_t const size,
                                 int const flags) noexcept
    -> std::expected<io_context, std::error_code> {
  auto io = io_context{};

  return try_call_neg(io_uring_queue_init, size, &io.impl_->io_uring_, flags)
      .transform([&](auto) { return std::move(io); });
}

void io_context::spawn(io_routine work) noexcept {
  assert(impl_);

  impl_->enqueue(work.address(), io_uring_prep_nop);
}

auto io_context::read(file_descriptor const &fd, std::byte *dst,
                      std::size_t count, std::size_t offset) noexcept
    -> io_awaiter {
  assert(impl_);

  return io_awaiter([&fd, dst, count, offset, this](void *data) {
    impl_->enqueue(data, io_uring_prep_read, *fd, dst, count, offset);
  });
}

auto io_context::write(file_descriptor const &fd, std::byte const *src,
                       std::size_t count, std::size_t offset) noexcept
    -> io_awaiter {
  assert(impl_);

  return io_awaiter([&fd, src, count, offset, this](void *data) {
    impl_->enqueue(data, io_uring_prep_write, *fd, src, count, offset);
  });

}

// auto io_context::accept(file_descriptor const &fd, std::byte *src,
//                        std::size_t count, std::size_t offset) noexcept
//     -> io_awaiter {
//   assert(impl_);
//
//   return io_awaiter([&fd, src, count, offset, this](void *data) {
//     impl_->enqueue(data, io_uring_prep_accept, *fd, src, count, offset);
//   });
// }

auto io_context::execute() noexcept -> std::expected<void, std::error_code> {
  assert(impl_);

  return impl_->execute();
}

auto io_context::sq_count() const noexcept -> std::size_t {
  assert(impl_);

  return impl_->sq_count();
}

auto io_context::cq_count() const noexcept -> std::size_t {
  assert(impl_);

  return impl_->cq_count();
}

} // namespace cong