#include <array>
#include <coroutine>
#include <concepts>
#include <cstring>
#include <expected>
#include <iostream>
#include <utility>
#include <functional>

#include <fcntl.h>

#include <liburing.h>

/**
 *
 * create socket
 * client = co_await io.enqueue(LISTEN, socket)
 * auto buf = std::array<128>{};
 * read_count = co_await io.enqueue(READ, client, buf)
 *
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

    struct resumable
    {
        struct promise_type
        {
            constexpr auto initial_suspend() const noexcept -> std::suspend_never { return {}; }
            constexpr auto final_suspend() const noexcept -> std::suspend_never { return {}; }
            constexpr auto return_void() const noexcept -> std::suspend_always { return {}; }
            constexpr auto get_return_object() const noexcept -> resumable { return {}; }
            constexpr auto unhandled_exception() const noexcept -> void {}
        };
    };

    template<std::size_t Size>
    class io_t
    {
    public:
        static constexpr auto size = Size;

        static auto make_io_t(int flags) noexcept
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
                    auto const status = cqe->res;
                    auto delegate = reinterpret_cast<void(*)()>(io_uring_cqe_get_data(cqe));
                    io_uring_cqe_seen(&ring_, cqe);

                    if (status < 0) {
                        return std::unexpected{std::error_code{-status, std::system_category()}};
                    }

                    std::invoke(delegate);

                    return {};
                });
        }

        /// TODO: Create FD
        [[nodiscard]]
        auto enqueue_read(int fd, std::byte* dst, std::size_t len, std::size_t offset, std::invocable auto&& delegate) noexcept
            -> std::expected<std::size_t, std::error_code>
        {
            return retrieve_submission_entry().and_then([&](auto* sqe){
                io_uring_prep_read(sqe, fd, dst, len, offset);
                io_uring_sqe_set_data(sqe, reinterpret_cast<void*>(static_cast<void(*)()>(delegate)));
                return submit();
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
                    static auto cont_ = cont;
                    io_.enqueue_read(fd, dst, len, offset, [](){
                        std::cout << "await_suspend done\n";
                        cont_();
                    });
                };

                constexpr auto await_resume() const noexcept -> void {};

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
        }
    private:

        explicit io_t() noexcept = default;

        io_uring ring_{ .ring_fd = -1 };

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
            return helpers::try_call_with_negative_r(io_uring_submit, &ring_);
        }
    };

}

auto read_stdin_twice(cong::io_t<4>& io) -> cong::resumable
{
    auto buf = std::array<char, 64u>{};

    co_await io.enqueue_read(STDIN_FILENO, reinterpret_cast<std::byte*>(buf.data()), buf.size(), 0);

    std::cout << "First read: " << buf.data() << '\n';

    co_await io.enqueue_read(STDIN_FILENO, reinterpret_cast<std::byte*>(buf.data()), buf.size(), 0);

    std::cout << "Second read: " << buf.data() << '\n';
}

int main() {
    auto io = cong::io_t<4u>::make_io_t(0);
    if (not io) {
        std::cerr << "io_t: " << io.error().message() << '\n';
        return io.error().value();
    }

    read_stdin_twice(*io);

    auto const wait_res = io->wait();
    if (not wait_res) {
        std::cerr << "wait: " << wait_res.error().message() << '\n';
        return wait_res.error().value();
    }

    return 0;
}
