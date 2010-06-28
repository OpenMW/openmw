
#include "lineparser.hpp"

#include <iostream>

#include "scanner.hpp"
#include "context.hpp"
#include "errorhandler.hpp"

namespace Compiler
{
    LineParser::LineParser (ErrorHandler& errorHandler, Context& context)
    : Parser (errorHandler, context), mState (BeginState)
    {}

    bool LineParser::parseInt (int value, const TokenLoc& loc, Scanner& scanner)
    {
        return Parser::parseInt (value, loc, scanner);
    }

    bool LineParser::parseFloat (float value, const TokenLoc& loc, Scanner& scanner)
    {
        return Parser::parseFloat (value, loc, scanner);
    }

    bool LineParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        if (mState==ShortState || mState==LongState || mState==FloatState)
        {
            if (!getContext().canDeclareLocals())
                getErrorHandler().error ("local variables can't be declared in this context", loc);
            
            std::cout << "declaring local variable: " << name << std::endl;
            mState = EndState;
            return true;
        }
        
        return Parser::parseName (name, loc, scanner);
    }

    bool LineParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        if (mState==BeginState)
        {
            switch (keyword)
            {
                case Scanner::K_short: mState = ShortState; return true;
                case Scanner::K_long: mState = LongState; return true;
                case Scanner::K_float: mState = FloatState; return true;
            }
        }
        
        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool LineParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline && mState==EndState)
            return false;
            
        return Parser::parseSpecial (code, loc, scanner);
    }
    
    void LineParser::reset()
    {
        mState = BeginState;
    }
}

