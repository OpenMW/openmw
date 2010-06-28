#ifndef COMPILER_SCRIPTPARSER_H_INCLUDED
#define COMPILER_SCRIPTPARSER_H_INCLUDED

#include "parser.hpp"

namespace Compiler
{
    // Script parser, to be used in dialogue scripts and as part of FileParser
    
    class ScriptParser : public Parser
    {
            bool mEnd;
            
        public:
        
            /// \param end of script is marked by end keyword.
            ScriptParser (ErrorHandler& errorHandler, Context& context, bool end = false);
    
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

            virtual void parseEOF (Scanner& scanner);
            ///< Handle EOF token.  
            
            void reset();
            ///< Reset parser to clean state.
    };
}

#endif

