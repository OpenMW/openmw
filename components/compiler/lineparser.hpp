#ifndef COMPILER_LINEPARSER_H_INCLUDED
#define COMPILER_LINEPARSER_H_INCLUDED

#include "parser.hpp"

namespace Compiler
{
    class Locals;
    
    /// \brief Line parser, to be used in console scripts and as part of ScriptParser
    
    class LineParser : public Parser
    {
            enum State
            {
                BeginState,
                ShortState, LongState, FloatState,
                EndState
            };

            Locals& mLocals;            
            State mState;
    
        public:
        
            LineParser (ErrorHandler& errorHandler, Context& context, Locals& locals);
    
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
            
            void reset();
            ///< Reset parser to clean state.            
    };
}

#endif
