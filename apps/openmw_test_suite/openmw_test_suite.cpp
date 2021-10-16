#include "esmloader/settings.hpp"

#include <gtest/gtest.h>

#include <boost/filesystem/path.hpp>

#ifdef WIN32
//we cannot use GTEST_API_ before main if we're building standalone exe application,
//and we're linking GoogleTest / GoogleMock as DLLs and not linking gtest_main / gmock_main
int main(int argc, char **argv) {
#else
GTEST_API_ int main(int argc, char **argv) {
#endif
    EsmLoaderTests::Settings::impl().mBasePath = boost::filesystem::path(argv[0]).parent_path();
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
