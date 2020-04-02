#ifndef COMPILER_STREAMERRORHANDLER_H_INCLUDED
#define COMPILER_STREAMERRORHANDLER_H_INCLUDED

#include <ostream>

#include "errorhandler.hpp"

namespace Compiler
{
    class ContextRestore;
    /// \brief Error handler implementation: Write errors into logging stream

    class StreamErrorHandler : public ErrorHandler
    {
            std::string mContext;

        // not implemented

            StreamErrorHandler (const StreamErrorHandler&);
            StreamErrorHandler& operator= (const StreamErrorHandler&);

            virtual void report (const std::string& message, const TokenLoc& loc, Type type);
            ///< Report error to the user.

            virtual void report (const std::string& message, Type type);
            ///< Report a file related error

        public:

            ContextRestore setContext(const std::string& context, bool restore = false);

        // constructors

            StreamErrorHandler ();
            ///< constructor
    };

    class ContextRestore
    {
            StreamErrorHandler* mHandler;
            const std::string mContext;
        public:
            ContextRestore (StreamErrorHandler* handler, const std::string& context);

            ~ContextRestore();
    };
}

#endif
