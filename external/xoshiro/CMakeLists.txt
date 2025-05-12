add_library(xoshiro INTERFACE)
add_library(xoshiro::xoshiro ALIAS xoshiro)

target_include_directories(xoshiro INTERFACE include)
mct_target_cxx_standard(xoshiro INTERFACE)
