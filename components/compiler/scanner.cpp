#include "scanner.hpp"

#include <cassert>
#include <sstream>

#include "errorhandler.hpp"
#include "exception.hpp"
#include "extensions.hpp"
#include "parser.hpp"

#include <components/misc/strings/conversion.hpp>
#include <components/misc/strings/lower.hpp>

namespace
{
    bool startsComparisonOperator(const Compiler::MultiChar& c)
    {
        return c == '=' || c == '<' || c == '>' || c == '!';
    }

    bool validComparisonOperatorCharacter(const Compiler::MultiChar& c)
    {
        return startsComparisonOperator(c) || c == ' ' || c == '\t';
    }
}

namespace Compiler
{
    bool Scanner::get(MultiChar& c)
    {
        if (!c.getFrom(mStream))
            return false;

        mPrevLoc = mLoc;

        if (c == '\n')
        {
            mStrictKeywords = false;
            mTolerantNames = false;
            mExpectName = false;
            mLoc.mColumn = 0;
            ++mLoc.mLine;
            mLoc.mLiteral.clear();
            mIgnoreSpecial = true;
        }
        else
        {
            ++mLoc.mColumn;
            c.appendTo(mLoc.mLiteral);
        }

        return true;
    }

    void Scanner::putback(MultiChar& c)
    {
        c.putback(mStream);
        mLoc = mPrevLoc;
    }

    bool Scanner::scanToken(Parser& parser)
    {
        switch (mPutback)
        {
            case Putback_Special:
            {
                mPutback = Putback_None;
                // Replicate behaviour from scanSpecial so putting something back doesn't change the way it's handled
                if (mExpectName && (mPutbackCode == S_member || mPutbackCode == S_minus))
                {
                    mExpectName = false;
                    bool cont = false;
                    bool tolerant = mTolerantNames;
                    mTolerantNames = true;
                    MultiChar c{ mPutbackCode == S_member ? '.' : '-' };
                    scanName(c, parser, cont);
                    mTolerantNames = tolerant;
                    return cont;
                }
                return parser.parseSpecial(mPutbackCode, mPutbackLoc, *this);
            }

            case Putback_Integer:

                mPutback = Putback_None;
                return parser.parseInt(mPutbackInteger, mPutbackLoc, *this);

            case Putback_Float:

                mPutback = Putback_None;
                return parser.parseFloat(mPutbackFloat, mPutbackLoc, *this);

            case Putback_Name:

                mPutback = Putback_None;
                return parser.parseName(mPutbackName, mPutbackLoc, *this);

            case Putback_Keyword:

                mPutback = Putback_None;
                return parser.parseKeyword(mPutbackCode, mPutbackLoc, *this);

            case Putback_None:

                break;
        }

        MultiChar c;

        if (!get(c))
        {
            parser.parseEOF(*this);
            return false;
        }
        else if (c == ';')
        {
            std::string comment;

            c.appendTo(comment);

            while (get(c))
            {
                if (c == '\n')
                {
                    putback(c);
                    break;
                }
                else
                    c.appendTo(comment);
            }

            TokenLoc loc(mLoc);
            mLoc.mLiteral.clear();

            return parser.parseComment(comment, loc, *this);
        }
        else if (c.isWhitespace())
        {
            mLoc.mLiteral.clear();
            return true;
        }
        else if (c == ':')
        {
            // treat : as a whitespace :(
            mLoc.mLiteral.clear();
            return true;
        }
        else if (c.isAlpha() || c == '_' || c == '"')
        {
            mIgnoreSpecial = false;
            bool cont = false;

            if (scanName(c, parser, cont))
            {
                mLoc.mLiteral.clear();
                return cont;
            }
        }
        else if (c.isDigit())
        {
            mIgnoreSpecial = false;
            bool cont = false;

            bool scanned = mExpectName ? scanName(c, parser, cont) : scanInt(c, parser, cont);
            if (scanned)
            {
                mLoc.mLiteral.clear();
                return cont;
            }
        }
        else if (c == 13) // linux compatibility hack
        {
            return true;
        }
        else
        {
            bool cont = false;

            if (scanSpecial(c, parser, cont))
            {
                mLoc.mLiteral.clear();
                return cont;
            }
        }

        TokenLoc loc(mLoc);
        mLoc.mLiteral.clear();

        mErrorHandler.error("Syntax error", loc);
        throw SourceException();
    }

