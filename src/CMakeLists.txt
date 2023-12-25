find_package(PkgConfig)
pkg_check_modules(liburing REQUIRED IMPORTED_TARGET GLOBAL liburing>=2.0)

add_library(cong io_context.cc)

target_compile_features(cong PUBLIC cxx_std_23)

target_link_libraries(cong PRIVATE PkgConfig::liburing)

target_include_directories(cong PUBLIC .)