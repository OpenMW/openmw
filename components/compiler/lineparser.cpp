#include "lineparser.hpp"

#include <memory>

#include <components/misc/stringops.hpp>

#include "scanner.hpp"
#include "context.hpp"
#include "errorhandler.hpp"
#include "skipparser.hpp"
#include "locals.hpp"
#include "generator.hpp"
#include "extensions.hpp"
#include "declarationparser.hpp"
#include "exception.hpp"

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

                Generator::report (mCode, mLiterals, "%d");
                break;

            case 'f':

                Generator::report (mCode, mLiterals, "%f");
                break;

            default:

                throw std::runtime_error ("Unknown expression result type");
        }
    }

    LineParser::LineParser (ErrorHandler& errorHandler, const Context& context, Locals& locals,
        Literals& literals, std::vector<Interpreter::Type_Code>& code, bool allowExpression)
    : Parser (errorHandler, context), mLocals (locals), mLiterals (literals), mCode (code),
       mState (BeginState), mReferenceMember(false), mButtons(0), mType(0),
       mExprParser (errorHandler, context, locals, literals), mAllowExpression (allowExpression)
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
            getErrorHandler().warning ("Stray string argument", loc);
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
            mMemberName = Misc::StringUtils::lowerCase (name);
            std::pair<char, bool> type = getContext().getMemberType (mMemberName, mName);

            if (type.first!=' ')
            {
                mState = SetMemberVarState2;
                mType = type.first;
                mReferenceMember = type.second;
                return true;
            }

            getErrorHandler().error ("Unknown variable", loc);
            SkipParser skip (getErrorHandler(), getContext());
            scanner.scan (skip);
            return false;
        }

        if (mState==MessageState || mState==MessageCommaState)
        {
            GetArgumentsFromMessageFormat processor;
            processor.process(name);
            std::string arguments = processor.getArguments();

            if (!arguments.empty())
            {
                mExprParser.reset();
                mExprParser.parseArguments (arguments, scanner, mCode);
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
        if (mState==MessageState || mState==MessageCommaState)
        {
            if (const Extensions *extensions = getContext().getExtensions())
            {
                std::string argumentType; // ignored
                bool hasExplicit = false; // ignored
                if (extensions->isInstruction (keyword, argumentType, hasExplicit))
                {
                    // pretend this is not a keyword
                    std::string name = loc.mLiteral;
                    if (name.size()>=2 && name[0]=='"' && name[name.size()-1]=='"')
                        name = name.substr (1, name.size()-2);
                    return parseName (name, loc, scanner);
                }
            }
        }

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
            getErrorHandler().warning ("Unknown variable", loc);
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

                case Scanner::K_startscript:

                    mExprParser.parseArguments ("c", scanner, mCode);
                    Generator::startScript (mCode, mLiterals, mExplicit);
                    mState = EndState;
                    return true;

                case Scanner::K_stopscript:

                    mExprParser.parseArguments ("c", scanner, mCode);
                    Generator::stopScript (mCode);
                    mState = EndState;
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
                        getErrorHandler().warning ("Stray explicit reference", loc);
                        mExplicit.clear();
                    }

                    try
                    {
                        // workaround for broken positioncell instructions.
                        /// \todo add option to disable this
                        std::unique_ptr<ErrorDowngrade> errorDowngrade (nullptr);
                        if (Misc::StringUtils::lowerCase (loc.mLiteral)=="positioncell")
                            errorDowngrade.reset (new ErrorDowngrade (getErrorHandler()));

                        std::vector<Interpreter::Type_Code> code;
                        int optionals = mExprParser.parseArguments (argumentType, scanner, code, keyword);
                        mCode.insert (mCode.end(), code.begin(), code.end());
                        extensions->generateInstructionCode (keyword, mCode, mLiterals,
                            mExplicit, optionals);
                    }
                    catch (const SourceException&)
                    {
                        // Ignore argument exceptions for positioncell.
                        /// \todo add option to disable this
                        if (Misc::StringUtils::lowerCase (loc.mLiteral)=="positioncell")
                        {
                            SkipParser skip (getErrorHandler(), getContext());
                            scanner.scan (skip);
                            return false;
                        }

                        throw;
                    }

                    mState = EndState;
                    return true;
                }
            }

            if (keyword==Scanner::K_getdisabled || keyword==Scanner::K_getdistance)
            {
                if (mAllowExpression)
                {
                    scanner.putbackKeyword (keyword, loc);
                    parseExpression (scanner, loc);
                }
                else
                {
                    getErrorHandler().warning ("Unexpected naked expression", loc);
                }
                mState = EndState;
                return true;
            }

            if (const Extensions *extensions = getContext().getExtensions())
            {
                char returnType;
                std::string argumentType;

                bool hasExplicit = mState==ExplicitState;

                if (extensions->isFunction (keyword, returnType, argumentType, hasExplicit))
                {
                    if (!hasExplicit && mState==ExplicitState)
                    {
                        getErrorHandler().warning ("Stray explicit reference", loc);
                        mExplicit.clear();
                    }

                    if (mAllowExpression)
                    {
                        scanner.putbackKeyword (keyword, loc);
                        parseExpression (scanner, loc);
                    }
                    else
                    {
                        std::vector<Interpreter::Type_Code> code;
                        int optionals = mExprParser.parseArguments (argumentType, scanner, code, keyword);
                        mCode.insert(mCode.end(), code.begin(), code.end());
                        extensions->generateFunctionCode (keyword, mCode, mLiterals, mExplicit, optionals);
                    }
                    mState = EndState;
                    return true;
                }
            }
        }

        if (mState==ExplicitState)
        {
            // drop stray explicit reference
            getErrorHandler().warning ("Stray explicit reference", loc);
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
                        getErrorHandler().error("Local variables cannot be declared in this context", loc);
                        SkipParser skip (getErrorHandler(), getContext());
                        scanner.scan (skip);
                        return true;
                    }

                    DeclarationParser declaration (getErrorHandler(), getContext(), mLocals);
                    if (declaration.parseKeyword (keyword, loc, scanner))
                        scanner.scan (declaration);

                    return false;
                }

                case Scanner::K_set: mState = SetState; return true;

                case Scanner::K_messagebox:

                    mState = MessageState;
                    scanner.enableStrictKeywords();
                    return true;

                case Scanner::K_return:

                    Generator::exit (mCode);
                    mState = EndState;
                    return true;

                case Scanner::K_stopscript:

                    mExprParser.parseArguments ("c", scanner, mCode);
                    Generator::stopScript (mCode);
                    mState = EndState;
                    return true;

                case Scanner::K_else:

                    getErrorHandler().warning ("Stray else", loc);
                    mState = EndState;
                    return true;

                case Scanner::K_endif:

                    getErrorHandler().warning ("Stray endif", loc);
                    mState = EndState;
                    return true;

                case Scanner::K_begin:

                    getErrorHandler().warning ("Stray begin", loc);
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
        if (mState==EndState && code==Scanner::S_open)
        {
            getErrorHandler().warning ("Stray '[' or '(' at the end of the line",
                loc);
            return true;
        }

        if (code==Scanner::S_newline &&
            (mState==EndState || mState==BeginState || mState==PotentialEndState))
            return false;

        if (code==Scanner::S_comma && mState==MessageState)
        {
            mState = MessageCommaState;
            return true;
        }

        if (code==Scanner::S_ref && mState==SetPotentialMemberVarState)
        {
            getErrorHandler().warning ("Stray explicit reference", loc);
            mState = SetState;
            return true;
        }

        if (code==Scanner::S_ref && mState==PotentialExplicitState)
        {
            mState = ExplicitState;
            return true;
        }

        if (code==Scanner::S_member && mState==PotentialExplicitState && mAllowExpression)
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
            (code==Scanner::S_open || code==Scanner::S_minus || code==Scanner::S_plus))
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

    void GetArgumentsFromMessageFormat::visitedPlaceholder(Placeholder placeholder, char /*padding*/, int /*width*/, int /*precision*/, Notation /*notation*/)
    {
        switch (placeholder)
        {
            case StringPlaceholder:
                mArguments += 'S';
                break;
            case IntegerPlaceholder:
                mArguments += 'l';
                break;
            case FloatPlaceholder:
                mArguments += 'f';
                break;
            default:
                break;
        }
    }

}
