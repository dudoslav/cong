#include <filesystem>
#include <sstream>

#include <fcntl.h>

#include <catch2/catch_test_macros.hpp>

#include <io_context.hh>
#include <resource.hh>
#include <utility.hh>

namespace Catch {
template <typename T> struct StringMaker<cong::io_result<T>> {
  static std::string convert(cong::io_result<T> const &value) {
    if (value) {
      /// TODO: Somehow make work
      // auto ss = std::ostringstream{};
      // ss << *value;
      // return ss.str();
      return "Contains value";
    } else {
      return value.error().message();
    }
  }
};
} // namespace Catch

auto open_file(std::filesystem::path const &path, int flags = 0u, int permissions = 0u)
    -> cong::io_result<cong::file_descriptor> {
  return cong::try_call(open, path.c_str(), flags, permissions).transform([](auto fd) {
    return cong::file_descriptor{fd};
  });
}

auto read(cong::io_context &io, cong::file_descriptor &fd) noexcept
    -> cong::io_routine<std::string> {
  auto result = std::array<char, 32u>{};

  co_await io.read(fd, reinterpret_cast<std::byte *>(result.data()),
                   result.size() - 1u, 0u);

  co_return result.data();
}

auto write(cong::io_context &io, cong::file_descriptor &fd, std::string_view out) -> cong::io_routine<void> {
  co_await io.write(fd, reinterpret_cast<std::byte const*>(out.data()), out.size(), 0u);
}

auto process(cong::io_context &io) noexcept -> cong::io_routine<void> {
  auto input_file = open_file("test.txt", O_RDONLY);
  REQUIRE(input_file);

  auto const input = co_await read(io, *input_file);

  REQUIRE(input == "Land AHOY!");

  auto output_file = open_file("out.txt", O_CREAT, 777);
  REQUIRE(output_file);

  co_await write(io, *output_file, {input.data(), input.size()});
}

TEST_CASE("Initialization", "[io_context]") {
  auto io = cong::io_context::make_io_context(4u);

  REQUIRE(io);

  /// TODO: cong::spawn(io, read);
  io->spawn(process(*io));

  while (io->cq_count()) {
    io->execute();
  }
}