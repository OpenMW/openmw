
#include "scanner.hpp"

#include <cctype>
#include <sstream>
#include <algorithm>

#include "exception.hpp"
#include "errorhandler.hpp"
#include "parser.hpp"

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
        char c;

        if (!get (c))
        {
            parser.parseEOF (*this);
            return false;
        }
        else if (c==';')
        {
            while (get (c) && c!='\n');

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

    bool Scanner::scanName (char c, Parser& parser, bool& cont)
    {
        static const char *keywords[] =
        {
            "begin", "end",
            "short", "long", "float",
            "if", "endif", "else", "elseif",
            "while", "endwhile",
            "return",
            "messagebox",
            "set", "to",
            0
        };

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

        std::string lowerCase;
        lowerCase.reserve (name.size());
        
        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        for (; keywords[i]; ++i)
            if (lowerCase==keywords[i])
                break;

        cont =
            keywords[i] ? parser.parseKeyword (i, loc, *this) : parser.parseName (name, loc, *this);

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
                else if (c=='\\')
                {
                    if (!get (c))
                    {
                        mErrorHandler.error ("incomplete escape sequence", mLoc);
                        break;
                    }
                }
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
        else if (c=='=')
        {
            if (get (c))
            {
                if (c=='=')
                    special = S_cmpEQ;
                else
                {
                    putback (c);
                    return false;
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

    Scanner::Scanner (ErrorHandler& errorHandler, std::istream& inputStream)
    : mErrorHandler (errorHandler), mStream (inputStream)
    {
    }

    void Scanner::scan (Parser& parser)
    {
        while (scanToken (parser));
    }
}

