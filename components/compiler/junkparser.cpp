#include "junkparser.hpp"

#include "scanner.hpp"

Compiler::JunkParser::JunkParser (ErrorHandler& errorHandler, const Context& context,
    int ignoreKeyword)
: Parser (errorHandler, context), mIgnoreKeyword (ignoreKeyword)
{}

bool Compiler::JunkParser::parseInt (int value, const TokenLoc& loc, Scanner& scanner)
{
    scanner.putbackInt (value, loc);
    return false;
}

bool Compiler::JunkParser::parseFloat (float value, const TokenLoc& loc, Scanner& scanner)
{
    scanner.putbackFloat (value, loc);
    return false;
}

bool Compiler::JunkParser::parseName (const std::string& name, const TokenLoc& loc,
    Scanner& scanner)
{
    scanner.putbackName (name, loc);
    return false;
}

bool Compiler::JunkParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
{
    if (keyword==mIgnoreKeyword)
        reportWarning ("Ignoring found junk", loc);
    else
        scanner.putbackKeyword (keyword, loc);

    return false;
}

bool Compiler::JunkParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
{
    if (code==Scanner::S_member)
        reportWarning ("Ignoring found junk", loc);
    else
        scanner.putbackSpecial (code, loc);

    return false;
}
