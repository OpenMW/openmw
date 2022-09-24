#include <components/fx/lexer.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace testing;
    using namespace fx::Lexer;

    struct LexerTest : Test
    {
    };

    struct LexerSingleTokenTest : Test
    {
        template <class Token>
        void test()
        {
            const std::string content = std::string(Token::repr);
            Lexer lexer(content);

            EXPECT_TRUE(std::holds_alternative<Token>(lexer.next()));
        }
    };

    TEST_F(LexerSingleTokenTest, single_token_shared)
    {
        test<Shared>();
    }
    TEST_F(LexerSingleTokenTest, single_token_technique)
    {
        test<Technique>();
    }
    TEST_F(LexerSingleTokenTest, single_token_render_target)
    {
        test<Render_Target>();
    }
    TEST_F(LexerSingleTokenTest, single_token_vertex)
    {
        test<Vertex>();
    }
    TEST_F(LexerSingleTokenTest, single_token_fragment)
    {
        test<Fragment>();
    }
    TEST_F(LexerSingleTokenTest, single_token_compute)
    {
        test<Compute>();
    }
    TEST_F(LexerSingleTokenTest, single_token_sampler_1d)
    {
        test<Sampler_1D>();
    }
    TEST_F(LexerSingleTokenTest, single_token_sampler_2d)
    {
        test<Sampler_2D>();
    }
    TEST_F(LexerSingleTokenTest, single_token_sampler_3d)
    {
        test<Sampler_3D>();
    }
    TEST_F(LexerSingleTokenTest, single_token_true)
    {
        test<True>();
    }
    TEST_F(LexerSingleTokenTest, single_token_false)
    {
        test<False>();
    }
    TEST_F(LexerSingleTokenTest, single_token_vec2)
    {
        test<Vec2>();
    }
    TEST_F(LexerSingleTokenTest, single_token_vec3)
    {
        test<Vec3>();
    }
    TEST_F(LexerSingleTokenTest, single_token_vec4)
    {
        test<Vec4>();
    }

    TEST(LexerTest, peek_whitespace_only_content_should_be_eof)
    {
        Lexer lexer(R"(

        )");

        EXPECT_TRUE(std::holds_alternative<Eof>(lexer.peek()));
    }

    TEST(LexerTest, float_with_no_prefixed_digits)
    {
        Lexer lexer(R"(
            0.123;
        )");

        auto token = lexer.next();
        EXPECT_TRUE(std::holds_alternative<Float>(token));
        EXPECT_FLOAT_EQ(std::get<Float>(token).value, 0.123f);
    }

    TEST(LexerTest, float_with_alpha_prefix)
    {
        Lexer lexer(R"(
            abc.123;
        )");

        EXPECT_TRUE(std::holds_alternative<Literal>(lexer.next()));

        auto token = lexer.next();
        EXPECT_TRUE(std::holds_alternative<Float>(token));
        EXPECT_FLOAT_EQ(std::get<Float>(token).value, 0.123f);
    }

    TEST(LexerTest, float_with_numeric_prefix)
    {
        Lexer lexer(R"(
            123.123;
        )");

        auto token = lexer.next();
        EXPECT_TRUE(std::holds_alternative<Float>(token));
        EXPECT_FLOAT_EQ(std::get<Float>(token).value, 123.123f);
    }

    TEST(LexerTest, int_should_not_be_float)
    {
        Lexer lexer(R"(
            123
        )");

        auto token = lexer.next();
        EXPECT_TRUE(std::holds_alternative<Integer>(token));
        EXPECT_EQ(std::get<Integer>(token).value, 123);
    }

    TEST(LexerTest, simple_string)
    {
        Lexer lexer(R"(
            "test string"
        )");

        auto token = lexer.next();
        EXPECT_TRUE(std::holds_alternative<String>(token));

        std::string parsed = std::string(std::get<String>(token).value);
        EXPECT_EQ("test string", parsed);
    }

    TEST(LexerTest, fail_on_unterminated_double_quotes)
    {
        Lexer lexer(R"(
            "unterminated string'
        )");

        EXPECT_THROW(lexer.next(), LexerException);
    }

    TEST(LexerTest, multiline_strings_with_single_quotes)
    {
        Lexer lexer(R"(
            "string that is
                on multiple with 'single quotes'
            and correctly terminated!"
        )");

        auto token = lexer.next();
        EXPECT_TRUE(std::holds_alternative<String>(token));
    }

    TEST(LexerTest, fail_on_unterminated_double_quotes_with_multiline_strings)
    {
        Lexer lexer(R"(
            "string that is
                on multiple with 'single quotes'
            and but is unterminated :(
        )");

        EXPECT_THROW(lexer.next(), LexerException);
    }

    TEST(LexerTest, jump_with_single_nested_bracket)
    {
        const std::string content = R"(
                #version 120

                void main()
                {
                    return 0;
                }})";

        const std::string expected = content.substr(0, content.size() - 1);

        Lexer lexer(content);

        auto block = lexer.jump();

        EXPECT_NE(block, std::nullopt);
        EXPECT_EQ(expected, std::string(block.value()));
    }

    TEST(LexerTest, jump_with_single_line_comments_and_mismatching_brackets)
    {
        const std::string content = R"(
                #version 120

                void main()
                {
                    // }
                    return 0;
                }})";

        const std::string expected = content.substr(0, content.size() - 1);

        Lexer lexer(content);

        auto block = lexer.jump();

        EXPECT_NE(block, std::nullopt);
        EXPECT_EQ(expected, std::string(block.value()));
    }

    TEST(LexerTest, jump_with_multi_line_comments_and_mismatching_brackets)
    {
        const std::string content = R"(
                #version 120

                void main()
                {
                    /*
                        }
                    */
                    return 0;
                }})";

        const std::string expected = content.substr(0, content.size() - 1);

        Lexer lexer(content);

        auto block = lexer.jump();

        EXPECT_NE(block, std::nullopt);
        EXPECT_EQ(expected, std::string(block.value()));
    }

    TEST(LexerTest, immediate_closed_blocks)
    {
        Lexer lexer(R"(block{})");

        EXPECT_TRUE(std::holds_alternative<Literal>(lexer.next()));
        EXPECT_TRUE(std::holds_alternative<Open_bracket>(lexer.next()));
        auto block = lexer.jump();
        EXPECT_TRUE(block.has_value());
        EXPECT_TRUE(block.value().empty());
        EXPECT_TRUE(std::holds_alternative<Close_bracket>(lexer.next()));
    }

}
