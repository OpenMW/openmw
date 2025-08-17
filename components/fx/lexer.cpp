#include "lexer.hpp"

#include <cctype>
#include <charconv>
#include <format>

namespace Fx
{
    namespace Lexer
    {
        Lexer::Lexer(std::string_view buffer)
            : mHead(buffer.data())
            , mTail(mHead + buffer.length())
            , mAbsolutePos(0)
            , mColumn(0)
            , mLine(0)
            , mBuffer(buffer)
            , mLastToken(Eof{})
        {
        }

        Token Lexer::next()
        {
            if (mLookahead)
            {
                auto token = *mLookahead;
                drop();
                return token;
            }

            mLastToken = scanToken();

            return mLastToken;
        }

        Token Lexer::peek()
        {
            if (!mLookahead)
                mLookahead = scanToken();

            return *mLookahead;
        }

        void Lexer::drop()
        {
            mLookahead = std::nullopt;
        }

        std::optional<std::string_view> Lexer::jump()
        {
            bool multi = false;
            bool single = false;
            auto start = mHead;
            std::size_t level = 1;

            mLastJumpBlock.line = mLine;

            if (head() == '}')
            {
                mLastJumpBlock.content = {};
                return mLastJumpBlock.content;
            }

            for (; mHead != mTail; advance())
            {
                if (head() == '\n')
                {
                    mLine++;
                    mColumn = 0;
                    if (single)
                    {
                        single = false;
                        continue;
                    }
                }
                else if (multi && head() == '*' && peekChar('/'))
                {
                    multi = false;
                    advance();
                    continue;
                }
                else if (multi || single)
                {
                    continue;
                }
                else if (head() == '/' && peekChar('/'))
                {
                    single = true;
                    advance();
                    continue;
                }
                else if (head() == '/' && peekChar('*'))
                {
                    multi = true;
                    advance();
                    continue;
                }

                if (head() == '{')
                    level++;
                else if (head() == '}')
                    level--;

                if (level == 0)
                {
                    mHead--;
                    mLine--;
                    auto sv = std::string_view{ start, static_cast<std::string_view::size_type>(mHead + 1 - start) };
                    mLastJumpBlock.content = sv;
                    return sv;
                }
            }

            mLastJumpBlock = {};
            return std::nullopt;
        }

        Lexer::Block Lexer::getLastJumpBlock() const
        {
            return mLastJumpBlock;
        }

        [[noreturn]] void Lexer::error(std::string_view msg)
        {
            throw LexerException(std::format("Line {} Col {}. {}", mLine + 1, mColumn, msg));
        }

        void Lexer::advance()
        {
            mAbsolutePos++;
            mHead++;
            mColumn++;
        }

        unsigned char Lexer::head()
        {
            return *mHead;
        }

        bool Lexer::peekChar(char c)
        {
            if (mHead == mTail)
                return false;
            return *(mHead + 1) == c;
        }

        Token Lexer::scanToken()
        {
            while (true)
            {
                if (mHead == mTail)
                    return { Eof{} };

                if (head() == '\n')
                {
                    mLine++;
                    mColumn = 0;
                }

                if (!std::isspace(head()))
                    break;

                advance();
            }

            if (head() == '\"')
                return scanStringLiteral();

            if (std::isalpha(head()))
                return scanLiteral();

            if (std::isdigit(head()) || head() == '.' || head() == '-')
                return scanNumber();

            switch (head())
            {
                case '=':
                    advance();
                    return { Equal{} };
                case '{':
                    advance();
                    return { Open_bracket{} };
                case '}':
                    advance();
                    return { Close_bracket{} };
                case '(':
                    advance();
                    return { Open_Parenthesis{} };
                case ')':
                    advance();
                    return { Close_Parenthesis{} };
                case '\"':
                    advance();
                    return { Quote{} };
                case ':':
                    advance();
                    return { Colon{} };
                case ';':
                    advance();
                    return { SemiColon{} };
                case '|':
                    advance();
                    return { VBar{} };
                case ',':
                    advance();
                    return { Comma{} };
                default:
                    error(std::format("unexpected token <{:c}>", head()));
            }
        }

        Token Lexer::scanLiteral()
        {
            auto start = mHead;
            advance();

            while (mHead != mTail && (std::isalnum(head()) || head() == '_'))
                advance();

            std::string_view value{ start, static_cast<std::string_view::size_type>(mHead - start) };

            if (value == "shared")
                return Shared{};
            if (value == "technique")
                return Technique{};
            if (value == "render_target")
                return Render_Target{};
            if (value == "vertex")
                return Vertex{};
            if (value == "fragment")
                return Fragment{};
            if (value == "compute")
                return Compute{};
            if (value == "sampler_1d")
                return Sampler_1D{};
            if (value == "sampler_2d")
                return Sampler_2D{};
            if (value == "sampler_3d")
                return Sampler_3D{};
            if (value == "uniform_bool")
                return Uniform_Bool{};
            if (value == "uniform_float")
                return Uniform_Float{};
            if (value == "uniform_int")
                return Uniform_Int{};
            if (value == "uniform_vec2")
                return Uniform_Vec2{};
            if (value == "uniform_vec3")
                return Uniform_Vec3{};
            if (value == "uniform_vec4")
                return Uniform_Vec4{};
            if (value == "true")
                return True{};
            if (value == "false")
                return False{};
            if (value == "vec2")
                return Vec2{};
            if (value == "vec3")
                return Vec3{};
            if (value == "vec4")
                return Vec4{};

            return Literal{ value };
        }

        Token Lexer::scanStringLiteral()
        {
            advance(); // consume quote
            auto start = mHead;

            bool terminated = false;

            for (; mHead != mTail; advance())
            {
                if (head() == '\"')
                {
                    terminated = true;
                    advance();
                    break;
                }
            }

            if (!terminated)
                error("unterminated string");

            return String{ { start, static_cast<std::string_view::size_type>(mHead - start - 1) } };
        }

        Token Lexer::scanNumber()
        {
            double buffer;
            const auto [endPtr, ec] = std::from_chars(mHead, mTail, buffer);
            if (ec != std::errc())
                error("critical error while parsing number");

            std::string_view literal(mHead, endPtr);
            mHead = endPtr;

            if (literal.find('.') != std::string_view::npos)
                return Float{ static_cast<float>(buffer) };

            return Integer{ static_cast<int>(buffer) };
        }
    }
}
