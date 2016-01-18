#ifndef COMPILER_DISCARDPARSER_H_INCLUDED
#define COMPILER_DISCARDPARSER_H_INCLUDED

#include "parser.hpp"

namespace Compiler
{
    /// \brief Parse a single optional numeric value or string and discard it
    class DiscardParser : public Parser
    {
            enum State
            {
                StartState, CommaState, MinusState
            };

            State mState;

        public:

            DiscardParser (ErrorHandler& errorHandler, const Context& context);

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

            virtual bool parseSpecial (int code, const TokenLoc& loc, Scanner& scanner);
            ///< Handle a special character token.
            /// \return fetch another token?

            virtual void reset();
            ///< Reset parser to clean state.
    };
}

#endif

