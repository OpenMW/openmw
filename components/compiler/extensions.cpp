#include "extensions.hpp"

#include <cassert>
#include <stdexcept>

#include "generator.hpp"
#include "literals.hpp"

namespace Compiler
{
    Extensions::Extensions() : mNextKeywordIndex (-1) {}

    int Extensions::searchKeyword (const std::string& keyword) const
    {
        auto iter = mKeywords.find (keyword);
        if (iter==mKeywords.end())
            return 0;

        return iter->second;
    }

    bool Extensions::isFunction (int keyword, ScriptReturn& returnType, ScriptArgs& argumentType,
        bool& explicitReference) const
    {
        auto iter = mFunctions.find (keyword);
        if (iter==mFunctions.end())
            return false;

        if (explicitReference && iter->second.mCodeExplicit==-1)
            explicitReference = false;

        returnType = iter->second.mReturn;
        argumentType = iter->second.mArguments;
        return true;
    }

    bool Extensions::isInstruction (int keyword, ScriptArgs& argumentType,
        bool& explicitReference) const
    {
        auto iter = mInstructions.find (keyword);
        if (iter==mInstructions.end())
            return false;

        if (explicitReference && iter->second.mCodeExplicit==-1)
            explicitReference = false;

        argumentType = iter->second.mArguments;
        return true;
    }

    void Extensions::registerFunction (const std::string& keyword, ScriptReturn returnType,
        const ScriptArgs& argumentType, int code, int codeExplicit)
    {
        Function function;

        if (argumentType.find ('/')==std::string::npos)
        {
            function.mSegment = 5;
            assert (code>=33554432 && code<=67108863);
            assert (codeExplicit==-1 || (codeExplicit>=33554432 && codeExplicit<=67108863));
        }
        else
        {
            function.mSegment = 3;
            assert (code>=0x20000 && code<=0x2ffff);
            assert (codeExplicit==-1 || (codeExplicit>=0x20000 && codeExplicit<=0x2ffff));
        }

        int keywordIndex = mNextKeywordIndex--;

        mKeywords.insert (std::make_pair (keyword, keywordIndex));

        function.mReturn = returnType;
        function.mArguments = argumentType;
        function.mCode = code;
        function.mCodeExplicit = codeExplicit;

        mFunctions.insert (std::make_pair (keywordIndex, function));
    }

    void Extensions::registerInstruction (const std::string& keyword,
        const ScriptArgs& argumentType, int code, int codeExplicit)
    {
        Instruction instruction;

        if (argumentType.find ('/')==std::string::npos)
        {
            instruction.mSegment = 5;
            assert (code>=33554432 && code<=67108863);
            assert (codeExplicit==-1 || (codeExplicit>=33554432 && codeExplicit<=67108863));
        }
        else
        {
            instruction.mSegment = 3;
            assert (code>=0x20000 && code<=0x2ffff);
            assert (codeExplicit==-1 || (codeExplicit>=0x20000 && codeExplicit<=0x2ffff));
        }

        int keywordIndex = mNextKeywordIndex--;

        mKeywords.insert (std::make_pair (keyword, keywordIndex));

        instruction.mArguments = argumentType;
        instruction.mCode = code;
        instruction.mCodeExplicit = codeExplicit;

        mInstructions.insert (std::make_pair (keywordIndex, instruction));
    }

    void Extensions::generateFunctionCode (int keyword, std::vector<Interpreter::Type_Code>& code,
        Literals& literals, const std::string& id, int optionalArguments) const
    {
        assert (optionalArguments>=0);

        auto iter = mFunctions.find (keyword);
        if (iter==mFunctions.end())
            throw std::logic_error ("unknown custom function keyword");

        if (optionalArguments && iter->second.mSegment!=3)
            throw std::logic_error ("functions with optional arguments must be placed into segment 3");

        if (!id.empty())
        {
            if (iter->second.mCodeExplicit==-1)
                throw std::logic_error ("explicit references not supported");

            int index = literals.addString (id);
            Generator::pushInt (code, literals, index);
        }

        switch (iter->second.mSegment)
        {
            case 3:

                if (optionalArguments>=256)
                    throw std::logic_error ("number of optional arguments is too large for segment 3");

                code.push_back (Generator::segment3 (
                    id.empty() ? iter->second.mCode : iter->second.mCodeExplicit,
                    optionalArguments));

                break;

            case 5:

                code.push_back (Generator::segment5 (
                    id.empty() ? iter->second.mCode : iter->second.mCodeExplicit));

                break;

            default:

                throw std::logic_error ("unsupported code segment");
        }
    }

    void Extensions::generateInstructionCode (int keyword,
        std::vector<Interpreter::Type_Code>& code, Literals& literals, const std::string& id,
        int optionalArguments) const
    {
        assert (optionalArguments>=0);

        auto iter = mInstructions.find (keyword);
        if (iter==mInstructions.end())
            throw std::logic_error ("unknown custom instruction keyword");

        if (optionalArguments && iter->second.mSegment!=3)
            throw std::logic_error ("instructions with optional arguments must be placed into segment 3");

        if (!id.empty())
        {
            if (iter->second.mCodeExplicit==-1)
                throw std::logic_error ("explicit references not supported");

            int index = literals.addString (id);
            Generator::pushInt (code, literals, index);
        }

        switch (iter->second.mSegment)
        {
            case 3:

                if (optionalArguments>=256)
                    throw std::logic_error ("number of optional arguments is too large for segment 3");

                code.push_back (Generator::segment3 (
                    id.empty() ? iter->second.mCode : iter->second.mCodeExplicit,
                    optionalArguments));

                break;

            case 5:

                code.push_back (Generator::segment5 (
                    id.empty() ? iter->second.mCode : iter->second.mCodeExplicit));

                break;

            default:

                throw std::logic_error ("unsupported code segment");
        }
    }

    void Extensions::listKeywords (std::vector<std::string>& keywords) const
    {
        for (const auto & mKeyword : mKeywords)
            keywords.push_back (mKeyword.first);
    }
}
