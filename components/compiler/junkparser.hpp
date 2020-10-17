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

            bool parseInt (int value, const TokenLoc& loc, Scanner& scanner) override;
            ///< Handle an int token.
            /// \return fetch another token?

            bool parseFloat (float value, const TokenLoc& loc, Scanner& scanner) override;
            ///< Handle a float token.
            /// \return fetch another token?

            bool parseName (const std::string& name, const TokenLoc& loc,
                Scanner& scanner) override;
            ///< Handle a name token.
            /// \return fetch another token?

            bool parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner) override;
            ///< Handle a keyword token.
            /// \return fetch another token?

            bool parseSpecial (int code, const TokenLoc& loc, Scanner& scanner) override;
            ///< Handle a special character token.
            /// \return fetch another token?
    };
}

#endif
