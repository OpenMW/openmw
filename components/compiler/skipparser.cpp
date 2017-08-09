#include "skipparser.hpp"

#include "scanner.hpp"

namespace Compiler
{
    SkipParser::SkipParser (ErrorHandler& errorHandler, const Context& context)
    : Parser (errorHandler, context)
    {}

    bool SkipParser::parseInt (int value, const TokenLoc& loc, Scanner& scanner)
    {
        return true;
    }

    bool SkipParser::parseFloat (float value, const TokenLoc& loc, Scanner& scanner)
    {
        return true;
    }

    bool SkipParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        return true;
    }

    bool SkipParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        return true;
    }

    bool SkipParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline)
            return false;

        return true;
    }
}

