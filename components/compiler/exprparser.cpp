
#include "exprparser.hpp"

#include <stdexcept>
#include <cassert>
#include <algorithm>
#include <stack>
#include <iterator>

#include "generator.hpp"
#include "scanner.hpp"
#include "errorhandler.hpp"
#include "locals.hpp"
#include "stringparser.hpp"
#include "extensions.hpp"
#include "context.hpp"
#include <components/misc/stringops.hpp>

namespace Compiler
{
    int ExprParser::getPriority (char op) const
    {
        switch (op)
        {
            case '(':

                return 0;

            case 'e': // ==
            case 'n': // !=
            case 'l': // <
            case 'L': // <=
            case 'g': // <
            case 'G': // >=

                return 1;

            case '+':
            case '-':

                return 2;

            case '*':
            case '/':

                return 3;

            case 'm':

                return 4;
        }

        return 0;
    }

    char ExprParser::getOperandType (int Index) const
    {
        assert (!mOperands.empty());
        assert (Index>=0);
        assert (Index<static_cast<int> (mOperands.size()));
        return mOperands[mOperands.size()-1-Index];
    }

    char ExprParser::getOperator() const
    {
        assert (!mOperators.empty());
        return mOperators[mOperators.size()-1];
    }

    bool ExprParser::isOpen() const
    {
        return std::find (mOperators.begin(), mOperators.end(), '(')!=mOperators.end();
    }

    void ExprParser::popOperator()
    {
        assert (!mOperators.empty());
        mOperators.resize (mOperators.size()-1);
    }

    void ExprParser::popOperand()
    {
        assert (!mOperands.empty());
        mOperands.resize (mOperands.size()-1);
    }

    void ExprParser::replaceBinaryOperands()
    {
        char t1 = getOperandType (1);
        char t2 = getOperandType();

        popOperand();
        popOperand();

        if (t1==t2)
            mOperands.push_back (t1);
        else if (t1=='f' || t2=='f')
            mOperands.push_back ('f');
        else
            std::logic_error ("failed to determine result operand type");
    }

    void ExprParser::pop()
    {
        char op = getOperator();

        switch (op)
        {
            case 'm':

                Generator::negate (mCode, getOperandType());
                popOperator();
                break;

            case '+':

                Generator::add (mCode, getOperandType (1), getOperandType());
                popOperator();
                replaceBinaryOperands();
                break;

            case '-':

                Generator::sub (mCode, getOperandType (1), getOperandType());
                popOperator();
                replaceBinaryOperands();
                break;

            case '*':

                Generator::mul (mCode, getOperandType (1), getOperandType());
                popOperator();
                replaceBinaryOperands();
                break;

            case '/':

                Generator::div (mCode, getOperandType (1), getOperandType());
                popOperator();
                replaceBinaryOperands();
                break;

            case 'e':
            case 'n':
            case 'l':
            case 'L':
            case 'g':
            case 'G':

                Generator::compare (mCode, op, getOperandType (1), getOperandType());
                popOperator();
                popOperand();
                popOperand();
                mOperands.push_back ('l');
                break;

            default:

                throw std::logic_error ("unknown operator");
        }
    }

    void ExprParser::pushIntegerLiteral (int value)
    {
        mNextOperand = false;
        mOperands.push_back ('l');
        Generator::pushInt (mCode, mLiterals, value);
    }

    void ExprParser::pushFloatLiteral (float value)
    {
        mNextOperand = false;
        mOperands.push_back ('f');
        Generator::pushFloat (mCode, mLiterals, value);
    }

    void ExprParser::pushBinaryOperator (char c)
    {
        while (!mOperators.empty() && getPriority (getOperator())>=getPriority (c))
            pop();

        mOperators.push_back (c);
        mNextOperand = true;
    }

    void ExprParser::close()
    {
        while (getOperator()!='(')
            pop();

        popOperator();
    }

    int ExprParser::parseArguments (const std::string& arguments, Scanner& scanner)
    {
        return parseArguments (arguments, scanner, mCode);
    }

