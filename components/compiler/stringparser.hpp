#ifndef COMPILER_STRINGPARSER_H_INCLUDED
#define COMPILER_STRINGPARSER_H_INCLUDED

#include <vector>

#include <components/interpreter/types.hpp>

#include "parser.hpp"
#include "tokenloc.hpp"

namespace Compiler
{
    class Literals;

    class StringParser : public Parser
    {
            enum State
            {
                StartState, CommaState
            };

            Literals& mLiterals;
            State mState;
            std::vector<Interpreter::Type_Code> mCode;
            bool mSmashCase;
            TokenLoc mTokenLoc;
            bool mDiscard;

        public:

            StringParser (ErrorHandler& errorHandler, const Context& context, Literals& literals);

            bool parseName (const std::string& name, const TokenLoc& loc,
                Scanner& scanner) override;
            ///< Handle a name token.
            /// \return fetch another token?

            bool parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner) override;
            ///< Handle a keyword token.
            /// \return fetch another token?

            bool parseSpecial (int code, const TokenLoc& loc, Scanner& scanner) override;
            ///< Handle a special character token.
            /// \return fetch another token?

            void append (std::vector<Interpreter::Type_Code>& code);
            ///< Append code for parsed string.

            void smashCase();
            ///< Transform all scanned strings to lower case

            void reset() override;
            ///< Reset parser to clean state (this includes the smashCase function).

            /// Returns TokenLoc object for string. If no string has been parsed, the TokenLoc
            /// object will be default initialised.
            const TokenLoc& getTokenLoc() const;

            /// If parsing a string, do not add it to the literal table and do not create code
            /// for it.
            void discard();
    };
}

#endif
