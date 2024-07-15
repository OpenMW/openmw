#include <components/debug/debugging.hpp>

#include <gtest/gtest.h>

int main(int argc, char* argv[])
{
    Log::sMinDebugLevel = Debug::getDebugLevel();

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
