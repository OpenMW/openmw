#ifndef COMPILER_SCANNER_H_INCLUDED
#define COMPILER_SCANNER_H_INCLUDED

#include <string>
#include <iosfwd>

#include "tokenloc.hpp"

namespace Compiler
{
    class ErrorHandler;
    class Parser;

    /// \brief Scanner
    ///
    /// This class translate a char-stream to a token stream (delivered via
    /// parser-callbacks).

    class Scanner
    {
            ErrorHandler& mErrorHandler;
            TokenLoc mLoc;
            TokenLoc mPrevLoc;
            std::istream& mStream;

        public:

            enum keyword
            {
                K_begin, K_end,
                K_short, K_long, K_float,
                K_if, K_endif, K_else, K_elseif,
                K_while, K_endwhile,
                K_return,
                K_messageBox,
                K_set, K_to
            };

            enum special
            {
                S_newline,
                S_open, S_close,
                S_cmpEQ, S_cmpNE, S_cmpLT, S_cmpLE, S_cmpGT, S_cmpGE,
                S_plus, S_minus, S_mult, S_div,
                S_comma,
                S_ref
            };

        private:

        // not implemented

            Scanner (const Scanner&);
            Scanner& operator= (const Scanner&);

            bool get (char& c);

            void putback (char c);

            bool scanToken (Parser& parser);

            bool scanInt (char c, Parser& parser, bool& cont);

            bool scanFloat (const std::string& intValue, Parser& parser, bool& cont);

            bool scanName (char c, Parser& parser, bool& cont);

            bool scanName (char c, std::string& name);

            bool scanSpecial (char c, Parser& parser, bool& cont);

            static bool isWhitespace (char c);

        public:

            Scanner (ErrorHandler& errorHandler, std::istream& inputStream);
		    ///< constructor

            void scan (Parser& parser);
            ///< Scan a token and deliver it to the parser.
    };
}

#endif
