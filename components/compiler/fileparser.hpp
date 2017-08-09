#ifndef COMPILER_FILEPARSER_H_INCLUDED
#define COMPILER_FILEPARSER_H_INCLUDED

#include "parser.hpp"
#include "scriptparser.hpp"
#include "locals.hpp"
#include "literals.hpp"

namespace Compiler
{
    // Top-level parser, to be used for global scripts, local scripts and targeted scripts
    
    class FileParser : public Parser
    {
            enum State
            {
                BeginState, NameState, BeginCompleteState, EndNameState,
                EndCompleteState
            };
            
            ScriptParser mScriptParser;
            State mState;
            std::string mName;
            Locals mLocals;

        public:
        
            FileParser (ErrorHandler& errorHandler, Context& context);

            std::string getName() const;
            ///< Return script name.

            void getCode (std::vector<Interpreter::Type_Code>& code) const;
            ///< store generated code in \a code.
            
            const Locals& getLocals() const;
            ///< get local variable declarations.

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
