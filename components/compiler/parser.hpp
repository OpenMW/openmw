#ifndef COMPILER_PARSER_H_INCLUDED
#define COMPILER_PARSER_H_INCLUDED

#include <string>

namespace Compiler
{
    class Scanner;
    struct TokenLoc;
    class ErrorHandler;
    class Context;

    /// \brief Parser base class
    ///
    /// This class defines a callback-parser.

    class Parser
    {
            ErrorHandler& mErrorHandler;
            const Context& mContext;
            bool mOptional;
            bool mEmpty;

        protected:

            [[noreturn]] void reportSeriousError (const std::string& message, const TokenLoc& loc);
            ///< Report the error and throw a exception.

            void reportWarning (const std::string& message, const TokenLoc& loc);
            ///< Report the warning without throwing an exception.

            [[noreturn]] void reportEOF();
            ///< Report an unexpected EOF condition.

            ErrorHandler& getErrorHandler();
            ///< Return error handler

            const Context& getContext() const;
            ///< Return context

            static std::string toLower (const std::string& name);

        public:

            Parser (ErrorHandler& errorHandler, const Context& context);
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

            virtual bool parseComment (const std::string& comment, const TokenLoc& loc,
                Scanner& scanner);
            ///< Handle comment token.
            /// \return fetch another token?
            ///
            /// - Default-implementation: ignored (and return true).

            virtual void parseEOF (Scanner& scanner);
            ///< Handle EOF token.
            ///
            /// - Default-implementation: Report an error.

            virtual void reset();
            ///< Reset parser to clean state.

            void setOptional (bool optional);
            ///< Optional mode: If nothign has been parsed yet and an unexpected token is delivered, stop
            /// parsing without raising an exception (after a reset the parser is in non-optional mode).

            void start();
            ///< Mark parser as non-empty (at least one token has been parser).

            bool isEmpty() const;
            ///< Has anything been parsed?
    };
}

#endif