    bool Scanner::scanInt(MultiChar& c, Parser& parser, bool& cont)
    {
        assert(c != '\0');
        std::string value;
        c.appendTo(value);

        while (get(c))
        {
            if (c.isDigit())
            {
                c.appendTo(value);
            }
            else if (!c.isMinusSign() && isStringCharacter(c))
            {
                /// workaround that allows names to begin with digits
                return scanName(c, parser, cont, value);
            }
            else if (c == '.')
            {
                return scanFloat(value, parser, cont);
            }
            else
            {
                putback(c);
                break;
            }
        }

        TokenLoc loc(mLoc);
        mLoc.mLiteral.clear();

        cont = parser.parseInt(Misc::StringUtils::toNumeric<int>(value, 0), loc, *this);

        return true;
    }

    bool Scanner::scanFloat(const std::string& intValue, Parser& parser, bool& cont)
    {
        std::string value = intValue + ".";

        MultiChar c;

        bool empty = intValue.empty() || intValue == "-";
        bool error = false;

        while (get(c))
        {
            if (c.isDigit())
            {
                c.appendTo(value);
                empty = false;
            }
            else if (c.isAlpha() || c == '_')
                error = true;
            else
            {
                putback(c);
                break;
            }
        }

        if (empty || error)
            return false;

        TokenLoc loc(mLoc);
        mLoc.mLiteral.clear();

        cont = parser.parseFloat(Misc::StringUtils::toNumeric<float>(value, 0.0f), loc, *this);

        return true;
    }

    static const char* sKeywords[] = {
        "begin",
        "end",
        "short",
        "long",
        "float",
        "if",
        "endif",
        "else",
        "elseif",
        "while",
        "endwhile",
        "return",
        "messagebox",
        "set",
        "to",
        nullptr,
    };

    bool Scanner::scanName(MultiChar& c, Parser& parser, bool& cont, std::string name)
    {
        c.appendTo(name);

        if (!scanName(name))
            return false;
        else if (name.empty())
            return true;

        TokenLoc loc(mLoc);
        mLoc.mLiteral.clear();

        if (name.size() >= 2 && name[0] == '"' && name[name.size() - 1] == '"')
        {
            name = name.substr(1, name.size() - 2);
            // allow keywords enclosed in ""
            /// \todo optionally disable
            if (mStrictKeywords)
            {
                cont = parser.parseName(name, loc, *this);
                return true;
            }
        }

        int i = 0;

        std::string lowerCase = Misc::StringUtils::lowerCase(name);
        bool isKeyword = false;
        for (; sKeywords[i]; ++i)
            if (lowerCase == sKeywords[i])
            {
                isKeyword = true;
                break;
            }

        // Russian localization and some mods use a quirk - add newline character directly
        // to compiled bytecode via HEX-editor to implement multiline messageboxes.
        // Of course, original editor can not compile such script.
        // Allow messageboxes to bypass the "incomplete string or name" error.
        if (lowerCase == "messagebox")
            enableIgnoreNewlines();
        else if (isKeyword)
            mIgnoreNewline = false;

        if (sKeywords[i])
        {
            cont = parser.parseKeyword(i, loc, *this);
            return true;
        }

        if (mExtensions)
        {
            if (int keyword = mExtensions->searchKeyword(lowerCase))
            {
                cont = parser.parseKeyword(keyword, loc, *this);
                return true;
            }
        }

        cont = parser.parseName(name, loc, *this);

        return true;
    }

