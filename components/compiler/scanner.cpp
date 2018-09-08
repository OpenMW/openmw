#include "scanner.hpp"

#include <cassert>
#include <cctype>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "exception.hpp"
#include "errorhandler.hpp"
#include "parser.hpp"
#include "extensions.hpp"

#include <components/misc/stringops.hpp>

namespace Compiler
{
    bool Scanner::get (char& c)
    {
        mStream.get (c);

        if (!mStream.good())
            return false;

        mPrevLoc =mLoc;

        if (c=='\n')
        {
            mStrictKeywords = false;
            mTolerantNames = false;
            mLoc.mColumn = 0;
            ++mLoc.mLine;
            mLoc.mLiteral.clear();
        }
        else
        {
            ++mLoc.mColumn;
            mLoc.mLiteral += c;
        }

        return true;
    }

    void Scanner::putback (char c)
    {
        mStream.putback (c);
        mLoc = mPrevLoc;
    }

    bool Scanner::scanToken (Parser& parser)
    {
        switch (mPutback)
        {
            case Putback_Special:

                mPutback = Putback_None;
                return parser.parseSpecial (mPutbackCode, mPutbackLoc, *this);

            case Putback_Integer:

                mPutback = Putback_None;
                return parser.parseInt (mPutbackInteger, mPutbackLoc, *this);

            case Putback_Float:

                mPutback = Putback_None;
                return parser.parseFloat (mPutbackFloat, mPutbackLoc, *this);

            case Putback_Name:

                mPutback = Putback_None;
                return parser.parseName (mPutbackName, mPutbackLoc, *this);

            case Putback_Keyword:

                mPutback = Putback_None;
                return parser.parseKeyword (mPutbackCode, mPutbackLoc, *this);

            case Putback_None:

                break;
        }

        char c;

        if (!get (c))
        {
            parser.parseEOF (*this);
            return false;
        }
        else if (c==';')
        {
            std::string comment;

            comment += c;

            while (get (c))
            {
                if (c=='\n')
                {
                    putback (c);
                    break;
                }
                else
                    comment += c;
            }

            TokenLoc loc (mLoc);
            mLoc.mLiteral.clear();

            return parser.parseComment (comment, loc, *this);
        }
        else if (isWhitespace (c))
        {
            mLoc.mLiteral.clear();
            return true;
        }
        else if (c==':')
        {
            // treat : as a whitespace :(
            mLoc.mLiteral.clear();
            return true;
        }
        else if (std::isalpha (c) || c=='_' || c=='"')
        {
            bool cont = false;

            if (scanName (c, parser, cont))
            {
                mLoc.mLiteral.clear();
                return cont;
            }
        }
        else if (std::isdigit (c))
        {
            bool cont = false;

            if (scanInt (c, parser, cont))
            {
                mLoc.mLiteral.clear();
                return cont;
            }
        }
        else if (c==13) // linux compatibility hack
        {
            return true;
        }
        else
        {
            bool cont = false;

            if (scanSpecial (c, parser, cont))
            {
                mLoc.mLiteral.clear();
                return cont;
            }
        }

        TokenLoc loc (mLoc);
        mLoc.mLiteral.clear();

        mErrorHandler.error ("syntax error", loc);
        throw SourceException();
    }

    bool Scanner::scanInt (char c, Parser& parser, bool& cont)
    {
        assert(c != '\0');
        std::string value;
        value += c;

        bool error = false;

        while (get (c))
        {
            if (std::isdigit (c))
            {
                value += c;
            }
            else if (c!='-' && isStringCharacter (c))
            {
                error = true;
                value += c;
            }
            else if (c=='.')
            {
                if (error)
                {
                    putback (c);
                    break;
                }
                return scanFloat (value, parser, cont);
            }
            else
            {
                putback (c);
                break;
            }
        }

        if (error)
        {
            /// workaround that allows names to begin with digits
            /// \todo disable
            TokenLoc loc (mLoc);
            mLoc.mLiteral.clear();
            cont = parser.parseName (value, loc, *this);
            return true;
//            return false;
        }

        TokenLoc loc (mLoc);
        mLoc.mLiteral.clear();

        std::istringstream stream (value);

        int intValue = 0;
        stream >> intValue;

        cont = parser.parseInt (intValue, loc, *this);
        return true;
    }

    bool Scanner::scanFloat (const std::string& intValue, Parser& parser, bool& cont)
    {
        std::string value = intValue + ".";

        char c;

        bool empty = intValue.empty() || intValue=="-";
        bool error = false;

        while (get (c))
        {
            if (std::isdigit (c))
            {
                value += c;
                empty = false;
            }
            else if (std::isalpha (c) || c=='_')
                error = true;
            else
            {
                putback (c);
                break;
            }
        }

        if (empty || error)
            return false;

        TokenLoc loc (mLoc);
        mLoc.mLiteral.clear();

        std::istringstream stream (value);

        float floatValue = 0;
        stream >> floatValue;

        cont = parser.parseFloat (floatValue, loc, *this);
        return true;
    }

