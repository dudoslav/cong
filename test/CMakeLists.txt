# TODO: Rework to support Conan and vcpkg
Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

Include(CTest)
Include(Catch)

add_executable(tests test.cc)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain cong)

catch_discover_tests(tests)

file(COPY test.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})