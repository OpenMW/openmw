
#include "errorhandler.hpp"

namespace Compiler
{
    // constructor

    ErrorHandler::ErrorHandler() : mWarnings (0), mErrors (0), mWarningsMode (1) {}

    // destructor

    ErrorHandler::~ErrorHandler() {}

    // Was compiling successful?

    bool ErrorHandler::isGood() const
    {
        return mErrors==0;
    }

    // Return number of errors

    int ErrorHandler::countErrors() const
    {
        return mErrors;
    }

    // Return number of warnings

    int ErrorHandler::countWarnings() const
    {
        return mWarnings;
    }

    // Generate a warning message.

    void ErrorHandler::warning (const std::string& message, const TokenLoc& loc)
    {
        if (mWarningsMode==1)
        {
            ++mWarnings;
            report (message, loc, WarningMessage);
        }
        else if (mWarningsMode==2)
            error (message, loc);
    }

    // Generate an error message.

    void ErrorHandler::error (const std::string& message, const TokenLoc& loc)
    {
        ++mErrors;
        report (message, loc, ErrorMessage);
    }

    // Generate an error message for an unexpected EOF.

    void ErrorHandler::endOfFile()
    {
        ++mErrors;
        report ("unexpected end of file", ErrorMessage);
    }

    // Remove all previous error/warning events

    void ErrorHandler::reset()
    {
        mErrors = mWarnings = 0;
    }

    void ErrorHandler::setWarningsMode (int mode)
    {
        mWarningsMode = mode;
    }
}
