
#include "lineparser.hpp"

#include "scanner.hpp"
#include "context.hpp"
#include "errorhandler.hpp"
#include "skipparser.hpp"
#include "locals.hpp"
#include "generator.hpp"
#include "extensions.hpp"

namespace Compiler
{
    LineParser::LineParser (ErrorHandler& errorHandler, Context& context, Locals& locals,
        Literals& literals, std::vector<Interpreter::Type_Code>& code)
    : Parser (errorHandler, context), mLocals (locals), mLiterals (literals), mCode (code),
       mState (BeginState), mExprParser (errorHandler, context, locals, literals)
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
            std::string name2 = toLower (name);

            // local variable?
            char type = mLocals.getType (name2);
            if (type!=' ')
            {
                mName = name2;
                mState = SetLocalVarState;
                return true;
            }
            
            type = getContext().getGlobalType (name2);
            if (type!=' ')
            {
                mName = name2;
                mType = type;
                mState = SetGlobalVarState;
                return true;
            }                        
            
            getErrorHandler().error ("unknown variable", loc);
            SkipParser skip (getErrorHandler(), getContext());
            scanner.scan (skip);
            return false;            
        }
        
        if (mState==MessageState || mState==MessageCommaState)
        {
            std::string arguments;
            
            for (std::size_t i=0; i<name.size(); ++i)
            {
                if (name[i]=='%')
                {
                    ++i;
                    if (i<name.size())
                    {
                        if (name[i]=='G' || name[i]=='g')
                        {
                            arguments += "l";
                        }
                        else if (name[i]=='S' || name[i]=='s')
                        {
                            arguments += 'S';
                        }
                        else if (name[i]=='.' || name[i]=='f')
                        {
                            arguments += 'f';
                        }
                    }
                }
            }

            if (!arguments.empty())
            {
                mExprParser.reset();
                mExprParser.parseArguments (arguments, scanner, mCode, true);
            }
            
            // for now skip buttons
           SkipParser skip (getErrorHandler(), getContext());
           scanner.scan (skip);            

            Generator::message (mCode, mLiterals, name, 0);
            mState = EndState;
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
                case Scanner::K_messagebox: mState = MessageState; return true;
                
                case Scanner::K_return:
                
                    Generator::exit (mCode);
                    mState = EndState;
                    return true;
                    
                case Scanner::K_startscript:

                    mExprParser.parseArguments ("c", scanner, mCode, true);                   
                    Generator::startScript (mCode);
                    mState = EndState;
                    return true;                
                    
                case Scanner::K_stopscript:

                    mExprParser.parseArguments ("c", scanner, mCode, true);                   
                    Generator::stopScript (mCode);
                    mState = EndState;
                    return true;     
                    
                case Scanner::K_enable:
                
                    Generator::enable (mCode);
                    mState = EndState;
                    return true;                           
                    
                case Scanner::K_disable:
                
                    Generator::disable (mCode);
                    mState = EndState;
                    return true;                           
            }
            
            // check for custom extensions
            if (const Extensions *extensions = getContext().getExtensions())
            {
                std::string argumentType;
                
                if (extensions->isInstruction (keyword, argumentType))
                {
                    mExprParser.parseArguments (argumentType, scanner, mCode, true);
                    
                    extensions->generateInstructionCode (keyword, mCode);
                    mState = EndState;
                    return true;
                }
            }
        }
        else if (mState==SetLocalVarState && keyword==Scanner::K_to)
        {
            mExprParser.reset();
            scanner.scan (mExprParser);
            
            std::vector<Interpreter::Type_Code> code;
            char type = mExprParser.append (code);
            
            Generator::assignToLocal (mCode, mLocals.getType (mName),
                mLocals.getIndex (mName), code, type);
                        
            mState = EndState;
            return true;
        }
        else if (mState==SetGlobalVarState && keyword==Scanner::K_to)
        {
            mExprParser.reset();
            scanner.scan (mExprParser);
            
            std::vector<Interpreter::Type_Code> code;
            char type = mExprParser.append (code);
            
            Generator::assignToGlobal (mCode, mLiterals, mType, mName, code, type);
                        
            mState = EndState;
            return true;
        }
                
        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool LineParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline && mState==EndState)
            return false;
            
        if (code==Scanner::S_comma && mState==MessageState)
        {
            mState = MessageCommaState;
            return true;
        }
            
        return Parser::parseSpecial (code, loc, scanner);
    }
    
    void LineParser::reset()
    {
        mState = BeginState;
        mName.clear();
    }
}

