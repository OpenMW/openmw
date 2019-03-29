#include "locals.hpp"

#include <components/esm/loadscpt.hpp>
#include <components/esm/variant.hpp>
#include <components/esm/locals.hpp>
#include <components/debug/debuglog.hpp>
#include <components/compiler/locals.hpp>
#include <components/compiler/exception.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

namespace MWScript
{
    void Locals::ensure (const std::string& scriptName)
    {
        if (!mInitialised)
        {
            const ESM::Script *script = MWBase::Environment::get().getWorld()->getStore().
                get<ESM::Script>().find (scriptName);

            configure (*script);
        }
    }

    Locals::Locals() : mInitialised (false) {}

    bool Locals::configure (const ESM::Script& script)
    {
        if (mInitialised)
            return false;

        const Compiler::Locals& locals =
            MWBase::Environment::get().getScriptManager()->getLocals (script.mId);

        mShorts.clear();
        mShorts.resize (locals.get ('s').size(), 0);
        mLongs.clear();
        mLongs.resize (locals.get ('l').size(), 0);
        mFloats.clear();
        mFloats.resize (locals.get ('f').size(), 0);

        mInitialised = true;
        return true;
    }

    bool Locals::isEmpty() const
    {
        return (mShorts.empty() && mLongs.empty() && mFloats.empty());
    }

    bool Locals::hasVar(const std::string &script, const std::string &var)
    {
        try
        {
            ensure (script);

            const Compiler::Locals& locals =
                MWBase::Environment::get().getScriptManager()->getLocals(script);
            int index = locals.getIndex(var);
            return (index != -1);
        }
        catch (const Compiler::SourceException&)
        {
            return false;
        }
    }

    int Locals::getIntVar(const std::string &script, const std::string &var)
    {
        ensure (script);

        const Compiler::Locals& locals = MWBase::Environment::get().getScriptManager()->getLocals(script);
        int index = locals.getIndex(var);
        char type = locals.getType(var);
        if(index != -1)
        {
            switch(type)
            {
                case 's':
                    return mShorts.at (index);

                case 'l':
                    return mLongs.at (index);

                case 'f':
                    return static_cast<int>(mFloats.at(index));
                default:
                    return 0;
            }
        }
        return 0;
    }

    float Locals::getFloatVar(const std::string &script, const std::string &var)
    {
        ensure (script);

        const Compiler::Locals& locals = MWBase::Environment::get().getScriptManager()->getLocals(script);
        int index = locals.getIndex(var);
        char type = locals.getType(var);
        if(index != -1)
        {
            switch(type)
            {
                case 's':
                    return mShorts.at (index);

                case 'l':
                    return mLongs.at (index);

                case 'f':
                    return mFloats.at(index);
                default:
                    return 0;
            }
        }
        return 0;
    }

    bool Locals::setVarByInt(const std::string& script, const std::string& var, int val)
    {
        ensure (script);

        const Compiler::Locals& locals = MWBase::Environment::get().getScriptManager()->getLocals(script);
        int index = locals.getIndex(var);
        char type = locals.getType(var);
        if(index != -1)
        {
            switch(type)
            {
                case 's':
                    mShorts.at (index) = val; break;

                case 'l':
                    mLongs.at (index) = val; break;

                case 'f':
                    mFloats.at(index) = static_cast<float>(val); break;
            }
            return true;
        }
        return false;
    }

    bool Locals::write (ESM::Locals& locals, const std::string& script) const
    {
        if (!mInitialised)
            return false;

        try
        {
            const Compiler::Locals& declarations =
                MWBase::Environment::get().getScriptManager()->getLocals(script);

            for (int i=0; i<3; ++i)
            {
                char type = 0;

                switch (i)
                {
                    case 0: type = 's'; break;
                    case 1: type = 'l'; break;
                    case 2: type = 'f'; break;
                }

                const std::vector<std::string>& names = declarations.get (type);

                for (int i2=0; i2<static_cast<int> (names.size()); ++i2)
                {
                    ESM::Variant value;

                    switch (i)
                    {
                        case 0: value.setType (ESM::VT_Int); value.setInteger (mShorts.at (i2)); break;
                        case 1: value.setType (ESM::VT_Int); value.setInteger (mLongs.at (i2)); break;
                        case 2: value.setType (ESM::VT_Float); value.setFloat (mFloats.at (i2)); break;
                    }

                    locals.mVariables.push_back (std::make_pair (names[i2], value));
                }
            }
        }
        catch (const Compiler::SourceException&)
        {
        }

        return true;
    }

    void Locals::read (const ESM::Locals& locals, const std::string& script)
    {
        ensure (script);

        try
        {
            const Compiler::Locals& declarations =
                MWBase::Environment::get().getScriptManager()->getLocals(script);

            int index = 0, numshorts = 0, numlongs = 0;
            for (unsigned int v=0; v<locals.mVariables.size();++v)
            {
                ESM::VarType type = locals.mVariables[v].second.getType();
                if (type == ESM::VT_Short)
                    ++numshorts;
                else if (type == ESM::VT_Int)
                    ++numlongs;
            }

            for (std::vector<std::pair<std::string, ESM::Variant> >::const_iterator iter
                = locals.mVariables.begin(); iter!=locals.mVariables.end(); ++iter,++index)
            {
                if (iter->first.empty())
                {
                    // no variable names available (this will happen for legacy, i.e. ESS-imported savegames only)
                    try
                    {
                        if (index >= numshorts+numlongs)
                            mFloats.at(index - (numshorts+numlongs)) = iter->second.getFloat();
                        else if (index >= numshorts)
                            mLongs.at(index - numshorts) = iter->second.getInteger();
                        else
                            mShorts.at(index) = iter->second.getInteger();
                    }
                    catch (std::exception& e)
                    {
                        Log(Debug::Error) << "Failed to read local variable state for script '"
                                          << script << "' (legacy format): " << e.what()
                                          << "\nNum shorts: " << numshorts << " / " << mShorts.size()
                                          << " Num longs: " << numlongs << " / " << mLongs.size();
                    }
                }
                else
                {
                    char type =  declarations.getType (iter->first);
                    int index2 = declarations.getIndex (iter->first);

                    // silently ignore locals that don't exist anymore
                    if (type == ' ' || index2 == -1)
                        continue;

                    try
                    {
                        switch (type)
                        {
                            case 's': mShorts.at (index2) = iter->second.getInteger(); break;
                            case 'l': mLongs.at (index2) = iter->second.getInteger(); break;
                            case 'f': mFloats.at (index2) = iter->second.getFloat(); break;
                        }
                    }
                    catch (...)
                    {
                        // ignore type changes
                        /// \todo write to log
                    }
                }
            }
        }
        catch (const Compiler::SourceException&)
        {
        }
    }
}
