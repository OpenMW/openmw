#include "declarationparser.hpp"

#include <components/misc/stringops.hpp>

#include "scanner.hpp"
#include "errorhandler.hpp"
#include "skipparser.hpp"
#include "locals.hpp"

Compiler::DeclarationParser::DeclarationParser (ErrorHandler& errorHandler, const Context& context,
    Locals& locals)
: Parser (errorHandler, context), mLocals (locals), mState (State_Begin), mType (0)
{}

bool Compiler::DeclarationParser::parseName (const std::string& name, const TokenLoc& loc,
    Scanner& scanner)
{
    if (mState==State_Name)
    {
        std::string name2 = ::Misc::StringUtils::lowerCase (name);

        char type = mLocals.getType (name2);

        if (type!=' ')
        {
            /// \todo add option to make re-declared local variables an error
            getErrorHandler().warning ("ignoring local variable re-declaration",
                loc);

            mState = State_End;
            return true;
        }

        mLocals.declare (mType, name2);

        mState = State_End;
        return true;
    }

    return Parser::parseName (name, loc, scanner);
}

bool Compiler::DeclarationParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
{
    if (mState==State_Begin)
    {
        switch (keyword)
        {
            case Scanner::K_short: mType = 's'; break;
            case Scanner::K_long: mType = 'l'; break;
            case Scanner::K_float: mType = 'f'; break;
            default: mType = 0;
        }

        if (mType)
        {
            mState = State_Name;
            return true;
        }
    }
    else if (mState==State_Name)
    {
        // allow keywords to be used as local variable names. MW script compiler, you suck!
        /// \todo option to disable this atrocity.
        return parseName (loc.mLiteral, loc, scanner);
    }

    return Parser::parseKeyword (keyword, loc, scanner);
}

bool Compiler::DeclarationParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
{
    if (code==Scanner::S_newline && mState==State_End)
        return false;

    return Parser::parseSpecial (code, loc, scanner);
}

void Compiler::DeclarationParser::reset()
{
    mState = State_Begin;
}
