#ifndef COMPILER_STRINGPARSER_H_INCLUDED
#define COMPILER_STRINGPARSER_H_INCLUDED

#include <vector>

#include <components/interpreter/types.hpp>

#include "parser.hpp"

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

        public:

            StringParser (ErrorHandler& errorHandler, const Context& context, Literals& literals);

            virtual bool parseName (const std::string& name, const TokenLoc& loc,
                Scanner& scanner);
            ///< Handle a name token.
            /// \return fetch another token?

            virtual bool parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner);
            ///< Handle a keyword token.
            /// \return fetch another token?

            virtual bool parseSpecial (int code, const TokenLoc& loc, Scanner& scanner);
            ///< Handle a special character token.
            /// \return fetch another token?

            void append (std::vector<Interpreter::Type_Code>& code);
            ///< Append code for parsed string.

            void smashCase();
            ///< Transform all scanned strings to lower case

            void reset();
            ///< Reset parser to clean state (this includes the smashCase function).
    };
}

#endif
