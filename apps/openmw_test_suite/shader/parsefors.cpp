#include <components/shader/shadermanager.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{
    using namespace testing;
    using namespace Shader;

    using DefineMap = ShaderManager::DefineMap;

    struct ShaderParseForsTest : Test
    {
        std::string mSource;
        const std::string mName = "shader";
    };

    TEST_F(ShaderParseForsTest, empty_should_succeed)
    {
        ASSERT_TRUE(parseFors(mSource, mName));
        EXPECT_EQ(mSource, "");
    }

    TEST_F(ShaderParseForsTest, should_fail_for_single_escape_symbol)
    {
        mSource = "$";
        ASSERT_FALSE(parseFors(mSource, mName));
        EXPECT_EQ(mSource, "$");
    }

    TEST_F(ShaderParseForsTest, should_fail_on_first_found_escaped_not_foreach)
    {
        mSource = "$foo ";
        ASSERT_FALSE(parseFors(mSource, mName));
        EXPECT_EQ(mSource, "$foo ");
    }

    TEST_F(ShaderParseForsTest, should_fail_on_absent_foreach_variable)
    {
        mSource = "$foreach ";
        ASSERT_FALSE(parseFors(mSource, mName));
        EXPECT_EQ(mSource, "$foreach ");
    }

    TEST_F(ShaderParseForsTest, should_fail_on_unmatched_after_variable)
    {
        mSource = "$foreach foo ";
        ASSERT_FALSE(parseFors(mSource, mName));
        EXPECT_EQ(mSource, "$foreach foo ");
    }

    TEST_F(ShaderParseForsTest, should_fail_on_absent_newline_after_foreach_list)
    {
        mSource = "$foreach foo 1,2,3 ";
        ASSERT_FALSE(parseFors(mSource, mName));
        EXPECT_EQ(mSource, "$foreach foo 1,2,3 ");
    }

    TEST_F(ShaderParseForsTest, should_fail_on_absent_endforeach_after_newline)
    {
        mSource = "$foreach foo 1,2,3\n";
        ASSERT_FALSE(parseFors(mSource, mName));
        EXPECT_EQ(mSource, "$foreach foo 1,2,3\n");
    }

    TEST_F(ShaderParseForsTest, should_replace_complete_foreach_by_line_number)
    {
        mSource = "$foreach foo 1,2,3\n$endforeach";
        ASSERT_TRUE(parseFors(mSource, mName));
        EXPECT_EQ(mSource, "\n#line 3");
    }

    TEST_F(ShaderParseForsTest, should_replace_loop_variable)
    {
        mSource = "$foreach foo 1,2,3\n$foo\n$endforeach";
        ASSERT_TRUE(parseFors(mSource, mName));
        EXPECT_EQ(mSource, "1\n2\n3\n\n#line 4");
    }

    TEST_F(ShaderParseForsTest, should_count_line_number_from_existing)
    {
        mSource = "$foreach foo 1,2,3\n#line 10\n$foo\n$endforeach";
        ASSERT_TRUE(parseFors(mSource, mName));
        EXPECT_EQ(mSource, "#line 10\n1\n#line 10\n2\n#line 10\n3\n\n#line 12");
    }

    TEST_F(ShaderParseForsTest, should_not_support_nested_loops)
    {
        mSource = "$foreach foo 1,2\n$foo\n$foreach bar 1,2\n$bar\n$endforeach\n$endforeach";
        ASSERT_FALSE(parseFors(mSource, mName));
        EXPECT_EQ(mSource, "1\n1\n2\n$foreach bar 1,2\n1\n\n#line 6\n2\n2\n$foreach bar 1,2\n2\n\n#line 6\n\n#line 7");
    }
}
