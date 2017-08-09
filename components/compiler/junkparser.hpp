#ifndef COMPILER_JUNKPARSER_H_INCLUDED
#define COMPILER_JUNKPARSER_H_INCLUDED

#include "parser.hpp"

namespace Compiler
{
    /// \brief Parse an optional single junk token
    class JunkParser : public Parser
    {
            int mIgnoreKeyword;

        public:

            JunkParser (ErrorHandler& errorHandler, const Context& context,
                int ignoreKeyword = -1);

            virtual bool parseInt (int value, const TokenLoc& loc, Scanner& scanner);
            ///< Handle an int token.
            /// \return fetch another token?

            virtual bool parseFloat (float value, const TokenLoc& loc, Scanner& scanner);
            ///< Handle a float token.
            /// \return fetch another token?

            virtual bool parseName (const std::string& name, const TokenLoc& loc,
                Scanner& scanner);
            ///< Handle a name token.
            /// \return fetch another token?

            virtual bool parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner);
            ///< Handle a keyword token.
            /// \return fetch another token?

            virtual bool parseSpecial (int code, const TokenLoc& loc, Scanner& scanner);
            ///< Handle a special character token.
            /// \return fetch another token?
    };
}

#endif
