#include <components/shader/shadermanager.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{
    using namespace testing;
    using namespace Shader;

    using DefineMap = ShaderManager::DefineMap;

    struct ShaderParseDefinesTest : Test
    {
        std::string mSource;
        const std::string mName = "shader";
        DefineMap mDefines;
        DefineMap mGlobalDefines;
    };

    TEST_F(ShaderParseDefinesTest, empty_should_succeed)
    {
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "");
    }

    TEST_F(ShaderParseDefinesTest, should_fail_for_absent_define)
    {
        mSource = "@foo\n";
        ASSERT_FALSE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "@foo\n");
    }

    TEST_F(ShaderParseDefinesTest, should_replace_by_existing_define)
    {
        mDefines["foo"] = "42";
        mSource = "@foo\n";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "42\n");
    }

    TEST_F(ShaderParseDefinesTest, should_replace_by_existing_global_define)
    {
        mGlobalDefines["foo"] = "42";
        mSource = "@foo\n";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "42\n");
    }

    TEST_F(ShaderParseDefinesTest, should_prefer_define_over_global_define)
    {
        mDefines["foo"] = "13";
        mGlobalDefines["foo"] = "42";
        mSource = "@foo\n";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "13\n");
    }

    namespace SupportedTerminals
    {
        struct ShaderParseDefinesTest : ::ShaderParseDefinesTest, WithParamInterface<char> {};

        TEST_P(ShaderParseDefinesTest, support_defines_terminated_by)
        {
            mDefines["foo"] = "13";
            mSource = "@foo" + std::string(1, GetParam());
            ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
            EXPECT_EQ(mSource, "13" + std::string(1, GetParam()));
        }

        INSTANTIATE_TEST_SUITE_P(
            SupportedTerminals,
            ShaderParseDefinesTest,
            Values(' ', '\n', '\r', '(', ')', '[', ']', '.', ';', ',')
        );
    }

    TEST_F(ShaderParseDefinesTest, should_not_support_define_ending_with_source)
    {
        mDefines["foo"] = "42";
        mSource = "@foo";
        ASSERT_FALSE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "@foo");
    }

    TEST_F(ShaderParseDefinesTest, should_replace_all_matched_values)
    {
        mDefines["foo"] = "42";
        mSource = "@foo @foo ";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "42 42 ");
    }

    TEST_F(ShaderParseDefinesTest, should_support_define_with_empty_name)
    {
        mDefines[""] = "42";
        mSource = "@ ";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "42 ");
    }

    TEST_F(ShaderParseDefinesTest, should_replace_all_found_defines)
    {
        mDefines["foo"] = "42";
        mDefines["bar"] = "13";
        mDefines["baz"] = "55";
        mSource = "@foo @bar ";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "42 13 ");
    }

    TEST_F(ShaderParseDefinesTest, should_fail_on_foreach_without_endforeach)
    {
        mSource = "@foreach ";
        ASSERT_FALSE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "$foreach ");
    }

    TEST_F(ShaderParseDefinesTest, should_fail_on_endforeach_without_foreach)
    {
        mSource = "@endforeach ";
        ASSERT_FALSE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "$endforeach ");
    }

    TEST_F(ShaderParseDefinesTest, should_replace_at_sign_by_dollar_for_foreach_endforeach)
    {
        mSource = "@foreach @endforeach ";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "$foreach $endforeach ");
    }

    TEST_F(ShaderParseDefinesTest, should_succeed_on_unmatched_nested_foreach)
    {
        mSource = "@foreach @foreach @endforeach ";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "$foreach $foreach $endforeach ");
    }

    TEST_F(ShaderParseDefinesTest, should_fail_on_unmatched_nested_endforeach)
    {
        mSource = "@foreach @endforeach @endforeach ";
        ASSERT_FALSE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "$foreach $endforeach $endforeach ");
    }

    TEST_F(ShaderParseDefinesTest, should_support_nested_foreach)
    {
        mSource = "@foreach @foreach @endforeach @endforeach ";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "$foreach $foreach $endforeach $endforeach ");
    }

    TEST_F(ShaderParseDefinesTest, should_support_foreach_variable)
    {
        mSource = "@foreach foo @foo @endforeach ";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "$foreach foo $foo $endforeach ");
    }

    TEST_F(ShaderParseDefinesTest, should_not_replace_foreach_variable_by_define)
    {
        mDefines["foo"] = "42";
        mSource = "@foreach foo @foo @endforeach ";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "$foreach foo $foo $endforeach ");
    }

    TEST_F(ShaderParseDefinesTest, should_support_nested_foreach_with_variable)
    {
        mSource = "@foreach foo @foo @foreach bar @bar @endforeach @endforeach ";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "$foreach foo $foo $foreach bar $bar $endforeach $endforeach ");
    }

    TEST_F(ShaderParseDefinesTest, should_not_support_single_line_comments_for_defines)
    {
        mDefines["foo"] = "42";
        mSource = "@foo // @foo\n";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "42 // 42\n");
    }

    TEST_F(ShaderParseDefinesTest, should_not_support_multiline_comments_for_defines)
    {
        mDefines["foo"] = "42";
        mSource = "/* @foo */ @foo ";
        ASSERT_TRUE(parseDefines(mSource, mDefines, mGlobalDefines, mName));
        EXPECT_EQ(mSource, "/* 42 */ 42 ");
    }
}
