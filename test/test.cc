#include <fcntl.h>

#include <catch2/catch_test_macros.hpp>

#include <io_context.hh>
#include <resource.hh>
#include <utility.hh>

auto read(cong::io_context &io) noexcept -> cong::io_routine {
  /// TODO: Make this non-relative
  auto fd =
      cong::try_call(open, "../../test/test.txt", 0u).transform([](auto fd) {
        return cong::file_descriptor{fd};
      });

  if (not fd) {
    FAIL("Open error is: " << fd.error().message());
  }

  REQUIRE(fd);

  auto buffer = std::array<std::byte, 32>{};
  co_await io.read(*fd, buffer.data(), buffer.size() - 1u, 0u);

  REQUIRE(reinterpret_cast<char *>(buffer.data()) == std::string{"Land AHOY!"});

  auto ofd = cong::try_call(open, "../../test/test_output.txt", O_CREAT)
                 .transform([](auto fd) { return cong::file_descriptor{fd}; });

  if (not ofd) {
    FAIL("Open error is: " << ofd.error().message());
  }

  REQUIRE(ofd);

  co_await io.write(*ofd, buffer.data(), std::strlen(reinterpret_cast<char*>(buffer.data())), 0u);

  co_return;
}

TEST_CASE("Initialization", "[io_context]") {
  auto io = cong::io_context::make_io_context(4u);

  REQUIRE(io);

  /// TODO: cong::spawn(io, read);
  io->spawn(read(*io));

  while (io->cq_count()) {
    io->execute();
  }
}