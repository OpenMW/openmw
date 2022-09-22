#include <components/shader/shadermanager.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

namespace
{
    using namespace testing;
    using namespace Shader;

    using DefineMap = ShaderManager::DefineMap;

    struct ShaderParseLinksTest : Test
    {
        std::string mSource;
        std::vector<std::string> mLinkTargets;
        ShaderManager::DefineMap mDefines;
        const std::string mName = "my_shader.glsl";

        bool parseLinks() { return parseDirectives(mSource, mLinkTargets, mDefines, {}, mName); }
    };

    TEST_F(ShaderParseLinksTest, empty_should_succeed)
    {
        ASSERT_TRUE(parseLinks());
        EXPECT_EQ(mSource, "");
        EXPECT_TRUE(mLinkTargets.empty());
    }

    TEST_F(ShaderParseLinksTest, should_fail_for_single_escape_symbol)
    {
        mSource = "$";
        ASSERT_FALSE(parseLinks());
        EXPECT_EQ(mSource, "$");
        EXPECT_TRUE(mLinkTargets.empty());
    }

    TEST_F(ShaderParseLinksTest, should_fail_on_first_found_escaped_not_valid_directive)
    {
        mSource = "$foo ";
        ASSERT_FALSE(parseLinks());
        EXPECT_EQ(mSource, "$foo ");
        EXPECT_TRUE(mLinkTargets.empty());
    }

    TEST_F(ShaderParseLinksTest, should_fail_on_absent_link_target)
    {
        mSource = "$link ";
        ASSERT_FALSE(parseLinks());
        EXPECT_EQ(mSource, "$link ");
        EXPECT_TRUE(mLinkTargets.empty());
    }

    TEST_F(ShaderParseLinksTest, should_not_require_newline)
    {
        mSource = "$link \"foo.glsl\"";
        ASSERT_TRUE(parseLinks());
        EXPECT_EQ(mSource, "");
        ASSERT_EQ(mLinkTargets.size(), 1);
        EXPECT_EQ(mLinkTargets[0], "foo.glsl");
    }

    TEST_F(ShaderParseLinksTest, should_require_quotes)
    {
        mSource = "$link foo.glsl";
        ASSERT_FALSE(parseLinks());
        EXPECT_EQ(mSource, "$link foo.glsl");
        EXPECT_EQ(mLinkTargets.size(), 0);
    }

    TEST_F(ShaderParseLinksTest, should_be_replaced_with_empty_line)
    {
        mSource = "$link \"foo.glsl\"\nbar";
        ASSERT_TRUE(parseLinks());
        EXPECT_EQ(mSource, "\nbar");
        ASSERT_EQ(mLinkTargets.size(), 1);
        EXPECT_EQ(mLinkTargets[0], "foo.glsl");
    }

    TEST_F(ShaderParseLinksTest, should_only_accept_on_true_condition)
    {
        mSource =
            R"glsl(
$link "foo.glsl" if 1
$link "bar.glsl" if 0
)glsl";
        ASSERT_TRUE(parseLinks());
        EXPECT_EQ(mSource,
            R"glsl(


)glsl");
        ASSERT_EQ(mLinkTargets.size(), 1);
        EXPECT_EQ(mLinkTargets[0], "foo.glsl");
    }
}