    static const char *sKeywords[] =
    {
        "begin", "end",
        "short", "long", "float",
        "if", "endif", "else", "elseif",
        "while", "endwhile",
        "return",
        "messagebox",
        "set", "to",
        "getsquareroot",
        "menumode",
        "random",
        "startscript", "stopscript", "scriptrunning",
        "getdistance",
        "getsecondspassed",
        "enable", "disable", "getdisabled",
        0
    };

    bool Scanner::scanName (char c, Parser& parser, bool& cont)
    {
        std::string name;
        name += c;

        if (!scanName (name))
            return false;

        TokenLoc loc (mLoc);
        mLoc.mLiteral.clear();

        if (name.size()>=2 && name[0]=='"' && name[name.size()-1]=='"')
        {
            name = name.substr (1, name.size()-2);
// allow keywords enclosed in ""
/// \todo optionally disable
            if (mStrictKeywords)
            {
                cont = parser.parseName (name, loc, *this);
                return true;
            }
        }

        int i = 0;

        std::string lowerCase = Misc::StringUtils::lowerCase(name);
        bool isKeyword = false;
        for (; sKeywords[i]; ++i)
            if (lowerCase==sKeywords[i])
            {
                isKeyword = true;
                break;
            }

        // Russian localization and some mods use a quirk - add newline character directly
        // to compiled bytecode via HEX-editor to implement multiline messageboxes.
        // Of course, original editor will not compile such script.
        // Allow messageboxes to bybass the "incomplete string or name" error.
        if (lowerCase == "messagebox")
            enableIgnoreNewlines();
        else if (isKeyword)
            mIgnoreNewline = false;

        if (sKeywords[i])
        {
            cont = parser.parseKeyword (i, loc, *this);
            return true;
        }

        if (mExtensions)
        {
            if (int keyword = mExtensions->searchKeyword (lowerCase))
            {
                cont = parser.parseKeyword (keyword, loc, *this);
                return true;
            }
        }

        cont = parser.parseName (name, loc, *this);

        return true;
    }

