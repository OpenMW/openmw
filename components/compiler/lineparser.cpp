
#include "lineparser.hpp"

#include <components/misc/stringops.hpp>

#include "scanner.hpp"
#include "context.hpp"
#include "errorhandler.hpp"
#include "skipparser.hpp"
#include "locals.hpp"
#include "generator.hpp"
#include "extensions.hpp"
#include "declarationparser.hpp"

namespace Compiler
{
    void LineParser::parseExpression (Scanner& scanner, const TokenLoc& loc)
    {
        mExprParser.reset();

        if (!mExplicit.empty())
        {
            mExprParser.parseName (mExplicit, loc, scanner);
            if (mState==MemberState)
                mExprParser.parseSpecial (Scanner::S_member, loc, scanner);
            else
                mExprParser.parseSpecial (Scanner::S_ref, loc, scanner);
        }

        scanner.scan (mExprParser);

        char type = mExprParser.append (mCode);
        mState = EndState;

        switch (type)
        {
            case 'l':

                Generator::report (mCode, mLiterals, "%g");
                break;

            case 'f':

                Generator::report (mCode, mLiterals, "%f");
                break;

            default:

                throw std::runtime_error ("unknown expression result type");
        }
    }

    LineParser::LineParser (ErrorHandler& errorHandler, const Context& context, Locals& locals,
        Literals& literals, std::vector<Interpreter::Type_Code>& code, bool allowExpression)
    : Parser (errorHandler, context), mLocals (locals), mLiterals (literals), mCode (code),
       mState (BeginState), mExprParser (errorHandler, context, locals, literals),
       mAllowExpression (allowExpression), mButtons(0), mType(0)
    {}

    bool LineParser::parseInt (int value, const TokenLoc& loc, Scanner& scanner)
    {
        if (mAllowExpression && mState==BeginState)
        {
            scanner.putbackInt (value, loc);
            parseExpression (scanner, loc);
            return true;
        }

        return Parser::parseInt (value, loc, scanner);
    }

    bool LineParser::parseFloat (float value, const TokenLoc& loc, Scanner& scanner)
    {
        if (mAllowExpression && mState==BeginState)
        {
            scanner.putbackFloat (value, loc);
            parseExpression (scanner, loc);
            return true;
        }

        return Parser::parseFloat (value, loc, scanner);
    }

    bool LineParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        if (mState==PotentialEndState)
        {
            getErrorHandler().warning ("stay string argument (ignoring it)", loc);
            mState = EndState;
            return true;
        }

        if (mState==SetState)
        {
            std::string name2 = Misc::StringUtils::lowerCase (name);
            mName = name2;

            // local variable?
            char type = mLocals.getType (name2);
            if (type!=' ')
            {
                mType = type;
                mState = SetLocalVarState;
                return true;
            }

            type = getContext().getGlobalType (name2);
            if (type!=' ')
            {
                mType = type;
                mState = SetGlobalVarState;
                return true;
            }

            mState = SetPotentialMemberVarState;
            return true;
        }

        if (mState==SetMemberVarState)
        {
            mMemberName = name;
            std::pair<char, bool> type = getContext().getMemberType (mMemberName, mName);

            if (type.first!=' ')
            {
                mState = SetMemberVarState2;
                mType = type.first;
                mReferenceMember = type.second;
                return true;
            }

            getErrorHandler().error ("unknown variable", loc);
            SkipParser skip (getErrorHandler(), getContext());
            scanner.scan (skip);
            return false;
        }

        if (mState==MessageState || mState==MessageCommaState)
        {
            std::string arguments;

            for (std::size_t i=0; i<name.size(); ++i)
            {
                if (name[i]=='%')
                {
                    ++i;
                    if (i<name.size())
                    {
                        if (name[i]=='G' || name[i]=='g')
                        {
                            arguments += "l";
                        }
                        else if (name[i]=='S' || name[i]=='s')
                        {
                            arguments += 'S';
                        }
                        else if (name[i]=='.' || name[i]=='f')
                        {
                            arguments += 'f';
                        }
                    }
                }
            }

            if (!arguments.empty())
            {
                mExprParser.reset();
                mExprParser.parseArguments (arguments, scanner, mCode, true);
            }

            mName = name;
            mButtons = 0;

            mState = MessageButtonState;
            return true;
        }

