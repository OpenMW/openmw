#ifndef COMPILER_CONTROLPARSER_H_INCLUDED
#define COMPILER_CONTROLPARSER_H_INCLUDED

#include <vector>

#include <components/interpreter/types.hpp>

#include "parser.hpp"
#include "exprparser.hpp"
#include "lineparser.hpp"

namespace Compiler
{
    class Locals;
    class Literals;

    // Control structure parser

    class ControlParser : public Parser
    {
            enum State
            {
                StartState,
                IfEndState, IfBodyState,
                IfElseifEndState, IfElseifBodyState,
                IfElseEndState, IfElseBodyState,
                IfEndifState,
                WhileEndState, WhileBodyState,
                WhileEndwhileState,
                IfElseJunkState
            };

            typedef std::vector<Interpreter::Type_Code> Codes;
            typedef std::vector<std::pair<Codes, Codes> > IfCodes;

            Locals& mLocals;
            Literals& mLiterals;
            Codes mCode;
            Codes mCodeBlock;
            IfCodes mIfCode; // condition, body
            LineParser mLineParser;
            ExprParser mExprParser;
            State mState;

            bool parseIfBody (int keyword, const TokenLoc& loc, Scanner& scanner);

            bool parseWhileBody (int keyword, const TokenLoc& loc, Scanner& scanner);

        public:

            ControlParser (ErrorHandler& errorHandler, const Context& context, Locals& locals,
                Literals& literals);

            void appendCode (std::vector<Interpreter::Type_Code>& code) const;
            ///< store generated code in \a code.

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

            void reset() override;
            ///< Reset parser to clean state.
    };
}

#endif
