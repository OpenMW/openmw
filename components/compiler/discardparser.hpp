#ifndef COMPILER_DISCARDPARSER_H_INCLUDED
#define COMPILER_DISCARDPARSER_H_INCLUDED

#include "parser.hpp"
#include "tokenloc.hpp"

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
            TokenLoc mTokenLoc;

        public:

            DiscardParser (ErrorHandler& errorHandler, const Context& context);

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

            bool parseSpecial (int code, const TokenLoc& loc, Scanner& scanner) override;
            ///< Handle a special character token.
            /// \return fetch another token?

            void reset() override;
            ///< Reset parser to clean state.

            /// Returns TokenLoc object for value. If no value has been parsed, the TokenLoc
            /// object will be default initialised.
            const TokenLoc& getTokenLoc() const;
    };
}

#endif
