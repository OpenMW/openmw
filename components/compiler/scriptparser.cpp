#include "scriptparser.hpp"

#include "scanner.hpp"
#include "skipparser.hpp"
#include "errorhandler.hpp"

namespace Compiler
{
    ScriptParser::ScriptParser (ErrorHandler& errorHandler, const Context& context,
        Locals& locals, bool end)
    : Parser (errorHandler, context), mOutput (locals),
      mLineParser (errorHandler, context, locals, mOutput.getLiterals(), mOutput.getCode()),
      mControlParser (errorHandler, context, locals, mOutput.getLiterals()),
      mEnd (end)
    {}

    void ScriptParser::getCode (std::vector<Interpreter::Type_Code>& code) const
    {
        mOutput.getCode (code);
    }

    bool ScriptParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        mLineParser.reset();
        if (mLineParser.parseName (name, loc, scanner))
            scanner.scan (mLineParser);

        return true;
    }

    bool ScriptParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        if (keyword==Scanner::K_while || keyword==Scanner::K_if || keyword==Scanner::K_elseif)
        {
            mControlParser.reset();
            if (mControlParser.parseKeyword (keyword, loc, scanner))
                scanner.scan (mControlParser);

            mControlParser.appendCode (mOutput.getCode());

            return true;
        }

        /// \todo add an option to disable this nonsense
        if (keyword==Scanner::K_endif)
        {
            // surplus endif
            getErrorHandler().warning ("endif without matching if/elseif", loc);

            SkipParser skip (getErrorHandler(), getContext());
            scanner.scan (skip);
            return true;
        }

        if (keyword==Scanner::K_end && mEnd)
        {
            return false;
        }

        mLineParser.reset();
        if (mLineParser.parseKeyword (keyword, loc, scanner))
            scanner.scan (mLineParser);

        return true;
    }

    bool ScriptParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline) // empty line
            return true;

        if (code==Scanner::S_open) /// \todo Option to switch this off
        {
            scanner.putbackSpecial (code, loc);
            return parseKeyword (Scanner::K_if, loc, scanner);
        }

        mLineParser.reset();
        if (mLineParser.parseSpecial (code, loc, scanner))
            scanner.scan (mLineParser);

        return true;
    }

    void ScriptParser::parseEOF (Scanner& scanner)
    {
        if (mEnd)
            Parser::parseEOF (scanner);
    }

    void ScriptParser::reset()
    {
        mLineParser.reset();
        mOutput.clear();
    }
}
