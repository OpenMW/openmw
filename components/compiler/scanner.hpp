#ifndef COMPILER_SCANNER_H_INCLUDED
#define COMPILER_SCANNER_H_INCLUDED

#include <cctype>
#include <string>
#include <iosfwd>
#include <vector>
#include <sstream>

#include "tokenloc.hpp"

namespace Compiler
{
    class ErrorHandler;
    class Parser;
    class Extensions;

    /// \brief Scanner
    ///
    /// This class translate a char-stream to a token stream (delivered via
    /// parser-callbacks).

    class MultiChar
    {
    public:
        MultiChar()
        {
            blank();
        }

        MultiChar(const char ch)
        {
            blank();
            mData[0] = ch;

            mLength = getCharLength(ch);
        }

        int getCharLength(const char ch)
        {
            unsigned char c = ch;
            if (c<=127) return 0;
            else if ((c & 0xE0) == 0xC0) return 1;
            else if ((c & 0xF0) == 0xE0) return 2;
            else if ((c & 0xF8) == 0xF0) return 3;
            else return -1;
        }

        bool operator== (const char ch)
        {
            return mData[0]==ch && mData[1]==0 && mData[2]==0 && mData[3]==0;
        }

        bool operator== (const MultiChar& ch)
        {
            return mData[0]==ch.mData[0] && mData[1]==ch.mData[1] && mData[2]==ch.mData[2] && mData[3]==ch.mData[3];
        }

        bool operator!= (const char ch)
        {
            return mData[0]!=ch || mData[1]!=0 || mData[2]!=0 || mData[3]!=0;
        }

        bool isWhitespace()
        {
            return (mData[0]==' ' || mData[0]=='\t') && mData[1]==0 && mData[2]==0 && mData[3]==0;
        }

        bool isDigit()
        {
            return std::isdigit(mData[0]) && mData[1]==0 && mData[2]==0 && mData[3]==0;
        }

        bool isMinusSign()
        {
            if (mData[0] == '-' && mData[1] == 0 && mData[2] == 0 && mData[3] == 0)
                return true;

            return mData[0] == '\xe2' && mData[1] == '\x80' && mData[2] == '\x93' && mData[3] == 0;
        }

        bool isAlpha()
        {
            if (isMinusSign())
                return false;

            return std::isalpha(mData[0]) || mData[1]!=0 || mData[2]!=0 || mData[3]!=0;
        }

        void appendTo(std::string& str)
        {
            for (int i = 0; i <= mLength; i++)
                str += mData[i];
        }

        void putback (std::istream& in)
        {
            for (int i = mLength; i >= 0; i--)
                in.putback (mData[i]);
        }

        bool getFrom(std::istream& in)
        {
            blank();

            char ch = in.peek();

            if (!in.good())
                return false;

            int length = getCharLength(ch);
            if (length < 0) return false;

            for (int i = 0; i <= length; i++)
            {
                in.get (ch);

                if (!in.good())
                    return false;

                mData[i] = ch;
            }

            mLength = length;

            return true;
        }

        bool peek(std::istream& in)
        {
            std::streampos p_orig = in.tellg();

            char ch = in.peek();

            if (!in.good())
                return false;

            int length = getCharLength(ch);
            if (length < 0) return false;

            for (int i = 0; i <= length; i++)
            {
                if (length >= i)
                {
                    in.get (ch);

                    if (!in.good())
                        return false;

                    mData[i] = ch;
                }
            }

            mLength = length;

            in.seekg(p_orig);
            return true;
        };

        void blank()
        {
            std::fill(mData, mData + sizeof(mData), 0);
            mLength = -1;
        }

        std::string data()
        {
            // NB: mLength is the number of the last element in the array
            return std::string(mData, mLength + 1);
        }

    private:
        char mData[4];
        int mLength;
    };

    class Scanner
    {
            enum putback_type
            {
                Putback_None, Putback_Special, Putback_Integer, Putback_Float,
                Putback_Name, Putback_Keyword
            };

            ErrorHandler& mErrorHandler;
            TokenLoc mLoc;
            TokenLoc mPrevLoc;
            std::istream& mStream;
            const Extensions *mExtensions;
            putback_type mPutback;
            int mPutbackCode;
            int mPutbackInteger;
            float mPutbackFloat;
            std::string mPutbackName;
            TokenLoc mPutbackLoc;
            bool mStrictKeywords;
            bool mTolerantNames;
            bool mIgnoreNewline;

        public:

            enum keyword
            {
                K_begin, K_end,
                K_short, K_long, K_float,
                K_if, K_endif, K_else, K_elseif,
                K_while, K_endwhile,
                K_return,
                K_messagebox,
                K_set, K_to,
                K_getsquareroot,
                K_menumode,
                K_random,
                K_startscript, K_stopscript, K_scriptrunning,
                K_getdistance,
                K_getsecondspassed,
                K_enable, K_disable, K_getdisabled
            };

            enum special
            {
                S_newline,
                S_open, S_close,
                S_cmpEQ, S_cmpNE, S_cmpLT, S_cmpLE, S_cmpGT, S_cmpGE,
                S_plus, S_minus, S_mult, S_div,
                S_comma,
                S_ref,
                S_member
            };

        private:

        // not implemented

            Scanner (const Scanner&);
            Scanner& operator= (const Scanner&);

            bool get (MultiChar& c);

            void putback (MultiChar& c);

            bool scanToken (Parser& parser);

            bool scanInt (MultiChar& c, Parser& parser, bool& cont);

            bool scanFloat (const std::string& intValue, Parser& parser, bool& cont);

            bool scanName (MultiChar& c, Parser& parser, bool& cont);

            /// \param name May contain the start of the name (one or more characters)
            bool scanName (std::string& name);

            bool scanSpecial (MultiChar& c, Parser& parser, bool& cont);

            bool isStringCharacter (MultiChar& c, bool lookAhead = true);

        public:

            Scanner (ErrorHandler& errorHandler, std::istream& inputStream,
                const Extensions *extensions = 0);
            ///< constructor

            void scan (Parser& parser);
            ///< Scan a token and deliver it to the parser.

            void putbackSpecial (int code, const TokenLoc& loc);
            ///< put back a special token

            void putbackInt (int value, const TokenLoc& loc);
            ///< put back an integer token

            void putbackFloat (float value, const TokenLoc& loc);
            ///< put back a float token

            void putbackName (const std::string& name, const TokenLoc& loc);
            ///< put back a name token

            void putbackKeyword (int keyword, const TokenLoc& loc);
            ///< put back a keyword token

            void listKeywords (std::vector<std::string>& keywords);
            ///< Append all known keywords to \a keywords.

            /// Treat newline character as a part of script command.
            ///
            /// \attention This mode lasts only until the next keyword is reached.
            void enableIgnoreNewlines();

            /// Do not accept keywords in quotation marks anymore.
            ///
            /// \attention This mode lasts only until the next newline is reached.
            void enableStrictKeywords();

            /// Continue parsing a name when hitting a '.' or a '-'
            ///
            /// \attention This mode lasts only until the next newline is reached.
            void enableTolerantNames();
    };
}

#endif
