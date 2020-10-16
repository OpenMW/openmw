#ifndef COMPILER_DECLARATIONPARSER_H_INCLUDED
#define COMPILER_DECLARATIONPARSER_H_INCLUDED

#include "parser.hpp"

namespace Compiler
{
    class Locals;

    class DeclarationParser : public Parser
    {
            enum State
            {
                State_Begin, State_Name, State_End
            };

            Locals& mLocals;
            State mState;
            char mType;

        public:

            DeclarationParser (ErrorHandler& errorHandler, const Context& context, Locals& locals);

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

    };
}

#endif
