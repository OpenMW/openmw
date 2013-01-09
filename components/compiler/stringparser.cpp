
#include "stringparser.hpp"

#include <algorithm>
#include <iterator>

#include "scanner.hpp"
#include "generator.hpp"
#include <components/misc/stringops.hpp>

namespace Compiler
{
    StringParser::StringParser (ErrorHandler& errorHandler, Context& context, Literals& literals)
    : Parser (errorHandler, context), mLiterals (literals), mState (StartState), mSmashCase (false)
    {

    }

    bool StringParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        if (mState==StartState || mState==CommaState)
        {
            start();
            if (mSmashCase)
                Generator::pushString (mCode, mLiterals, Misc::StringUtils::lowerCase (name));
            else
                Generator::pushString (mCode, mLiterals, name);

            return false;
        }

        return Parser::parseName (name, loc, scanner);
    }

    bool StringParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_comma && mState==StartState)
        {
            mState = CommaState;
            return true;
        }

        return Parser::parseSpecial (code, loc, scanner);
    }

    void StringParser::append (std::vector<Interpreter::Type_Code>& code)
    {
        std::copy (mCode.begin(), mCode.end(), std::back_inserter (code));
    }

    void StringParser::reset()
    {
        mState = StartState;
        mCode.clear();
        mSmashCase = false;
        Parser::reset();
    }

    void StringParser::smashCase()
    {
        mSmashCase = true;
    }
}
