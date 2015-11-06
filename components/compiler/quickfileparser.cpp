#include "quickfileparser.hpp"

#include "skipparser.hpp"
#include "scanner.hpp"

Compiler::QuickFileParser::QuickFileParser (ErrorHandler& errorHandler, const Context& context,
    Locals& locals)
: Parser (errorHandler, context), mDeclarationParser (errorHandler, context, locals)
{}

bool Compiler::QuickFileParser::parseName (const std::string& name, const TokenLoc& loc,
    Scanner& scanner)
{
    SkipParser skip (getErrorHandler(), getContext());
    scanner.scan (skip);
    return true;
}

bool Compiler::QuickFileParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
{
    if (keyword==Scanner::K_end)
        return false;

    if (keyword==Scanner::K_short || keyword==Scanner::K_long || keyword==Scanner::K_float)
    {
        mDeclarationParser.reset();
        scanner.putbackKeyword (keyword, loc);
        scanner.scan (mDeclarationParser);
        return true;
    }

    SkipParser skip (getErrorHandler(), getContext());
    scanner.scan (skip);
    return true;
}

bool Compiler::QuickFileParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
{
    if (code!=Scanner::S_newline)
    {
        SkipParser skip (getErrorHandler(), getContext());
        scanner.scan (skip);
    }

    return true;
}

void Compiler::QuickFileParser::parseEOF (Scanner& scanner)
{

}
