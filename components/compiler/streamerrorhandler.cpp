#include "streamerrorhandler.hpp"

#include <sstream>

#include <components/debug/debuglog.hpp>

#include "tokenloc.hpp"

namespace Compiler
{
    // Report error to the user.

    void StreamErrorHandler::report (const std::string& message, const TokenLoc& loc,
        Type type)
    {
        Debug::Level logLevel = Debug::Info; // Usually script warnings are not too important
        if (type == ErrorMessage)
            logLevel = Debug::Error;

        std::stringstream text;

        if (type==ErrorMessage)
            text << "Error: ";
        else
            text << "Warning: ";

        if (!mContext.empty())
            text << mContext << " ";

        text << "line " << loc.mLine+1 << ", column " << loc.mColumn+1
             << " (" << loc.mLiteral << "): " << message;

        Log(logLevel) << text.str();
    }

    // Report a file related error

    void StreamErrorHandler::report (const std::string& message, Type type)
    {
        Debug::Level logLevel = Debug::Info;
        if (type==ErrorMessage)
            logLevel = Debug::Error;

        std::stringstream text;

        if (type==ErrorMessage)
            text << "Error: ";
        else
            text << "Warning: ";

        if (!mContext.empty())
            text << mContext << " ";

        text << "file: " << message << std::endl;

        Log(logLevel) << text.str();
    }

    void StreamErrorHandler::setContext(const std::string &context)
    {
        mContext = context;
    }

    StreamErrorHandler::StreamErrorHandler()  {}

    ContextOverride::ContextOverride(StreamErrorHandler& handler, const std::string& context) : mHandler(handler), mContext(handler.mContext)
    {
        mHandler.setContext(context);
    }

    ContextOverride::~ContextOverride()
    {
        mHandler.setContext(mContext);
    }
}
