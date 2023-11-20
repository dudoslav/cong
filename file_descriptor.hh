#pragma once

namespace cong {

using file_descriptor = int;

static constexpr file_descriptor invalid_file_descriptor = -1;

class file_descriptor_guard
{
public:
    /// Even though this is public it should never be called... Make a lot of friends and make this private
    explicit file_descriptor_guard(file_descriptor handle);

    /* Copy */
    file_descriptor_guard(const file_descriptor_guard& other) = delete;
    auto operator=(const file_descriptor_guard& other) -> file_descriptor_guard& = delete;

    /* Move */
    file_descriptor_guard(file_descriptor_guard&& other) noexcept;
    auto operator=(file_descriptor_guard&& other) noexcept -> file_descriptor_guard&;

    /// This is my lifelong dillema if to make this const
    auto get_handle() -> int;

    ~file_descriptor_guard() noexcept;

private:
    file_descriptor handle_{invalid_file_descriptor};
};

}