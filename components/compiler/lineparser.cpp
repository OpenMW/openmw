
#include "lineparser.hpp"

#include "scanner.hpp"

namespace Compiler
{
    LineParser::LineParser (ErrorHandler& errorHandler, Context& context)
    : Parser (errorHandler, context)
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
        return Parser::parseName (name, loc, scanner);
    }

    bool LineParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool LineParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        return Parser::parseSpecial (code, loc, scanner);
    }
    
    void LineParser::reset()
    {
    
    }
}

