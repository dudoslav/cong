#pragma once

#include <coroutine>

namespace cong {

    class resumable
    {
    public:
        class promise_type
        {
        public:
            constexpr auto initial_suspend() const noexcept -> std::suspend_never { return {}; }

            constexpr auto final_suspend() const noexcept -> std::suspend_never { return {}; }

            constexpr auto return_void() const noexcept -> std::suspend_always { return {}; }

            constexpr auto get_return_object() const noexcept -> resumable { return {}; }

            constexpr auto unhandled_exception() const noexcept -> void {}
        };

    private:
        // std::coroutine_handle<promise_type> handle_;
    };

}

