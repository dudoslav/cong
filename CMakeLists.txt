cmake_minimum_required(VERSION 3.27)
project(cong)

add_executable(
        cong
        main.cc
)

target_compile_features(
        cong
        PRIVATE
        cxx_std_23
)

target_link_libraries(
        cong
        PRIVATE
        uring # TODO: This is not hermetic
)
