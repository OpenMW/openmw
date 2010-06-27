#include "fileparser.hpp"

#include <iostream>

#include "tokenloc.hpp"


#include "scanner.hpp"

namespace Compiler
{
    FileParser::FileParser (ErrorHandler& errorHandler, Context& context)
    : Parser (errorHandler, context)
    {}

    bool FileParser::parseInt (int value, const TokenLoc& loc, Scanner& scanner)
    {
        std::cout << "integer: " << value << std::endl;
        return true;
    }

    bool FileParser::parseFloat (float value, const TokenLoc& loc, Scanner& scanner)
    {
        std::cout << "float: " << value << std::endl;
        return true;
    }

    bool FileParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        std::cout << "name: " << name << std::endl;
        return true;
    }

    bool FileParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        std::cout << "keyword: " << loc.mLiteral << std::endl;
        return true;
    }

    bool FileParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline)
            std::cout << "newline" << std::endl;
        else
            std::cout << "special: " << loc.mLiteral << std::endl;
            
        return true;
    }

    void FileParser::parseEOF (Scanner& scanner)
    {
        std::cout << "end of file" << std::endl;
    }
}

