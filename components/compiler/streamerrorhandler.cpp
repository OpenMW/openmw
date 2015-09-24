#include "streamerrorhandler.hpp"

#include "tokenloc.hpp"

namespace Compiler
{
    // Report error to the user.

    void StreamErrorHandler::report (const std::string& message, const TokenLoc& loc,
        Type type)
    {
        if (type==ErrorMessage)
            mStream << "error ";
        else
            mStream << "warning ";

        mStream
            << "line " << loc.mLine+1 << ", column " << loc.mColumn+1
            << " (" << loc.mLiteral << ")" << std::endl
            << "    " << message << std::endl;
    }

    // Report a file related error

    void StreamErrorHandler::report (const std::string& message, Type type)
    {
        if (type==ErrorMessage)
            mStream << "error ";
        else
            mStream << "warning ";

        mStream
            << "file:" << std::endl
            << "    " << message << std::endl;
    }

    StreamErrorHandler::StreamErrorHandler (std::ostream& ErrorStream) : mStream (ErrorStream) {}
}
