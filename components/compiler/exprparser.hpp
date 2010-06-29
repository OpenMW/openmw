#ifndef COMPILER_EXPRPARSER_H_INCLUDED
#define COMPILER_EXPRPARSER_H_INCLUDED

#include <vector>

#include <components/interpreter/types.hpp>

#include "parser.hpp"

namespace Compiler
{
    class Locals;
    class Literals;

    class ExprParser : public Parser
    {
            struct Operand
            {
                char mType;
                int mInteger;
                float mFloat;
            };
            
            Locals& mLocals;  
            Literals& mLiterals;
            std::vector<Operand> mOperands;
            
        public:
    
            ExprParser (ErrorHandler& errorHandler, Context& context, Locals& locals,
                Literals& literals);
            ///< constructor

            char getType() const;
            ///< Return type of parsed expression ('l' integer, 'f' float)

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
            
            char write (std::vector<Interpreter::Type_Code>& code);
            ///< Generate code for parsed expression.
            /// \return Type ('l': integer, 'f': float)
    };
}

#endif