    bool Scanner::scanName(std::string& name)
    {
        MultiChar c;
        bool error = false;

        while (get(c))
        {
            if (!name.empty() && name[0] == '"')
            {
                if (c == '"')
                {
                    c.appendTo(name);
                    break;
                }
                // ignoring escape sequences for now, because they are messing up stupid Windows path names.
                //                else if (c=='\\')
                //                {
                //                    if (!get (c))
                //                    {
                //                        error = true;
                //                        mErrorHandler.error ("incomplete escape sequence", mLoc);
                //                        break;
                //                    }
                //                }
                else if (c == '\n')
                {
                    if (mIgnoreNewline)
                        mErrorHandler.warning("string contains newline character, make sure that it is intended", mLoc);
                    else
                    {
                        bool allWhitespace = true;
                        for (size_t i = 1; i < name.size(); i++)
                        {
                            // ignore comments
                            if (name[i] == ';')
                                break;
                            else if (name[i] != '\t' && name[i] != ' ' && name[i] != '\r')
                            {
                                allWhitespace = false;
                                break;
                            }
                        }
                        if (allWhitespace)
                        {
                            name.clear();
                            mLoc.mLiteral.clear();
                            mErrorHandler.warning("unterminated empty string", mLoc);
                            return true;
                        }

                        error = true;
                        mErrorHandler.error("incomplete string or name", mLoc);
                        break;
                    }
                }
            }
            else if (!(c == '"' && name.empty()))
            {
                if (!isStringCharacter(c) && !(mTolerantNames && (c == '.' || c == '-')))
                {
                    putback(c);
                    break;
                }
            }

            c.appendTo(name);
        }

        return !error;
    }

    bool Scanner::scanSpecial(MultiChar& c, Parser& parser, bool& cont)
    {
        bool expectName = mExpectName;
        mExpectName = false;
        int special = -1;

        if (c == '\n')
            special = S_newline;
        else if (mIgnoreSpecial)
        {
            // Ignore junk special characters
            TokenLoc loc = mLoc;
            while (get(c))
            {
                if (c.isAlpha() || c == '_' || c == '"' || c.isDigit() || c == ';' || c == '\n')
                {
                    putback(c);
                    break;
                }
                c.appendTo(loc.mLiteral);
            }
            mErrorHandler.warning("Stray special character at start of line", loc);
            cont = true;
            return true;
        }
        else if (c == '(' || c == '[') /// \todo option to disable the use of [ as alias for (
            special = S_open;
        else if (c == ')' || c == ']') /// \todo option to disable the use of ] as alias for )
            special = S_close;
        else if (c == '.')
        {
            MultiChar next;
            // check, if this starts a float literal
            if (get(next))
            {
                putback(next);

                if (next.isDigit())
                    return scanFloat("", parser, cont);
            }

            special = S_member;
        }
        else if (startsComparisonOperator(c))
        {
            TokenLoc loc = mLoc;
            std::string op;
            c.appendTo(op);
            while (get(c))
            {
                if (validComparisonOperatorCharacter(c))
                    c.appendTo(op);
                else
                {
                    putback(c);
                    break;
                }
            }
            size_t end = op.size();
            while (end > 0 && (op[end - 1] == '\t' || op[end - 1] == ' '))
                --end;
            switch (op[0])
            {
                case '=':
                    special = S_cmpEQ;
                    if (end != 2 || op[1] != '=')
                        mErrorHandler.warning("invalid operator " + op.substr(0, end) + ", treating it as ==", loc);
                    break;
                case '<':
                    special = S_cmpLT;
                    if (op[1] == '=')
                    {
                        special = S_cmpLE;
                        if (end != 2)
                            mErrorHandler.warning("invalid operator " + op.substr(0, end) + ", treating it as <=", loc);
                    }
                    else if (end != 1)
                        mErrorHandler.warning("invalid operator " + op.substr(0, end) + ", treating it as <", loc);
                    break;
                case '>':
                    special = S_cmpGT;
                    if (op[1] == '=')
                    {
                        special = S_cmpGE;
                        if (end != 2)
                            mErrorHandler.warning("invalid operator " + op.substr(0, end) + ", treating it as >=", loc);
                    }
                    else if (end != 1)
                        mErrorHandler.warning("invalid operator " + op.substr(0, end) + ", treating it as >", loc);
                    break;
                case '!':
                    special = S_cmpNE;
                    if (end != 2 || op[1] != '=')
                        mErrorHandler.warning("invalid operator " + op.substr(0, end) + ", treating it as !=", loc);
                    break;
                default:
                    return false;
            }
        }
        else if (c.isMinusSign())
        {
            MultiChar next;
            if (get(next))
            {
                if (next == '>')
                    special = S_ref;
                else
                {
                    putback(next);
                    special = S_minus;
                }
            }
            else
                special = S_minus;
        }
        else if (c == '+')
            special = S_plus;
        else if (c == '*')
            special = S_mult;
        else if (c == '/')
            special = S_div;
        else
            return false;

        if (special == S_newline)
            mLoc.mLiteral = "<newline>";
        else if (expectName && (special == S_member || special == S_minus))
        {
            bool tolerant = mTolerantNames;
            mTolerantNames = true;
            bool out = scanName(c, parser, cont);
            mTolerantNames = tolerant;
            return out;
        }

        TokenLoc loc(mLoc);
        mLoc.mLiteral.clear();

        cont = parser.parseSpecial(special, loc, *this);

        return true;
    }

