
#include "parser.hpp"

#include <cctype>
#include <algorithm>

#include "errorhandler.hpp"
#include "exception.hpp"

namespace Compiler
{
    // Report the error and throw an exception.

    void Parser::reportSeriousError (const std::string& message, const TokenLoc& loc)
    {
        mErrorHandler.error (message, loc);
        throw SourceException();
    }

    // Report the error

    void Parser::reportError (const std::string& message, const TokenLoc& loc)
    {
        mErrorHandler.error (message, loc);
    }

    // Report the warning without throwing an exception.

    void Parser::reportWarning (const std::string& message, const TokenLoc& loc)
    {
        mErrorHandler.warning (message, loc);
    }

    // Report an unexpected EOF condition.

    void Parser::reportEOF()
    {
        mErrorHandler.endOfFile();
        throw EOFException();
    }

    // Return error handler

    ErrorHandler& Parser::getErrorHandler()
    {
        return mErrorHandler;
    }

    // Return context

    Context& Parser::getContext()
    {
        return mContext;
    }

    std::string Parser::toLower (const std::string& name)
    {
        std::string lowerCase;
        
        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);
            
        return lowerCase;
    }

    Parser::Parser (ErrorHandler& errorHandler, Context& context)
    : mErrorHandler (errorHandler), mContext (context)
    {}

    // destructor

    Parser::~Parser() {}

    // Handle an int token.
    // \return fetch another token?
    //
    // - Default-implementation: Report an error.

    bool Parser::parseInt (int value, const TokenLoc& loc, Scanner& scanner)
    {
        reportSeriousError ("Unexpected numeric value", loc);
        return false;
    }

    // Handle a float token.
    // \return fetch another token?
    //
    // - Default-implementation: Report an error.

    bool Parser::parseFloat (float value, const TokenLoc& loc, Scanner& scanner)
    {
        reportSeriousError ("Unexpected floating point value", loc);
        return false;
    }

    // Handle a name token.
    // \return fetch another token?
    //
    // - Default-implementation: Report an error.

    bool Parser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        reportSeriousError ("Unexpected name", loc);
        return false;
    }

    // Handle a keyword token.
    // \return fetch another token?
    //
    // - Default-implementation: Report an error.

    bool Parser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        reportSeriousError ("Unexpected keyword", loc);
        return false;
    }

    // Handle a special character token.
    // \return fetch another token?
    //
    // - Default-implementation: Report an error.

    bool Parser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        reportSeriousError ("Unexpected special token", loc);
        return false;
    }

    // Handle an EOF token.
    //
    // - Default-implementation: Report an error.

    void Parser::parseEOF (Scanner& scanner)
    {
        reportEOF();
    }
}

