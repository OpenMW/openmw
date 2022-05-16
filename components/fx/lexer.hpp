#ifndef OPENMW_COMPONENTS_FX_LEXER_H
#define OPENMW_COMPONENTS_FX_LEXER_H

#include <string_view>
#include <string>
#include <variant>
#include <optional>
#include <cstdint>
#include <stdexcept>

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include "lexer_types.hpp"

namespace fx
{
    namespace Lexer
    {
        struct LexerException : std::runtime_error
        {
            LexerException(const std::string& message) : std::runtime_error(message) {}
            LexerException(const char* message) : std::runtime_error(message) {}
        };

        class Lexer
        {
        public:
            struct Block
            {
                int line;
                std::string_view content;
            };

            Lexer(std::string_view buffer);
            Lexer() = delete;

            Token next();
            Token peek();

            // Jump ahead to next uncommented closing bracket at level zero. Assumes the head is at an opening bracket.
            // Returns the contents of the block excluding the brackets and places cursor at closing bracket.
            std::optional<std::string_view> jump();

            Block getLastJumpBlock() const;

            [[noreturn]] void error(const std::string& msg);

        private:
            void drop();
            void advance();
            char head();
            bool peekChar(char c);

            Token scanToken();
            Token scanLiteral();
            Token scanStringLiteral();
            Token scanNumber();

            const char* mHead;
            const char* mTail;
            std::size_t mAbsolutePos;
            std::size_t mColumn;
            std::size_t mLine;
            std::string_view mBuffer;
            Token mLastToken;
            std::optional<Token> mLookahead;

            Block mLastJumpBlock;
        };
    }
}

#endif