    bool Scanner::isStringCharacter(MultiChar& c, bool lookAhead)
    {
        if (lookAhead && c.isMinusSign())
        {
            /// \todo disable this when doing more stricter compiling. Also, find out who is
            /// responsible for allowing it in the first place and meet up with that person in
            /// a dark alley.
            MultiChar next;
            if (next.peek(mStream) && isStringCharacter(next, false))
                return true;
        }

        return c.isAlpha() || c.isDigit() || c == '_' ||
            /// \todo disable this when doing more stricter compiling
            c == '`' || c == '\'';
    }

    // constructor

    Scanner::Scanner(ErrorHandler& errorHandler, std::istream& inputStream, const Extensions* extensions)
        : mErrorHandler(errorHandler)
        , mStream(inputStream)
        , mExtensions(extensions)
        , mPutback(Putback_None)
        , mPutbackCode(0)
        , mPutbackInteger(0)
        , mPutbackFloat(0)
        , mStrictKeywords(false)
        , mTolerantNames(false)
        , mIgnoreNewline(false)
        , mExpectName(false)
        , mIgnoreSpecial(true)
    {
    }

    void Scanner::scan(Parser& parser)
    {
        while (scanToken(parser))
            ;
        mExpectName = false;
    }

    void Scanner::putbackSpecial(int code, const TokenLoc& loc)
    {
        mPutback = Putback_Special;
        mPutbackCode = code;
        mPutbackLoc = loc;
    }

    void Scanner::putbackInt(int value, const TokenLoc& loc)
    {
        mPutback = Putback_Integer;
        mPutbackInteger = value;
        mPutbackLoc = loc;
    }

    void Scanner::putbackFloat(float value, const TokenLoc& loc)
    {
        mPutback = Putback_Float;
        mPutbackFloat = value;
        mPutbackLoc = loc;
    }

    void Scanner::putbackName(const std::string& name, const TokenLoc& loc)
    {
        mPutback = Putback_Name;
        mPutbackName = name;
        mPutbackLoc = loc;
    }

    void Scanner::putbackKeyword(int keyword, const TokenLoc& loc)
    {
        mPutback = Putback_Keyword;
        mPutbackCode = keyword;
        mPutbackLoc = loc;
    }

    void Scanner::listKeywords(std::vector<std::string>& keywords)
    {
        for (int i = 0; Compiler::sKeywords[i]; ++i)
            keywords.emplace_back(Compiler::sKeywords[i]);

        if (mExtensions)
            mExtensions->listKeywords(keywords);
    }

    void Scanner::enableIgnoreNewlines()
    {
        mIgnoreNewline = true;
    }

    void Scanner::enableStrictKeywords()
    {
        mStrictKeywords = true;
    }

    void Scanner::enableTolerantNames()
    {
        mTolerantNames = true;
    }

    void Scanner::enableExpectName()
    {
        mExpectName = true;
    }
}
