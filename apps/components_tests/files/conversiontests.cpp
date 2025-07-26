#include <components/files/conversion.hpp>
#include <components/misc/strings/conversion.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace testing;
    using namespace Files;

    constexpr auto test_path_u8 = u8"./tmp/ÒĎƎɠˠΏЌԹעڨ/ऊঋਐઊଊ/ஐఋಋഊ/ฎນ༈ႩᄇḮὯ⁂₁₩ℒ/Ⅷ↝∑/☝✌〥ぐズ㌎丕.갔３갛";
    constexpr auto test_path = "./tmp/ÒĎƎɠˠΏЌԹעڨ/ऊঋਐઊଊ/ஐఋಋഊ/ฎນ༈ႩᄇḮὯ⁂₁₩ℒ/Ⅷ↝∑/☝✌〥ぐズ㌎丕.갔３갛";

    TEST(OpenMWConversion, should_support_unicode_string_to_path)
    {
        auto p = Files::pathFromUnicodeString(test_path);
        EXPECT_EQ(Misc::StringUtils::u8StringToString(p.u8string()), Misc::StringUtils::u8StringToString(test_path_u8));
    }

    TEST(OpenMWConversion, should_support_path_to_unicode_string)
    {
        std::filesystem::path p{ test_path_u8 };
        EXPECT_EQ(Files::pathToUnicodeString(p), test_path);
    }
}
