#ifndef COMPILER_SCRIPTPARSER_H_INCLUDED
#define COMPILER_SCRIPTPARSER_H_INCLUDED

#include "controlparser.hpp"
#include "lineparser.hpp"
#include "output.hpp"
#include "parser.hpp"

namespace Compiler
{
    class Locals;

    // Script parser, to be used in dialogue scripts and as part of FileParser

    class ScriptParser : public Parser
    {
        Output mOutput;
        LineParser mLineParser;
        ControlParser mControlParser;
        bool mEnd;

    public:
        /// \param end of script is marked by end keyword.
        ScriptParser(ErrorHandler& errorHandler, const Context& context, Locals& locals, bool end = false);

        Interpreter::Program getProgram() const;

        bool parseName(const std::string& name, const TokenLoc& loc, Scanner& scanner) override;
        ///< Handle a name token.
        /// \return fetch another token?

        bool parseKeyword(int keyword, const TokenLoc& loc, Scanner& scanner) override;
        ///< Handle a keyword token.
        /// \return fetch another token?

        bool parseSpecial(int code, const TokenLoc& loc, Scanner& scanner) override;
        ///< Handle a special character token.
        /// \return fetch another token?

        void parseEOF(Scanner& scanner) override;
        ///< Handle EOF token.

        void reset() override;
        ///< Reset parser to clean state.
    };
}

#endif
