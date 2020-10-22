#ifndef COMPILER_EXPRPARSER_H_INCLUDED
#define COMPILER_EXPRPARSER_H_INCLUDED

#include <vector>

#include <components/interpreter/types.hpp>

#include "parser.hpp"
#include "tokenloc.hpp"

namespace Compiler
{
    class Locals;
    class Literals;

    class ExprParser : public Parser
    {
            Locals& mLocals;
            Literals& mLiterals;
            std::vector<char> mOperands;
            std::vector<char> mOperators;
            bool mNextOperand;
            TokenLoc mTokenLoc;
            std::vector<Interpreter::Type_Code> mCode;
            bool mFirst;
            bool mArgument;
            std::string mExplicit;
            bool mRefOp;
            bool mMemberOp;

            static int getPriority (char op) ;

            char getOperandType (int Index = 0) const;

            char getOperator() const;

            bool isOpen() const;

            void popOperator();

            void popOperand();

            void replaceBinaryOperands();

            void pop();

            void pushIntegerLiteral (int value);

            void pushFloatLiteral (float value);

            void pushBinaryOperator (char c);

            void close();

            int parseArguments (const std::string& arguments, Scanner& scanner);

            bool handleMemberAccess (const std::string& name);

        public:

            ExprParser (ErrorHandler& errorHandler, const Context& context, Locals& locals,
                Literals& literals, bool argument = false);
            ///< constructor
            /// \param argument Parser is used to parse function- or instruction-
            /// arguments (this influences the precedence rules).

            char getType() const;
            ///< Return type of parsed expression ('l' integer, 'f' float)

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

            void reset() override;
            ///< Reset parser to clean state.

            char append (std::vector<Interpreter::Type_Code>& code);
            ///< Generate code for parsed expression.
            /// \return Type ('l': integer, 'f': float)

            int parseArguments (const std::string& arguments, Scanner& scanner,
                std::vector<Interpreter::Type_Code>& code, int ignoreKeyword = -1);
            ///< Parse sequence of arguments specified by \a arguments.
            /// \param arguments Uses ScriptArgs typedef
            /// \see Compiler::ScriptArgs
            /// \param invert Store arguments in reverted order.
            /// \param ignoreKeyword A keyword that is seen as junk
            /// \return number of optional arguments

            const TokenLoc& getTokenLoc() const;
    };
}

#endif
