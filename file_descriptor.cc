#include <utility>
#include <unistd.h>

#include "file_descriptor.hh"

namespace cong {

file_descriptor_guard::file_descriptor_guard(file_descriptor const handle) : handle_{handle} {}

file_descriptor_guard::file_descriptor_guard(file_descriptor_guard&& other) noexcept
    : handle_{std::exchange(other.handle_, invalid_file_descriptor)} {}

auto file_descriptor_guard::operator=(file_descriptor_guard&& other) noexcept -> file_descriptor_guard&
{
    handle_ = std::exchange(other.handle_, invalid_file_descriptor);
    return *this;
}

auto file_descriptor_guard::get_handle() -> int
{
    return handle_;
}

file_descriptor_guard::~file_descriptor_guard() noexcept {
    if (handle_ != -1) {
        close(handle_);
    }
}

}
