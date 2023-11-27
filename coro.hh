#pragma once

#include <coroutine>
#include <source_location>

namespace cong {

    template<typename T, typename E>
    class promise;

    template<typename T, typename E>
    class coro
    {
    public:
        using promise_type = promise<T, E>;
        using handle = std::coroutine_handle<promise_type>;

        ~coro() noexcept
        {
            if (handle_)
            {
                handle_.destroy();
            }
        }

        [[nodiscard]]
        auto expected() noexcept -> std::expected<T, E>&
        {
            return handle_.promise();
        }

        [[nodiscard]]
        auto source_location() noexcept -> std::source_location
        {
            return handle_.promise().source_location;
        }

    private:
        friend class promise<T, E>;

        handle handle_;

        explicit coro(handle h) noexcept : handle_{h} {}
    };

    template<typename T, typename E>
    class promise : public std::expected<T, E>
    {
    public:
        constexpr auto initial_suspend() const noexcept -> std::suspend_never { return {}; }

        constexpr auto final_suspend() const noexcept -> std::suspend_never { return {}; }

        constexpr auto return_value(auto&& value) noexcept -> void
        {
            this->value() = std::forward<decltype(value)>(value);
        }

        constexpr auto get_return_object() noexcept -> coro<T, E>
        {
            return coro<T, E>{ coro<T, E>::handle::from_promise(*this) };
        }

        constexpr auto unhandled_exception() noexcept -> void
        {
            /// Well, we should do something with the exception :/
            this->error() = E{};
        }

        std::source_location source_location;
    };

}