    bool Scanner::scanName (std::string& name)
    {
        char c;
        bool error = false;

        while (get (c))
        {
            if (!name.empty() && name[0]=='"')
            {
                if (c=='"')
                {
                    name += c;
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
                else if (c=='\n')
                {
                    if (mIgnoreNewline)
                        mErrorHandler.warning ("string contains newline character, make sure that it is intended", mLoc);
                    else
                    {
                        error = true;
                        mErrorHandler.error ("incomplete string or name", mLoc);
                        break;
                    }
                }
            }
            else if (!(c=='"' && name.empty()))
            {
                if (!isStringCharacter (c) && !(mTolerantNames && (c=='.' || c=='-')))
                {
                    putback (c);
                    break;
                }
            }

            name += c;
        }

        return !error;
    }

    bool Scanner::scanSpecial (char c, Parser& parser, bool& cont)
    {
        int special = -1;

        if (c=='\n')
            special = S_newline;
        else if (c=='(' || c=='[') /// \todo option to disable the use of [ as alias for (
            special = S_open;
        else if (c==')' || c==']')  /// \todo option to disable the use of ] as alias for )
            special = S_close;
        else if (c=='.')
        {
            // check, if this starts a float literal
            if (get (c))
            {
                putback (c);

                if (std::isdigit (c))
                    return scanFloat ("", parser, cont);
            }

            special = S_member;
        }
        else if (c=='=')
        {
            if (get (c))
            {
                /// \todo hack to allow a space in comparison operators (add option to disable)
                if (c==' ' && !get (c))
                    special = S_cmpEQ;
                else if (c=='=')
                    special = S_cmpEQ;
                else if (c == '>' || c == '<')  // Treat => and =< as ==
                {
                    special = S_cmpEQ;
                    mErrorHandler.warning (std::string("invalid operator =") + c + ", treating it as ==", mLoc);
                }
                else
                {
                    special = S_cmpEQ;
                    putback (c);
//                    return false;
/// Allow = as synonym for ==. \todo optionally disable for post-1.0 scripting improvements.
                }
            }
            else
            {
                putback (c);
                return false;
            }
        }
        else if (c=='!')
        {
            if (get (c))
            {
                /// \todo hack to allow a space in comparison operators (add option to disable)
                if (c==' ' && !get (c))
                    return false;

                if (c=='=')
                    special = S_cmpNE;
                else
                {
                    putback (c);
                    return false;
                }
            }
            else
                return false;
        }
        else if (c=='-')
        {
            if (get (c))
            {
                if (c=='>')
                    special = S_ref;
                else
                {
                    putback (c);
                    special = S_minus;
                }
            }
            else
                special = S_minus;
        }
        else if (static_cast<unsigned char> (c)==0xe2)
        {
            /// Workaround for some translator who apparently can't keep his minus in order
            /// \todo disable for later script formats
            if (get (c) && static_cast<unsigned char> (c)==0x80 &&
                get (c) && static_cast<unsigned char> (c)==0x93)
            {
                if (get (c))
                {
                    if (c=='>')
                        special = S_ref;
                    else
                    {
                        putback (c);
                        special = S_minus;
                    }
                }
                else
                    special = S_minus;
            }
            else
            {
                mErrorHandler.error ("Invalid character", mLoc);
                return false;
            }
        }
        else if (c=='<')
        {
            if (get (c))
            {
                /// \todo hack to allow a space in comparison operators (add option to disable)
                if (c==' ' && !get (c))
                    special = S_cmpLT;
                else if (c=='=')
                {
                    special = S_cmpLE;

                    if (get (c) && c!='=') // <== is a allowed as an alternative to <=  :(
                        putback (c);
                }
                else if (c == '<' || c == '>') // Treat <> and << as <
                {
                    special = S_cmpLT;
                    mErrorHandler.warning (std::string("invalid operator <") + c + ", treating it as <", mLoc);
                }
                else
                {
                    putback (c);
                    special = S_cmpLT;
                }
            }
            else
                special = S_cmpLT;
        }
        else if (c=='>')
        {
            if (get (c))
            {
                /// \todo hack to allow a space in comparison operators (add option to disable)
                if (c==' ' && !get (c))
                    special = S_cmpGT;
                else if (c=='=')
                {
                    special = S_cmpGE;

                    if (get (c) && c!='=') // >== is a allowed as an alternative to >=  :(
                        putback (c);
                }
                else if (c == '<' || c == '>') // Treat >< and >> as >
                {
                    special = S_cmpGT;
                    mErrorHandler.warning (std::string("invalid operator >") + c + ", treating it as >", mLoc);
                }
                else
                {
                    putback (c);
                    special = S_cmpGT;
                }
            }
            else
                special = S_cmpGT;
        }
        else if (c==',')
            special = S_comma;
        else if (c=='+')
            special = S_plus;
        else if (c=='*')
            special = S_mult;
        else if (c=='/')
            special = S_div;
        else
            return false;

        if (special==S_newline)
            mLoc.mLiteral = "<newline>";

        TokenLoc loc (mLoc);
        mLoc.mLiteral.clear();

        cont = parser.parseSpecial (special, loc, *this);

        return true;
    }

    bool Scanner::isStringCharacter (char c, bool lookAhead)
    {
        return std::isalpha (c) || std::isdigit (c) || c=='_' ||
            /// \todo disable this when doing more stricter compiling
            c=='`' || c=='\'' ||
            /// \todo disable this when doing more stricter compiling. Also, find out who is
            /// responsible for allowing it in the first place and meet up with that person in
            /// a dark alley.
            (c=='-' && (!lookAhead || isStringCharacter (mStream.peek(), false)));
    }

    bool Scanner::isWhitespace (char c)
    {
        return c==' ' || c=='\t';
    }

    // constructor

    Scanner::Scanner (ErrorHandler& errorHandler, std::istream& inputStream,
        const Extensions *extensions)
    : mErrorHandler (errorHandler), mStream (inputStream), mExtensions (extensions),
      mPutback (Putback_None), mPutbackCode(0), mPutbackInteger(0), mPutbackFloat(0),
      mStrictKeywords (false), mTolerantNames (false), mIgnoreNewline(false)
    {
    }

    void Scanner::scan (Parser& parser)
    {
        while (scanToken (parser));
    }

    void Scanner::putbackSpecial (int code, const TokenLoc& loc)
    {
        mPutback = Putback_Special;
        mPutbackCode = code;
        mPutbackLoc = loc;
    }

    void Scanner::putbackInt (int value, const TokenLoc& loc)
    {
        mPutback = Putback_Integer;
        mPutbackInteger = value;
        mPutbackLoc = loc;
    }

    void Scanner::putbackFloat (float value, const TokenLoc& loc)
    {
        mPutback = Putback_Float;
        mPutbackFloat = value;
        mPutbackLoc = loc;
    }

    void Scanner::putbackName (const std::string& name, const TokenLoc& loc)
    {
        mPutback = Putback_Name;
        mPutbackName = name;
        mPutbackLoc = loc;
    }

    void Scanner::putbackKeyword (int keyword, const TokenLoc& loc)
    {
        mPutback = Putback_Keyword;
        mPutbackCode = keyword;
        mPutbackLoc = loc;
    }

    void Scanner::listKeywords (std::vector<std::string>& keywords)
    {
        for (int i=0; Compiler::sKeywords[i]; ++i)
            keywords.push_back (Compiler::sKeywords[i]);

        if (mExtensions)
            mExtensions->listKeywords (keywords);
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
}