    bool ExprParser::handleMemberAccess (const std::string& name)
    {
        mMemberOp = false;

        std::string name2 = Misc::StringUtils::lowerCase (name);
        std::string id = Misc::StringUtils::lowerCase (mExplicit);

        char type = getContext().getMemberType (name2, id);

        if (type!=' ')
        {
            Generator::fetchMember (mCode, mLiterals, type, name2, id);
            mNextOperand = false;
            mExplicit.clear();
            mOperands.push_back (type=='f' ? 'f' : 'l');
            return true;
        }

        return false;
    }

    ExprParser::ExprParser (ErrorHandler& errorHandler, Context& context, Locals& locals,
        Literals& literals, bool argument)
    : Parser (errorHandler, context), mLocals (locals), mLiterals (literals),
      mNextOperand (true), mFirst (true), mArgument (argument), mRefOp (false), mMemberOp (false)
    {}

    bool ExprParser::parseInt (int value, const TokenLoc& loc, Scanner& scanner)
    {
        if (!mExplicit.empty())
            return Parser::parseInt (value, loc, scanner);

        mFirst = false;

        if (mNextOperand)
        {
            start();

            pushIntegerLiteral (value);
            mTokenLoc = loc;
            return true;
        }
        else
        {
            // no comma was used between arguments
            scanner.putbackInt (value, loc);
            return false;
        }
    }

    bool ExprParser::parseFloat (float value, const TokenLoc& loc, Scanner& scanner)
    {
        if (!mExplicit.empty())
            return Parser::parseFloat (value, loc, scanner);

        mFirst = false;

        if (mNextOperand)
        {
            start();

            pushFloatLiteral (value);
            mTokenLoc = loc;
            return true;
        }
        else
        {
            // no comma was used between arguments
            scanner.putbackFloat (value, loc);
            return false;
        }
    }

    bool ExprParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        if (!mExplicit.empty())
        {
            if (mMemberOp && handleMemberAccess (name))
                return true;

            return Parser::parseName (name, loc, scanner);
        }

        mFirst = false;

        if (mNextOperand)
        {
            start();

            std::string name2 = Misc::StringUtils::lowerCase (name);

            char type = mLocals.getType (name2);

            if (type!=' ')
            {
                Generator::fetchLocal (mCode, type, mLocals.getIndex (name2));
                mNextOperand = false;
                mOperands.push_back (type=='f' ? 'f' : 'l');
                return true;
            }

            type = getContext().getGlobalType (name2);

            if (type!=' ')
            {
                Generator::fetchGlobal (mCode, mLiterals, type, name2);
                mNextOperand = false;
                mOperands.push_back (type=='f' ? 'f' : 'l');
                return true;
            }

            if (mExplicit.empty() && getContext().isId (name2))
            {
                mExplicit = name2;
                return true;
            }
        }
        else
        {
            // no comma was used between arguments
            scanner.putbackName (name, loc);
            return false;
        }

