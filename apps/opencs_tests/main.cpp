#include <components/debug/debugging.hpp>
#include <components/testing/util.hpp>

#include <gtest/gtest.h>

int main(int argc, char* argv[])
{
    Log::sMinDebugLevel = Debug::getDebugLevel();

    testing::InitGoogleTest(&argc, argv);

    const int result = RUN_ALL_TESTS();
    if (result == 0)
        std::filesystem::remove_all(TestingOpenMW::outputDir());
    return result;
}