        if (mState==MessageButtonState || mState==MessageButtonCommaState)
        {
            Generator::pushString (mCode, mLiterals, name);
            mState = MessageButtonState;
            ++mButtons;
            return true;
        }

        if (mState==BeginState && getContext().isId (name))
        {
            mState = PotentialExplicitState;
            mExplicit = Misc::StringUtils::lowerCase (name);
            return true;
        }

        if (mState==BeginState && mAllowExpression)
        {
            std::string name2 = Misc::StringUtils::lowerCase (name);

            char type = mLocals.getType (name2);

            if (type!=' ')
            {
                scanner.putbackName (name, loc);
                parseExpression (scanner, loc);
                return true;
            }

            type = getContext().getGlobalType (name2);

            if (type!=' ')
            {
                scanner.putbackName (name, loc);
                parseExpression (scanner, loc);
                return true;
            }
        }

        return Parser::parseName (name, loc, scanner);
    }

    bool LineParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        if (mState==SetMemberVarState)
        {
            mMemberName = loc.mLiteral;
            std::pair<char, bool> type = getContext().getMemberType (mMemberName, mName);

            if (type.first!=' ')
            {
                mState = SetMemberVarState2;
                mType = type.first;
                mReferenceMember = type.second;
                return true;
            }
        }

        if (mState==SetPotentialMemberVarState && keyword==Scanner::K_to)
        {
            getErrorHandler().warning ("unknown variable (ignoring set instruction)", loc);
            SkipParser skip (getErrorHandler(), getContext());
            scanner.scan (skip);
            return false;
        }

        if (mState==SetState)
        {
            // allow keywords to be used as variable names when assigning a value to a variable.
            return parseName (loc.mLiteral, loc, scanner);
        }

        if (mState==BeginState || mState==ExplicitState)
        {
            switch (keyword)
            {
                case Scanner::K_enable:

                    Generator::enable (mCode, mLiterals, mExplicit);
                    mState = PotentialEndState;
                    return true;

                case Scanner::K_disable:

                    Generator::disable (mCode, mLiterals, mExplicit);
                    mState = PotentialEndState;
                    return true;
            }

            // check for custom extensions
            if (const Extensions *extensions = getContext().getExtensions())
            {
                std::string argumentType;

                bool hasExplicit = mState==ExplicitState;
                if (extensions->isInstruction (keyword, argumentType, hasExplicit))
                {
                    if (!hasExplicit && mState==ExplicitState)
                    {
                        getErrorHandler().warning ("stray explicit reference (ignoring it)", loc);
                        mExplicit.clear();
                    }

                    int optionals = mExprParser.parseArguments (argumentType, scanner, mCode, true);

                    extensions->generateInstructionCode (keyword, mCode, mLiterals, mExplicit, optionals);
                    mState = EndState;
                    return true;
                }
            }

            if (mAllowExpression)
            {
                if (keyword==Scanner::K_getdisabled || keyword==Scanner::K_getdistance)
                {
                    scanner.putbackKeyword (keyword, loc);
                    parseExpression (scanner, loc);
                    mState = EndState;
                    return true;
                }

                if (const Extensions *extensions = getContext().getExtensions())
                {
                    char returnType;
                    std::string argumentType;

                    bool hasExplicit = !mExplicit.empty();

                    if (extensions->isFunction (keyword, returnType, argumentType, hasExplicit))
                    {
                        if (!hasExplicit && !mExplicit.empty())
                        {
                            getErrorHandler().warning ("stray explicit reference (ignoring it)", loc);
                            mExplicit.clear();
                        }

                        scanner.putbackKeyword (keyword, loc);
                        parseExpression (scanner, loc);
                        mState = EndState;
                        return true;
                    }
                }
            }
        }

        if (mState==ExplicitState)
        {
            // drop stray explicit reference
            getErrorHandler().warning ("stray explicit reference (ignoring it)", loc);
            mState = BeginState;
            mExplicit.clear();
        }

        if (mState==BeginState)
        {
            switch (keyword)
            {
                case Scanner::K_short:
                case Scanner::K_long:
                case Scanner::K_float:
                {
                    if (!getContext().canDeclareLocals())
                    {
                        getErrorHandler().error (
                            "local variables can't be declared in this context", loc);
                        SkipParser skip (getErrorHandler(), getContext());
                        scanner.scan (skip);
                        return true;
                    }

                    DeclarationParser declaration (getErrorHandler(), getContext(), mLocals);
                    if (declaration.parseKeyword (keyword, loc, scanner))
                        scanner.scan (declaration);

                    return true;
                }

                case Scanner::K_set: mState = SetState; return true;
                case Scanner::K_messagebox: mState = MessageState; return true;

                case Scanner::K_return:

                    Generator::exit (mCode);
                    mState = EndState;
                    return true;

                case Scanner::K_startscript:

                    mExprParser.parseArguments ("c", scanner, mCode, true);
                    Generator::startScript (mCode);
                    mState = EndState;
                    return true;

                case Scanner::K_stopscript:

                    mExprParser.parseArguments ("c", scanner, mCode, true);
                    Generator::stopScript (mCode);
                    mState = EndState;
                    return true;

                case Scanner::K_else:

                    getErrorHandler().warning ("stay else (ignoring it)", loc);
                    mState = EndState;
                    return true;

                case Scanner::K_endif:

                    getErrorHandler().warning ("stay endif (ignoring it)", loc);
                    mState = EndState;
                    return true;

                case Scanner::K_begin:

                    getErrorHandler().warning ("stay begin (ignoring it)", loc);
                    mState = EndState;
                    return true;
            }
        }
        else if (mState==SetLocalVarState && keyword==Scanner::K_to)
        {
            mExprParser.reset();
            scanner.scan (mExprParser);

            std::vector<Interpreter::Type_Code> code;
            char type = mExprParser.append (code);

            Generator::assignToLocal (mCode, mLocals.getType (mName),
                mLocals.getIndex (mName), code, type);

            mState = EndState;
            return true;
        }
        else if (mState==SetGlobalVarState && keyword==Scanner::K_to)
        {
            mExprParser.reset();
            scanner.scan (mExprParser);

            std::vector<Interpreter::Type_Code> code;
            char type = mExprParser.append (code);

            Generator::assignToGlobal (mCode, mLiterals, mType, mName, code, type);

            mState = EndState;
            return true;
        }
        else if (mState==SetMemberVarState2 && keyword==Scanner::K_to)
        {
            mExprParser.reset();
            scanner.scan (mExprParser);

            std::vector<Interpreter::Type_Code> code;
            char type = mExprParser.append (code);

            Generator::assignToMember (mCode, mLiterals, mType, mMemberName, mName, code, type,
                !mReferenceMember);

            mState = EndState;
            return true;
        }

        if (mAllowExpression)
        {
            if (keyword==Scanner::K_getsquareroot || keyword==Scanner::K_menumode ||
                keyword==Scanner::K_random || keyword==Scanner::K_scriptrunning ||
                keyword==Scanner::K_getsecondspassed)
            {
                scanner.putbackKeyword (keyword, loc);
                parseExpression (scanner, loc);
                mState = EndState;
                return true;
            }
        }

        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool LineParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline &&
            (mState==EndState || mState==BeginState || mState==PotentialEndState))
            return false;

        if (code==Scanner::S_comma && mState==MessageState)
        {
            mState = MessageCommaState;
            return true;
        }

        if (code==Scanner::S_ref && mState==PotentialExplicitState)
        {
            mState = ExplicitState;
            return true;
        }

        if (code==Scanner::S_member && mState==PotentialExplicitState)
        {
            mState = MemberState;
            parseExpression (scanner, loc);
            mState = EndState;
            return true;
        }

        if (code==Scanner::S_newline && mState==MessageButtonState)
        {
            Generator::message (mCode, mLiterals, mName, mButtons);
            return false;
        }

        if (code==Scanner::S_comma && mState==MessageButtonState)
        {
            mState = MessageButtonCommaState;
            return true;
        }

        if (code==Scanner::S_member && mState==SetPotentialMemberVarState)
        {
            mState = SetMemberVarState;
            return true;
        }

        if (mAllowExpression && mState==BeginState &&
            (code==Scanner::S_open || code==Scanner::S_minus))
        {
            scanner.putbackSpecial (code, loc);
            parseExpression (scanner, loc);
            mState = EndState;
            return true;
        }

        return Parser::parseSpecial (code, loc, scanner);
    }

    void LineParser::reset()
    {
        mState = BeginState;
        mName.clear();
        mExplicit.clear();
    }
}