        return Parser::parseName (name, loc, scanner);
    }

    bool ExprParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        mFirst = false;

        if (!mExplicit.empty())
        {
            if (mRefOp && mNextOperand)
            {
                if (keyword==Scanner::K_getdisabled)
                {
                    start();

                    mTokenLoc = loc;

                    Generator::getDisabled (mCode, mLiterals, mExplicit);
                    mOperands.push_back ('l');
                    mExplicit.clear();
                    mRefOp = false;

                    mNextOperand = false;
                    return true;
                }
                else if (keyword==Scanner::K_getdistance)
                {
                    start();

                    mTokenLoc = loc;
                    parseArguments ("c", scanner);

                    Generator::getDistance (mCode, mLiterals, mExplicit);
                    mOperands.push_back ('f');
                    mExplicit.clear();
                    mRefOp = false;

                    mNextOperand = false;
                    return true;
                }

                // check for custom extensions
                if (const Extensions *extensions = getContext().getExtensions())
                {
                    char returnType;
                    std::string argumentType;

                    if (extensions->isFunction (keyword, returnType, argumentType, true))
                    {
                        start();

                        mTokenLoc = loc;
                        int optionals = parseArguments (argumentType, scanner);

                        extensions->generateFunctionCode (keyword, mCode, mLiterals, mExplicit,
                            optionals);
                        mOperands.push_back (returnType);
                        mExplicit.clear();
                        mRefOp = false;

                        mNextOperand = false;
                        return true;
                    }
                }
            }

            return Parser::parseKeyword (keyword, loc, scanner);
        }

        if (mNextOperand)
        {
            if (keyword==Scanner::K_getsquareroot)
            {
                start();

                mTokenLoc = loc;
                parseArguments ("f", scanner);

                Generator::squareRoot (mCode);
                mOperands.push_back ('f');

                mNextOperand = false;
                return true;
            }
            else if (keyword==Scanner::K_menumode)
            {
                start();

                mTokenLoc = loc;

                Generator::menuMode (mCode);
                mOperands.push_back ('l');

                mNextOperand = false;
                return true;
            }
            else if (keyword==Scanner::K_random)
            {
                start();

                mTokenLoc = loc;
                parseArguments ("l", scanner);

                Generator::random (mCode);
                mOperands.push_back ('l');

                mNextOperand = false;
                return true;
            }
            else if (keyword==Scanner::K_scriptrunning)
            {
                start();

                mTokenLoc = loc;
                parseArguments ("c", scanner);

                Generator::scriptRunning (mCode);
                mOperands.push_back ('l');

                mNextOperand = false;
                return true;
            }
            else if (keyword==Scanner::K_getdistance)
            {
                start();

                mTokenLoc = loc;
                parseArguments ("c", scanner);

                Generator::getDistance (mCode, mLiterals, "");
                mOperands.push_back ('f');

                mNextOperand = false;
                return true;
            }
            else if (keyword==Scanner::K_getsecondspassed)
            {
                start();

                mTokenLoc = loc;

                Generator::getSecondsPassed (mCode);
                mOperands.push_back ('f');

                mNextOperand = false;
                return true;
            }
            else if (keyword==Scanner::K_getdisabled)
            {
                start();

                mTokenLoc = loc;

                Generator::getDisabled (mCode, mLiterals, "");
                mOperands.push_back ('l');

                mNextOperand = false;
                return true;
            }
            else
            {
                // check for custom extensions
                if (const Extensions *extensions = getContext().getExtensions())
                {
                    start();

                    char returnType;
                    std::string argumentType;

                    if (extensions->isFunction (keyword, returnType, argumentType, false))
                    {
                        mTokenLoc = loc;
                        int optionals = parseArguments (argumentType, scanner);

                        extensions->generateFunctionCode (keyword, mCode, mLiterals, "", optionals);
                        mOperands.push_back (returnType);

                        mNextOperand = false;
                        return true;
                    }
                }
            }
        }
        else
        {
            // no comma was used between arguments
            scanner.putbackKeyword (keyword, loc);
            return false;
        }

        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool ExprParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (!mExplicit.empty())
        {
            if (!mRefOp && code==Scanner::S_ref)
            {
                mRefOp = true;
                return true;
            }

            if (!mMemberOp && code==Scanner::S_member)
            {
                mMemberOp = true;
                return true;
            }

            return Parser::parseSpecial (code, loc, scanner);
        }

        if (code==Scanner::S_comma)
        {
            mTokenLoc = loc;

            if (mFirst)
            {
                // leading comma
                mFirst = false;
                return true;
            }

            // end marker
            scanner.putbackSpecial (code, loc);
            return false;
        }

        mFirst = false;

        if (code==Scanner::S_newline)
        {
            // end marker
            mTokenLoc = loc;
            scanner.putbackSpecial (code, loc);
            return false;
        }

        if (code==Scanner::S_minus && mNextOperand)
        {
            // unary
            mOperators.push_back ('m');
            mTokenLoc = loc;
            return true;
        }

        if (code==Scanner::S_open)
        {
            if (mNextOperand)
            {
                mOperators.push_back ('(');
                mTokenLoc = loc;
                return true;
            }
            else
            {
                // no comma was used between arguments
                scanner.putbackKeyword (code, loc);
                return false;
            }
        }

        if (code==Scanner::S_close && !mNextOperand)
        {
            if (isOpen())
            {
                close();
                return true;
            }

            mTokenLoc = loc;
            scanner.putbackSpecial (code, loc);
            return false;
        }

        if (!mNextOperand)
        {
            mTokenLoc = loc;
            char c = 0; // comparison

            switch (code)
            {
                case Scanner::S_plus: c = '+'; break;
                case Scanner::S_minus: c = '-'; break;
                case Scanner::S_mult: pushBinaryOperator ('*'); return true;
                case Scanner::S_div: pushBinaryOperator ('/'); return true;
                case Scanner::S_cmpEQ: c = 'e'; break;
                case Scanner::S_cmpNE: c = 'n'; break;
                case Scanner::S_cmpLT: c = 'l'; break;
                case Scanner::S_cmpLE: c = 'L'; break;
                case Scanner::S_cmpGT: c = 'g'; break;
                case Scanner::S_cmpGE: c = 'G'; break;
            }

            if (c)
            {
                if (mArgument && !isOpen())
                {
                    // expression ends here
                    // Thank you Morrowind for this rotten syntax :(
                    scanner.putbackSpecial (code, loc);
                    return false;
                }

                pushBinaryOperator (c);
                return true;
            }
        }

        return Parser::parseSpecial (code, loc, scanner);
    }

    void ExprParser::reset()
    {
        mOperands.clear();
        mOperators.clear();
        mNextOperand = true;
        mCode.clear();
        mFirst = true;
        mExplicit.clear();
        mRefOp = false;
        mMemberOp = false;
        Parser::reset();
    }

    char ExprParser::append (std::vector<Interpreter::Type_Code>& code)
    {
        if (mOperands.empty() && mOperators.empty())
        {
            getErrorHandler().error ("missing expression", mTokenLoc);
            return 'l';
        }

        if (mNextOperand || mOperands.empty())
        {
            getErrorHandler().error ("syntax error in expression", mTokenLoc);
            return 'l';
        }

        while (!mOperators.empty())
            pop();

        std::copy (mCode.begin(), mCode.end(), std::back_inserter (code));

        assert (mOperands.size()==1);
        return mOperands[0];
    }

    int ExprParser::parseArguments (const std::string& arguments, Scanner& scanner,
        std::vector<Interpreter::Type_Code>& code, bool invert)
    {
        bool optional = false;
        int optionalCount = 0;

        ExprParser parser (getErrorHandler(), getContext(), mLocals, mLiterals, true);
        StringParser stringParser (getErrorHandler(), getContext(), mLiterals);

        std::stack<std::vector<Interpreter::Type_Code> > stack;

        for (std::string::const_iterator iter (arguments.begin()); iter!=arguments.end();
            ++iter)
        {
            if (*iter=='/')
            {
                optional = true;
            }
            else if (*iter=='S' || *iter=='c')
            {
                stringParser.reset();

                if (optional)
                    stringParser.setOptional (true);

                if (*iter=='c') stringParser.smashCase();
                scanner.scan (stringParser);

                if (optional && stringParser.isEmpty())
                    break;

                if (invert)
                {
                    std::vector<Interpreter::Type_Code> tmp;
                    stringParser.append (tmp);

                    stack.push (tmp);
                }
                else
                    stringParser.append (code);

                if (optional)
                    ++optionalCount;
            }
            else
            {
                parser.reset();

                if (optional)
                    parser.setOptional (true);

                scanner.scan (parser);

                if (optional && parser.isEmpty())
                    break;

                std::vector<Interpreter::Type_Code> tmp;

                char type = parser.append (tmp);

                if (type!=*iter)
                    Generator::convert (tmp, type, *iter);

                if (invert)
                    stack.push (tmp);
                else
                    std::copy (tmp.begin(), tmp.end(), std::back_inserter (code));

                if (optional)
                    ++optionalCount;
            }
        }

        while (!stack.empty())
        {
            std::vector<Interpreter::Type_Code>& tmp = stack.top();

            std::copy (tmp.begin(), tmp.end(), std::back_inserter (code));

            stack.pop();
        }

        return optionalCount;
    }
}
