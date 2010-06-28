
#include "scriptparser.hpp"

#include "scanner.hpp"

namespace Compiler
{
    ScriptParser::ScriptParser (ErrorHandler& errorHandler, Context& context, bool end)
    : Parser (errorHandler, context), mLineParser (errorHandler, context), mEnd (end)
    {}

    bool ScriptParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        mLineParser.reset();
        if (mLineParser.parseName (name, loc, scanner))
            scanner.scan (mLineParser);
        
        return true;
    }

    bool ScriptParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        if (keyword==Scanner::K_end && mEnd)
        {
            return false;
        }
    
        mLineParser.reset();
        if (mLineParser.parseKeyword (keyword, loc, scanner))
            scanner.scan (mLineParser);
            
        return true;
    }

    bool ScriptParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline) // empty line
            return true;
            
        mLineParser.reset();
        if (mLineParser.parseSpecial (code, loc, scanner))
            scanner.scan (mLineParser);
            
        return true;
    }

    void ScriptParser::parseEOF (Scanner& scanner)
    {
        if (mEnd)
            Parser::parseEOF (scanner);
    }

    void ScriptParser::reset()
    {
        mLineParser.reset();
    }
}

