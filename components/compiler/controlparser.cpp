#include "controlparser.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>

#include "scanner.hpp"
#include "generator.hpp"
#include "errorhandler.hpp"
#include "skipparser.hpp"

namespace Compiler
{
    bool ControlParser::parseIfBody (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        if (keyword==Scanner::K_endif || keyword==Scanner::K_elseif ||
            keyword==Scanner::K_else)
        {
            std::pair<Codes, Codes> entry;

            if (mState!=IfElseBodyState)
                mExprParser.append (entry.first);

            std::copy (mCodeBlock.begin(), mCodeBlock.end(),
                std::back_inserter (entry.second));

            mIfCode.push_back (entry);

            mCodeBlock.clear();

            if (keyword==Scanner::K_endif)
            {
                // store code for if-cascade
                Codes codes;

                for (IfCodes::reverse_iterator iter (mIfCode.rbegin());
                    iter!=mIfCode.rend(); ++iter)
                {
                    Codes block;

                    if (iter!=mIfCode.rbegin())
                        Generator::jump (iter->second, codes.size()+1);

                    if (!iter->first.empty())
                    {
                        // if or elseif
                        std::copy (iter->first.begin(), iter->first.end(),
                            std::back_inserter (block));
                        Generator::jumpOnZero (block, iter->second.size()+1);
                    }

                    std::copy (iter->second.begin(), iter->second.end(),
                        std::back_inserter (block));

                   std::swap (codes, block);

                   std::copy (block.begin(), block.end(), std::back_inserter (codes));
                }

                std::copy (codes.begin(), codes.end(), std::back_inserter (mCode));

                mIfCode.clear();
                mState = IfEndifState;
            }
            else if (keyword==Scanner::K_elseif)
            {
                mExprParser.reset();
                scanner.scan (mExprParser);

                mState = IfElseifEndState;
            }
            else if (keyword==Scanner::K_else)
            {
                mState = IfElseJunkState; /// \todo should be IfElseEndState; add an option for that
            }

            return true;
        }
        else if (keyword==Scanner::K_if || keyword==Scanner::K_while)
        {
            // nested
            ControlParser parser (getErrorHandler(), getContext(), mLocals, mLiterals);

            if (parser.parseKeyword (keyword, loc, scanner))
                scanner.scan (parser);

            parser.appendCode (mCodeBlock);

            return true;
        }
        else
        {
            mLineParser.reset();
            if (mLineParser.parseKeyword (keyword, loc, scanner))
                scanner.scan (mLineParser);

            return true;
        }
    }

    bool ControlParser::parseWhileBody (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        if (keyword==Scanner::K_endwhile)
        {
            Codes loop;

            Codes expr;
            mExprParser.append (expr);

            Generator::jump (loop, -static_cast<int> (mCodeBlock.size()+expr.size()));

            std::copy (expr.begin(), expr.end(), std::back_inserter (mCode));

            Codes skip;

            Generator::jumpOnZero (skip, mCodeBlock.size()+loop.size()+1);

            std::copy (skip.begin(), skip.end(), std::back_inserter (mCode));

            std::copy (mCodeBlock.begin(), mCodeBlock.end(), std::back_inserter (mCode));

            Codes loop2;

            Generator::jump (loop2, -static_cast<int> (mCodeBlock.size()+expr.size()+skip.size()));

            if (loop.size()!=loop2.size())
                throw std::logic_error (
                    "Internal compiler error: failed to generate a while loop");

            std::copy (loop2.begin(), loop2.end(), std::back_inserter (mCode));

            mState = WhileEndwhileState;
            return true;
        }
        else if (keyword==Scanner::K_if || keyword==Scanner::K_while)
        {
            // nested
            ControlParser parser (getErrorHandler(), getContext(), mLocals, mLiterals);

            if (parser.parseKeyword (keyword, loc, scanner))
                scanner.scan (parser);

            parser.appendCode (mCodeBlock);

            return true;
        }
        else
        {
            mLineParser.reset();
            if (mLineParser.parseKeyword (keyword, loc, scanner))
                scanner.scan (mLineParser);

            return true;
        }
    }

    ControlParser::ControlParser (ErrorHandler& errorHandler, const Context& context, Locals& locals,
        Literals& literals)
    : Parser (errorHandler, context), mLocals (locals), mLiterals (literals),
      mLineParser (errorHandler, context, locals, literals, mCodeBlock),
      mExprParser (errorHandler, context, locals, literals),
      mState (StartState)
    {

    }

    void ControlParser::appendCode (std::vector<Interpreter::Type_Code>& code) const
    {
        std::copy (mCode.begin(), mCode.end(), std::back_inserter (code));
    }

    bool ControlParser::parseName (const std::string& name, const TokenLoc& loc, Scanner& scanner)
    {
        if (mState==IfBodyState || mState==IfElseifBodyState || mState==IfElseBodyState ||
            mState==WhileBodyState)
        {
            scanner.putbackName (name, loc);
            mLineParser.reset();
            scanner.scan (mLineParser);
            return true;
        }
        else if (mState==IfElseJunkState)
        {
            getErrorHandler().warning ("Extra text after else", loc);
            SkipParser skip (getErrorHandler(), getContext());
            scanner.scan (skip);
            mState = IfElseBodyState;
            return true;
        }

        return Parser::parseName (name, loc, scanner);
    }

    bool ControlParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        if (mState==StartState)
        {
            if (keyword==Scanner::K_if || keyword==Scanner::K_elseif)
            {
                if (keyword==Scanner::K_elseif)
                    getErrorHandler().warning ("elseif without matching if", loc);

                mExprParser.reset();
                scanner.scan (mExprParser);

                mState = IfEndState;
                return true;
            }
            else if (keyword==Scanner::K_while)
            {
                mExprParser.reset();
                scanner.scan (mExprParser);

                mState = WhileEndState;
                return true;
            }
        }
        else if (mState==IfBodyState || mState==IfElseifBodyState || mState==IfElseBodyState)
        {
            if (parseIfBody (keyword, loc, scanner))
                return true;
        }
        else if (mState==WhileBodyState)
        {
            if ( parseWhileBody (keyword, loc, scanner))
                return true;
        }
        else if (mState==IfElseJunkState)
        {
            getErrorHandler().warning ("Extra text after else", loc);
            SkipParser skip (getErrorHandler(), getContext());
            scanner.scan (skip);
            mState = IfElseBodyState;
            return true;
        }

        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool ControlParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline)
        {
            switch (mState)
            {
                case IfEndState: mState = IfBodyState; return true;
                case IfElseifEndState: mState = IfElseifBodyState; return true;
                case IfElseEndState: mState = IfElseBodyState; return true;
                case IfElseJunkState: mState = IfElseBodyState; return true;

                case WhileEndState: mState = WhileBodyState; return true;

                case IfBodyState:
                case IfElseifBodyState:
                case IfElseBodyState:
                case WhileBodyState:

                    return true; // empty line

                case IfEndifState:
                case WhileEndwhileState:

                    return false;

                default: ;
            }
        }
        else if (mState==IfElseJunkState)
        {
            getErrorHandler().warning ("Extra text after else", loc);
            SkipParser skip (getErrorHandler(), getContext());
            scanner.scan (skip);
            mState = IfElseBodyState;
            return true;
        }

        return Parser::parseSpecial (code, loc, scanner);
    }

    void ControlParser::reset()
    {
        mCode.clear();
        mCodeBlock.clear();
        mIfCode.clear();
        mState = StartState;
        Parser::reset();
    }
}
