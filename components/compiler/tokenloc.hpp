#ifndef COMPILER_TOKENLOC_H_INCLUDED
#define COMPILER_TOKENLOC_H_INCLUDED

#include <string>

namespace Compiler
{
    /// \brief Location of a token in a source file

    struct TokenLoc
    {
        int mColumn;
        int mLine;
        std::string mLiteral;

        TokenLoc() : mColumn (0), mLine (0), mLiteral () {}
    };
}

#endif // TOKENLOC_H_INCLUDED
