#include <gtest/gtest.h>

#ifdef WIN32
//we cannot use GTEST_API_ before main if we're building standalone exe application,
//and we're linking GoogleTest / GoogleMock as DLLs and not linking gtest_main / gmock_main
int main(int argc, char **argv) {
#else
GTEST_API_ int main(int argc, char **argv) {
#endif
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
