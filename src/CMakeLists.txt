add_library(cong io_context.cc)

target_compile_features(cong PUBLIC cxx_std_23)

target_link_libraries(cong PRIVATE uring)

target_include_directories(cong PUBLIC .)