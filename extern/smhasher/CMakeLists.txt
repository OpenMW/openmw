add_library(smhasher STATIC MurmurHash3.cpp)
target_include_directories(smhasher INTERFACE .)
