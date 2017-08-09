#ifndef COMPILER_SKIPPARSER_H_INCLUDED
#define COMPILER_SKIPPARSER_H_INCLUDED

#include "parser.hpp"

namespace Compiler
{
    // \brief Skip parser for skipping a line
    //
    // This parser is mainly intended for skipping the rest of a faulty line.

    class SkipParser : public Parser
    {
        public:

            SkipParser (ErrorHandler& errorHandler, const Context& context);

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

