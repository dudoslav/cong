cmake_minimum_required(VERSION 3.27)

option(CONG_BUILD_TESTING "Enable tests" OFF)

project(cong)

add_subdirectory(src)

if(CONG_BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()