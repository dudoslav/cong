#include <array>
#include <coroutine>
#include <concepts>
#include <cstring>
#include <expected>
#include <iostream>
#include <utility>
#include <functional>
#include <format>

#include <fcntl.h>

#include <liburing.h>

#include "coro.hh"
#include "unwrap.hh"
#include "resource.hh"

using file_descriptor = cong::resource<int, decltype(close), close>;

/**
 *
 * create socket
 * client = co_await io.enqueue(LISTEN, socket)
 * auto buf = std::array<128>{};
 * read_count = co_await io.enqueue(READ, client, buf)
 *
 * TODO: Create FD
 */

namespace cong {

    namespace helpers
    {

        auto try_call_with_negative_r(auto&& fun, auto&&... args) noexcept
            -> std::expected<int, std::error_code>
        {
            auto const res = fun(std::forward<decltype(args)>(args)...);

            if (res < 0)
            {
                return std::unexpected{std::error_code{-res, std::system_category()}};
            }

            return res;
        }

    }

    class io_t
    {
    public:
        [[nodiscard]]
        static auto make_io_t(std::size_t size, int flags) noexcept
            -> std::expected<io_t, std::error_code>
        {
            auto io = io_t{};

            return helpers::try_call_with_negative_r(io_uring_queue_init, size, &io.ring_, flags)
                .transform([&](auto){ return std::move(io);});
        }

        auto wait() noexcept -> std::expected<void, std::error_code>
        {
            auto* cqe = static_cast<io_uring_cqe*>(nullptr);

            return helpers::try_call_with_negative_r(io_uring_wait_cqe, &ring_, &cqe)
                .and_then([&](auto) -> std::expected<void, std::error_code>
                {
                    --queued_;
                    auto const status = cqe->res;
                    auto const coro = std::coroutine_handle<>::from_address(io_uring_cqe_get_data(cqe));
                    io_uring_cqe_seen(&ring_, cqe);

                    if (status < 0) {
                        return std::unexpected{std::error_code{-status, std::system_category()}};
                    }

                    coro();

                    /// We cannot do this since coro is RAII and this would sometimes double free
                    // if (coro.done()) {
                    //     coro.destroy();
                    // }

                    return {};
                });
        }

        [[nodiscard]]
        auto enqueue_read(int fd, std::byte* dst, std::size_t len, std::size_t offset) noexcept
        {
            struct io_awaiter
            {
                constexpr auto await_ready() const noexcept -> bool {
                    std::cout << "await_ready\n";
                    return false;
                };

                constexpr auto await_suspend(std::coroutine_handle<> cont) const noexcept -> void {
                    std::cout << "await_suspend\n";
                    io_.enqueue_read_impl(fd, dst, len, offset, cont);
                };

                constexpr auto await_resume() const noexcept -> void
                {
                    std::cout << "await_resume\n";
                };

                /// TODO: Make private
                io_t& io_;
                int fd;
                std::byte* dst;
                std::size_t len;
                std::size_t offset;
            };

            return io_awaiter{*this, fd, dst, len, offset};
        }

        ~io_t() noexcept
        {
            if (ring_.ring_fd != -1) {
                io_uring_queue_exit(&ring_);
            }
        }

        io_t(io_t const &) = delete;
        io_t& operator=(io_t const &) = delete;

        io_t(io_t && other) noexcept
            : ring_{std::exchange(other.ring_, io_t{}.ring_)} /// TODO: Does this makes sense?
        {}

        io_t& operator=(io_t && other) noexcept
        {
            ring_ = std::exchange(other.ring_, io_t{}.ring_);
            return *this;
        }

        [[nodiscard]]
        auto queued() const noexcept -> std::size_t { return queued_; }

    private:

        explicit io_t() noexcept = default;

        io_uring ring_{ .ring_fd = -1 };
        std::size_t queued_{};

        [[nodiscard]]
        auto retrieve_submission_entry() noexcept
            -> std::expected<io_uring_sqe*, std::error_code>
        {
            auto* sqe = io_uring_get_sqe(&ring_);

            if (not sqe) {
                return std::unexpected{std::make_error_code(std::errc::io_error)};
            }

            return sqe;
        }

        [[nodiscard]]
        auto submit() noexcept
            -> std::expected<std::size_t, std::error_code>
        {
            ++queued_;
            return helpers::try_call_with_negative_r(io_uring_submit, &ring_);
        }

        [[nodiscard]]
        auto enqueue_read_impl(int fd, std::byte* dst, std::size_t len, std::size_t offset, std::coroutine_handle<> coro) noexcept
            -> std::expected<std::size_t, std::error_code>
        {
            return retrieve_submission_entry().and_then([&](auto* sqe){
                io_uring_prep_read(sqe, fd, dst, len, offset);
                io_uring_sqe_set_data(sqe, coro.address());
                return submit();
            });
        }
    };

}

auto read_stdin_twice(cong::io_t& io) -> cong::coro<void, std::error_code>
{
    auto buf = std::array<char, 64u>{};

    co_await io.enqueue_read(STDIN_FILENO, reinterpret_cast<std::byte*>(buf.data()), buf.size(), 0);

    std::cout << "First read: " << buf.data() << '\n';

    co_await io.enqueue_read(STDIN_FILENO, reinterpret_cast<std::byte*>(buf.data()), buf.size(), 0);

    std::cout << "Second read: " << buf.data() << '\n';
}

/// TODO: Make this expected not coro
auto entry() -> cong::coro<void, std::error_code>
{
    auto io = co_await unwrap(cong::io_t::make_io_t(4, 999));

    /// We need to extend the lifetime of this. Maybe implement spawn on io_t so that it would hold its lifetime
    /// Lets find out what spawned really means
    auto reader = read_stdin_twice(io);

    while (io.queued())
    {
        /// io.wait() also runs all ready coroutines so it should be renamed to run I guess
        co_await unwrap(io.wait());
    }
}

auto main() -> int {
    if (auto coro = entry(); coro.expected()) {
        std::cerr << std::format(
            "Error occured: {}\nAt: {}:{}:{}",
            coro.expected().error().message(),
            coro.source_location().file_name(),
            coro.source_location().line(),
            coro.source_location().function_name());
        return coro.expected().error().value();
    }

    return EXIT_SUCCESS;
}
