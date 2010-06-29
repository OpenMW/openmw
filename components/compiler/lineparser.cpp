
#include "lineparser.hpp"

#include "scanner.hpp"
#include "context.hpp"
#include "errorhandler.hpp"
#include "skipparser.hpp"
#include "locals.hpp"
#include "generator.hpp"

namespace Compiler
{
    LineParser::LineParser (ErrorHandler& errorHandler, Context& context, Locals& locals,
        Literals& literals, std::vector<Interpreter::Type_Code>& code)
    : Parser (errorHandler, context), mLocals (locals), mLiterals (literals), mCode (code),
       mState (BeginState)
    {}

    bool LineParser::parseInt (int value, const TokenLoc& loc, Scanner& scanner)
    {
        if (mState==SetLocalToState)
        {
            Generator::assignIntToLocal (mCode, mLiterals, mLocals.getType (mName),
                mLocals.getIndex (mName), value);
        
            mState = EndState;
            mName.clear();
        
            return true;
        }
    
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
            {
                getErrorHandler().error ("local variables can't be declared in this context", loc);
                SkipParser skip (getErrorHandler(), getContext());
                scanner.scan (skip);
                return false;
            }
         
            std::string name2 = toLower (name);
            
            char type = mLocals.getType (name2);
            
            if (type!=' ')
            {
                getErrorHandler().error ("can't re-declare local variable", loc);
                SkipParser skip (getErrorHandler(), getContext());
                scanner.scan (skip);
                return false;
            }
            
            mLocals.declare (mState==ShortState ? 's' : (mState==LongState ? 'l' : 'f'),
                name2);
            
            mState = EndState;
            return true;
        }
        
        if (mState==SetState)
        {
            // local variable?
            std::string name2 = toLower (name);
            char type = mLocals.getType (name2);
            if (type!=' ')
            {
                mName = name2;
                mState = SetLocalVarState;
                return true;
            }
            
            getErrorHandler().error ("unknown variable", loc);
            SkipParser skip (getErrorHandler(), getContext());
            scanner.scan (skip);
            return false;            
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
                case Scanner::K_set: mState = SetState; return true;
            }
        }
        else if (mState==SetLocalVarState && keyword==Scanner::K_to)
        {
            mState = SetLocalToState;
            return true;
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
        mName.clear();
    }
}

