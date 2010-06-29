
#include "exprparser.hpp"

#include <stdexcept>

#include "generator.hpp"
#include "scanner.hpp"
#include "errorhandler.hpp"

namespace Compiler
{
    char ExprParser::popUnaryOperator()
    {
        if (mOperators.empty())
            return 0;
    
        char op = mOperators[mOperators.size()-1];
        
        if (op!='m') // unary -
            return 0;
            
        mOperators.resize (mOperators.size()-1);
            
        return op;
    }
    
    char ExprParser::popOperand (std::vector<Interpreter::Type_Code>& code)
    {
        Operand operand = mOperands[mOperands.size()-1];
        mOperands.resize (mOperands.size()-1);
    
        if (operand.mType=='l')
            Generator::pushInt (code, mLiterals, operand.mInteger);
        else if (operand.mType=='f')
            Generator::pushFloat (code, mLiterals, operand.mFloat);
        else
            throw std::logic_error ("unknown expression type");     
            
       return operand.mType;       
    }

    ExprParser::ExprParser (ErrorHandler& errorHandler, Context& context, Locals& locals,
        Literals& literals)
    : Parser (errorHandler, context), mLocals (locals), mLiterals (literals),
      mNextOperand (true)
    {}

    bool ExprParser::parseInt (int value, const TokenLoc& loc, Scanner& scanner)
    {
        if (mNextOperand)
        {       
            Operand operand;
            operand.mType = 'l';
            operand.mInteger = value;
        
            mOperands.push_back (operand);
            
            mNextOperand = false;
            mTokenLoc = loc;
            return true;
        }
        
        return Parser::parseInt (value, loc, scanner);
    }

    bool ExprParser::parseFloat (float value, const TokenLoc& loc, Scanner& scanner)
    {
        if (mNextOperand)
        {    
            Operand operand;
            operand.mType = 'f';
            operand.mFloat = value;
        
            mOperands.push_back (operand);
            
            mNextOperand = false;
            mTokenLoc = loc;
            return true;
        }            
            
        return Parser::parseFloat (value, loc, scanner);
    }

    bool ExprParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        return Parser::parseName (name, loc, scanner);
    }

    bool ExprParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool ExprParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_comma)
        {
            // end marker
            mTokenLoc = loc;
            return false;
        }
        
        if (code==Scanner::S_newline)
        {
            // end marker
            mTokenLoc = loc;
            scanner.putbackNewline (loc);
            return false;
        }
        
        if (code==Scanner::S_minus)
        {
            if (mNextOperand)            
            {
                // unary
                mOperators.push_back ('m');
                mTokenLoc = loc;
                return true;
            }
        }
        
        return Parser::parseSpecial (code, loc, scanner);
    }
    
    void ExprParser::reset()
    {
        mOperands.clear();
        mOperators.clear();
        mNextOperand = true;
    }
    
    char ExprParser::write (std::vector<Interpreter::Type_Code>& code)
    {
        if (mOperands.empty() && mOperators.empty())
            getErrorHandler().error ("missing expression", mTokenLoc);
            
        if (mNextOperand)
            getErrorHandler().error ("syntax error in expression", mTokenLoc);

        char type = ' ';

        while (!mOperands.empty())
        {
            type = popOperand (code);
                
            while (char op = popUnaryOperator())
            {
                if (op=='m')
                {
                    Generator::negate (code, type);
                }
                else
                    throw std::logic_error ("unknown unary operator");
            }
        }
        
        return type;
    }    
}

