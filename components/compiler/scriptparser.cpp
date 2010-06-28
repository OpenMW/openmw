
#include "scriptparser.hpp"

#include "scanner.hpp"

namespace Compiler
{
    ScriptParser::ScriptParser (ErrorHandler& errorHandler, Context& context, bool end)
    : Parser (errorHandler, context), mEnd (end)
    {}

    bool ScriptParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        return Parser::parseName (name, loc, scanner);
    }

    bool ScriptParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        if (keyword==Scanner::K_end && mEnd)
        {
            return false;
        }
    
        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool ScriptParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline) // empty line
            return true;
            
        return Parser::parseSpecial (code, loc, scanner);
    }

    void ScriptParser::parseEOF (Scanner& scanner)
    {
        if (mEnd)
            Parser::parseEOF (scanner);
    }

    void ScriptParser::reset()
    {
    
    }
}

