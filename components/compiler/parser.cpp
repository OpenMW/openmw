#include "parser.hpp"

#include "errorhandler.hpp"
#include "exception.hpp"
#include "scanner.hpp"

#include <components/misc/stringops.hpp>

namespace Compiler
{
    // Report the error and throw an exception.

    void Parser::reportSeriousError (const std::string& message, const TokenLoc& loc)
    {
        mErrorHandler.error (message, loc);
        throw SourceException();
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

    const Context& Parser::getContext() const
    {
        return mContext;
    }

    std::string Parser::toLower (const std::string& name)
    {
        std::string lowerCase = Misc::StringUtils::lowerCase(name);

        return lowerCase;
    }

    Parser::Parser (ErrorHandler& errorHandler, const Context& context)
    : mErrorHandler (errorHandler), mContext (context), mOptional (false), mEmpty (true)
    {}

    // destructor

    Parser::~Parser() {}

    // Handle an int token.
    // \return fetch another token?
    //
    // - Default-implementation: Report an error.

    bool Parser::parseInt (int value, const TokenLoc& loc, Scanner& scanner)
    {
        if (!(mOptional && mEmpty))
            reportSeriousError ("Unexpected numeric value", loc);
        else
            scanner.putbackInt (value, loc);

        return false;
    }

    // Handle a float token.
    // \return fetch another token?
    //
    // - Default-implementation: Report an error.

    bool Parser::parseFloat (float value, const TokenLoc& loc, Scanner& scanner)
    {
        if (!(mOptional && mEmpty))
            reportSeriousError ("Unexpected floating point value", loc);
        else
            scanner.putbackFloat (value, loc);

        return false;
    }

    // Handle a name token.
    // \return fetch another token?
    //
    // - Default-implementation: Report an error.

    bool Parser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        if (!(mOptional && mEmpty))
            reportSeriousError ("Unexpected name", loc);
        else
            scanner.putbackName (name, loc);

        return false;
    }

    // Handle a keyword token.
    // \return fetch another token?
    //
    // - Default-implementation: Report an error.

    bool Parser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        if (!(mOptional && mEmpty))
            reportSeriousError ("Unexpected keyword", loc);
        else
            scanner.putbackKeyword (keyword, loc);

        return false;
    }

    // Handle a special character token.
    // \return fetch another token?
    //
    // - Default-implementation: Report an error.

    bool Parser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (!(mOptional && mEmpty))
            reportSeriousError ("Unexpected special token", loc);
        else
            scanner.putbackSpecial (code, loc);

        return false;
    }

    bool Parser::parseComment (const std::string& comment, const TokenLoc& loc, Scanner& scanner)
    {
        return true;
    }

    // Handle an EOF token.
    //
    // - Default-implementation: Report an error.

    void Parser::parseEOF (Scanner& scanner)
    {
        reportEOF();
    }

    void Parser::reset()
    {
        mOptional = false;
        mEmpty = true;
    }

    void Parser::setOptional (bool optional)
    {
        mOptional = optional;
    }

    void Parser::start()
    {
        mEmpty = false;
    }

    bool Parser::isEmpty() const
    {
        return mEmpty;
    }
}
