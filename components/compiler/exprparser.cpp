
#include "exprparser.hpp"

#include <stdexcept>

#include "generator.hpp"

namespace Compiler
{
    ExprParser::ExprParser (ErrorHandler& errorHandler, Context& context, Locals& locals,
        Literals& literals)
    : Parser (errorHandler, context), mLocals (locals), mLiterals (literals)
    {}

    bool ExprParser::parseInt (int value, const TokenLoc& loc, Scanner& scanner)
    {
        Operand operand;
        operand.mType = 'l';
        operand.mInteger = value;
        
        mOperands.push_back (operand);
        
        return false;
    }

    bool ExprParser::parseFloat (float value, const TokenLoc& loc, Scanner& scanner)
    {
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
        return Parser::parseSpecial (code, loc, scanner);
    }
    
    void ExprParser::reset()
    {
        mOperands.clear();
    }
    
    char ExprParser::write (std::vector<Interpreter::Type_Code>& code)
    {
        if (mOperands.empty())
            throw std::logic_error ("empty expression");
            
        Operand operand = mOperands[mOperands.size()-1];
        mOperands.clear();
        
        Generator::pushInt (code, mLiterals, operand.mInteger);
        
        return operand.mType;
    }    
}

