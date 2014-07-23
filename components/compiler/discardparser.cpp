
#include "discardparser.hpp"

#include "scanner.hpp"

namespace Compiler
{
    DiscardParser::DiscardParser (ErrorHandler& errorHandler, const Context& context)
    : Parser (errorHandler, context), mState (StartState)
    {

    }

    bool DiscardParser::parseInt (int value, const TokenLoc& loc, Scanner& scanner)
    {
        if (mState==StartState || mState==CommaState || mState==MinusState)
        {
            start();
            return false;
        }

        return Parser::parseInt (value, loc, scanner);
    }

    bool DiscardParser::parseFloat (float value, const TokenLoc& loc, Scanner& scanner)
    {
        if (mState==StartState || mState==CommaState || mState==MinusState)
        {
            start();
            return false;
        }

        return Parser::parseFloat (value, loc, scanner);
    }

    bool DiscardParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        if (mState==StartState || mState==CommaState)
        {
            start();
            return false;
        }

        return Parser::parseName (name, loc, scanner);
    }

    bool DiscardParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_comma && mState==StartState)
        {
            mState = CommaState;
            return true;
        }

        if (code==Scanner::S_minus && (mState==StartState || mState==CommaState))
        {
            mState = MinusState;
            return true;
        }

        return Parser::parseSpecial (code, loc, scanner);
    }

    void DiscardParser::reset()
    {
        mState = StartState;
        Parser::reset();
    }
}
