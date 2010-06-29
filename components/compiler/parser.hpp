#ifndef COMPILER_PARSER_H_INCLUDED
#define COMPILER_PARSER_H_INCLUDED

#include <string>

namespace Compiler
{
    class Scanner;
    class TokenLoc;
    class ErrorHandler;
    class Context;

    /// \brief Parser base class
    ///
    /// This class defines a callback-parser.

    class Parser
    {
            ErrorHandler& mErrorHandler;
            Context& mContext;

        protected:

            void reportSeriousError (const std::string& message, const TokenLoc& loc);
            ///< Report the error and throw a exception.

            void reportError (const std::string& message, const TokenLoc& loc);
            ///< Report the error

            void reportWarning (const std::string& message, const TokenLoc& loc);
            ///< Report the warning without throwing an exception.

            void reportEOF();
            ///< Report an unexpected EOF condition.

            ErrorHandler& getErrorHandler();
            ///< Return error handler

            Context& getContext();
            ///< Return context

            static std::string toLower (const std::string& name);

        public:

            Parser (ErrorHandler& errorHandler, Context& context);
            ///< constructor

            virtual ~Parser();
		    ///< destructor

            virtual bool parseInt (int value, const TokenLoc& loc, Scanner& scanner);
            ///< Handle an int token.
            /// \return fetch another token?
            ///
            /// - Default-implementation: Report an error.

            virtual bool parseFloat (float value, const TokenLoc& loc, Scanner& scanner);
            ///< Handle a float token.
            /// \return fetch another token?
            ///
            /// - Default-implementation: Report an error.

            virtual bool parseName (const std::string& name, const TokenLoc& loc,
                Scanner& scanner);
            ///< Handle a name token.
            /// \return fetch another token?
            ///
            /// - Default-implementation: Report an error.

            virtual bool parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner);
            ///< Handle a keyword token.
            /// \return fetch another token?
            ///
            /// - Default-implementation: Report an error.

            virtual bool parseSpecial (int code, const TokenLoc& loc, Scanner& scanner);
            ///< Handle a special character token.
            /// \return fetch another token?
            ///
            /// - Default-implementation: Report an error.

            virtual void parseEOF (Scanner& scanner);
            ///< Handle EOF token.
            ///
            /// - Default-implementation: Report an error.
    };
}

#endif
