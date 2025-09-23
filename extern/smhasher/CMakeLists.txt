add_library(smhasher STATIC MurmurHash3.cpp include/smhasher/MurmurHash3.h)
target_include_directories(smhasher PUBLIC include)
target_include_directories(smhasher PRIVATE include/smhasher)
