#ifndef COMPILER_LINEPARSER_H_INCLUDED
#define COMPILER_LINEPARSER_H_INCLUDED

#include <vector>
#include <string>

#include <components/interpreter/types.hpp>
#include <components/misc/messageformatparser.hpp>

#include "parser.hpp"
#include "exprparser.hpp"

namespace Compiler
{
    class Locals;
    class Literals;

    /// \brief Line parser, to be used in console scripts and as part of ScriptParser

    class LineParser : public Parser
    {
            enum State
            {
                BeginState,
                SetState, SetLocalVarState, SetGlobalVarState, SetPotentialMemberVarState,
                SetMemberVarState, SetMemberVarState2,
                MessageState, MessageCommaState, MessageButtonState, MessageButtonCommaState,
                EndState, PotentialExplicitState, ExplicitState, MemberState
            };

            Locals& mLocals;
            Literals& mLiterals;
            std::vector<Interpreter::Type_Code>& mCode;
            State mState;
            std::string mName;
            std::string mMemberName;
            bool mReferenceMember;
            int mButtons;
            std::string mExplicit;
            char mType;
            ExprParser mExprParser;
            bool mAllowExpression;

            void parseExpression (Scanner& scanner, const TokenLoc& loc);

        public:

            LineParser (ErrorHandler& errorHandler, const Context& context, Locals& locals,
                Literals& literals, std::vector<Interpreter::Type_Code>& code,
                bool allowExpression = false);
            ///< \param allowExpression Allow lines consisting of a naked expression
            /// (result is send to the messagebox interface)

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
    };

    class GetArgumentsFromMessageFormat : public ::Misc::MessageFormatParser
    {
        private:
            std::string mArguments;

        protected:
            void visitedPlaceholder(Placeholder placeholder, char padding, int width, int precision, Notation notation) override;
            void visitedCharacter(char c) override {}

        public:
            void process(const std::string& message) override
            {
                mArguments.clear();
                ::Misc::MessageFormatParser::process(message);
            }
            std::string getArguments() const { return mArguments; }
    };
}

#endif
