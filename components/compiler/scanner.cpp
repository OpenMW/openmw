
#include "scanner.hpp"

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
            while (get (c))
            {
                if (c=='\n')
                {
                    putback (c);
                    break;
                }
            }

            mLoc.mLiteral.clear();

            return true;
        }
        else if (isWhitespace (c))
        {
            mLoc.mLiteral.clear();
            return true;
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
        else if (std::isalpha (c) || c=='_' || c=='"')
        {
            bool cont = false;

            if (scanName (c, parser, cont))
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
        std::string value;

        value += c;
        bool empty = false;

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
            else if (c=='.' && !error)
            {
                return scanFloat (value, parser, cont);
            }
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

    static const char *keywords[] =
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

        if (!scanName (c, name))
            return false;

        TokenLoc loc (mLoc);
        mLoc.mLiteral.clear();

        if (name.size()>=2 && name[0]=='"' && name[name.size()-1]=='"')
        {
            name = name.substr (1, name.size()-2);
            cont = parser.parseName (name, loc, *this);
            return true;
        }

        int i = 0;

        std::string lowerCase = Misc::StringUtils::lowerCase(name);

        for (; keywords[i]; ++i)
            if (lowerCase==keywords[i])
                break;

        if (keywords[i])
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

    bool Scanner::scanName (char c, std::string& name)
    {
        bool first = false;
        bool error = false;

        name.clear();

        putback (c);

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
//                        mErrorHandler.error ("incomplete escape sequence", mLoc);
//                        break;
//                    }
//                }
                else if (c=='\n')
                {
                    mErrorHandler.error ("incomplete string or name", mLoc);
                    break;
                }
            }
            else if (!(c=='"' && name.empty()))
            {
                if (!(std::isalpha (c) || std::isdigit (c) || c=='_'))
                {
                    putback (c);
                    break;
                }

                if (first && std::isdigit (c))
                    error = true;
            }

            name += c;
            first = false;
        }

        return !error;
    }

    bool Scanner::scanSpecial (char c, Parser& parser, bool& cont)
    {
        int special = -1;

        if (c=='\n')
            special = S_newline;
        else if (c=='(')
            special = S_open;
        else if (c==')')
            special = S_close;
        else if (c=='.')
            special = S_member;
        else if (c=='=')
        {
            if (get (c))
            {
                if (c=='=')
                    special = S_cmpEQ;
                else
                {
                    special = S_cmpEQ;
                    putback (c);
//                    return false;
// Allow = as synonym for ==. \todo optionally disable for post-1.0 scripting improvements.
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
        else if (c=='<')
        {
            if (get (c))
            {
                if (c=='=')
                    special = S_cmpLE;
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
                if (c=='=')
                    special = S_cmpGE;
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

    bool Scanner::isWhitespace (char c)
    {
        return c==' ' || c=='\t';
    }

    // constructor

    Scanner::Scanner (ErrorHandler& errorHandler, std::istream& inputStream,
        const Extensions *extensions)
    : mErrorHandler (errorHandler), mStream (inputStream), mExtensions (extensions),
      mPutback (Putback_None)
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
        for (int i=0; Compiler::keywords[i]; ++i)
            keywords.push_back (Compiler::keywords[i]);

        if (mExtensions)
            mExtensions->listKeywords (keywords);
    }
}
