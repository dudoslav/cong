#pragma once

#include <source_location>

template<typename T>
struct unwrapper;

template<typename T, typename E>
struct unwrapper<std::expected<T, E>>
{
        bool await_ready() const noexcept
        {
            return expected.has_value();
        }

        template<typename U>
        void await_suspend(std::coroutine_handle<cong::promise<U, E>> coro) const
        {
            coro.promise().error() = expected.error();
            coro.promise().source_location = source_location;
        }

        T await_resume()
        {
            if constexpr (not std::is_same_v<T, void>) {
                return std::move(expected.value());
            }
        }

        std::expected<T, E> expected;
        std::source_location source_location;
};

template<typename T>
struct unwrapper<std::optional<T>>
{
        bool await_ready() const noexcept
        {
            return optional.has_value();
        }

        template<typename U, typename E>
        void await_suspend(std::coroutine_handle<cong::promise<U, E>> coro) const
        {
            coro.promise().source_location = source_location;
        }

        T await_resume()
        {
            if constexpr (not std::is_same_v<T, void>) {
                return std::move(optional.value());
            }
        }

        std::optional<T> optional;
        std::source_location source_location;
};

[[nodiscard]]
auto unwrap(auto value, std::source_location source_location = std::source_location::current()) noexcept -> unwrapper<decltype(value)>
{
    return unwrapper<decltype(value)>{std::forward<decltype(value)>(value), source_location};
}
